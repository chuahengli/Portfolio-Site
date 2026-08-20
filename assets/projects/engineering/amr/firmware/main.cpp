#include <Wire.h>
#include "Config.h"
#include "Axis.h"

// Final firmware for the Arm Subsystem PCB: TWO independent axes (X and Y),
// each with 2 arms (ToF + servo + retract-home limit) sharing one external
// BTS7960 motor, plus per-axis travel hard-stops. Both axes tick concurrently.
// All 4 servos are driven off one shared PCA9685 (I2C), not GPIO -- this keeps
// them out of the way of the BTS7960 motors' LEDC PWM channels.
Axis axisX, axisY;
Adafruit_PWMServoDriver pwm(PCA9685_ADDR);

// Live servo slew rate, shared by both axes. Tunable from the console with
// '-' / '+' so you can find a speed you like without a reflash; once you have
// it, put the number in cfg.servoSpeedDegPerSec below (or in Flipper.h).
float servoSpeedDps = FLIPPER_DEFAULT_SPEED_DPS;
const float SERVO_SPEED_STEP_DPS = 15.0f;

void applyServoSpeed(float dps) {
  axisX.setServoSpeed(dps);
  axisY.setServoSpeed(dps);
  servoSpeedDps = axisX.servoSpeed();   // read back, in case it was clamped
}

// ---------------------------------------------------------------------------
// 'b' = both axes, SEQUENTIALLY: axis X's whole cycle, then axis Y's.
//
// They used to run concurrently, but a VL53L0X read BLOCKS for tens of ms and
// loop() ticks the axes one after the other -- so while axis Y sat inside its
// sensor reads, NOTHING was watching axis X's end-stops, and vice versa. Two
// extending axes pushed the loop period to ~200 ms, which is both a long blind
// window on the hard stops and a ~600 ms delay on the 3-sample air confirm.
//
// Running one axis at a time means the idle axis ticks in microseconds, so the
// moving one is serviced twice as often. It also keeps one motor and one set of
// end-stops in play at a time, and halves the peak current on the shared 5V
// rail (a brownout mid-extend would be a nasty way to lose control of a motor).
//
// This is a mitigation, NOT a fix: the moving axis still blocks ITSELF for
// ~50-100 ms per tick on its own two ToF reads. The real fix is continuous
// (non-blocking) ranging.
// ---------------------------------------------------------------------------
// Full mission: clamp X, clamp Y, signal CLAMPED and hold while the robot
// drives to the target, then release and stow X, then Y.
enum SeqStage {
  SEQ_IDLE,
  SEQ_CLAMP_X,   // axis X cycling to its clamp
  SEQ_CLAMP_Y,   // axis Y cycling to its clamp
  SEQ_HOLD,      // both clamped, signal HIGH, waiting for the release key
  SEQ_REL_X,     // axis X unloading / flipping down / stowing
  SEQ_REL_Y
};
SeqStage seqStage = SEQ_IDLE;

// HIGH = both axes clamped and holding, safe for the robot to move.
// The pin is driven on every call (so "make it safe" is always unconditional);
// only the console line is suppressed when nothing actually changed.
bool clampSignalOn = false;
void setClampSignal(bool on) {
  digitalWrite(CLAMP_SIGNAL_PIN, on ? HIGH : LOW);
  if (on == clampSignalOn) return;
  clampSignalOn = on;
  Serial.print(F("=== CLAMP SIGNAL (GPIO ")); Serial.print(CLAMP_SIGNAL_PIN);
  Serial.println(on ? F(") -> HIGH : trolley clamped, safe to move")
                    : F(") -> LOW : not clamped"));
}

// ---------------------------------------------------------------------------
// ONE AXIS MOVES AT A TIME -- enforced on the manual keys, not just in 'b'.
//
// The end-stop guard only runs when tick() runs. A VL53L0X read BLOCKS for
// ~50 ms, so while axis Y is inside its two reads, NOTHING is sampling axis X's
// travel MIN/MAX -- and vice versa. One axis moving has a worst-case blind
// window of one ToF read; two axes moving has three, because the idle-axis tick
// stops being free. travelMin/travelMax must never go unwatched while a motor
// is driving toward them, so the second mover is refused outright.
//
// 'b' already sequenced the axes (see the note above). This closes the same
// hole on 'x'/'y' and the jog keys, which went straight to the axis.
bool axisBusy(Axis& ax, const char* what) {
  if (!ax.isRunning() && !ax.isJogging()) return false;
  Serial.print(F("=== ")); Serial.print(what);
  Serial.print(F(" refused: axis ")); Serial.print(ax.name());
  Serial.print(F(" is still moving (")); Serial.print(ax.stateName());
  Serial.println(F("). Press 's' first. ==="));
  return true;
}

// Same rule, applied to the BLOCKING self-tests. Those stall loop() for
// hundreds of ms ('4', '5') to 15 seconds ('m') -- and a stalled loop() is a
// stalled end-stop guard for BOTH axes. Axis::blockingTestRefused() already
// covers the axis the test belongs to; this covers the other one, which no
// Axis object can see.
bool anyAxisBusy(const char* what) {
  return axisBusy(axisX, what) || axisBusy(axisY, what);
}

void cancelSequence(const char* why) {
  if (seqStage == SEQ_IDLE) return;
  seqStage = SEQ_IDLE;
  setClampSignal(false);        // never leave "safe to move" asserted after an abort
  Serial.print(F("=== SEQUENCE cancelled: ")); Serial.println(why);
}

void startSequence() {
  // The whole point of the sequence is that ONE axis moves at a time. If the
  // other axis is already mid-cycle (a stray 'x'/'y', or a second press of
  // 'b'), starting here would run both at once -- exactly what this replaced.
  if (axisX.isRunning() || axisY.isRunning()) {
    Serial.println(F("=== SEQUENCE refused: an axis is already running. Press 's' first. ==="));
    return;
  }
  Serial.println(F("\n=== SEQUENCE: clamp X, clamp Y, then hold ==="));
  axisX.start();
  seqStage = SEQ_CLAMP_X;
}

// Release: flippers down and stow, X first, then Y. Also usable without a
// sequence -- after a manual 'x'/'y' that left an axis clamped.
void startRelease() {
  if (!axisX.isClamped() && !axisY.isClamped()) {
    Serial.println(F("=== release refused: neither axis is CLAMPED ==="));
    return;
  }
  setClampSignal(false);        // we are about to let go: no longer safe to move
  Serial.println(F("\n=== RELEASE: unload, flip down, stow. X first, then Y ==="));
  if (axisX.isClamped()) {
    axisX.release();
    seqStage = SEQ_REL_X;
  } else {
    axisY.release();
    seqStage = SEQ_REL_Y;
  }
}

// Runs after both ticks, so the states it reads are this loop's.
void serviceSequence() {
  switch (seqStage) {
    case SEQ_IDLE:
      break;

    case SEQ_CLAMP_X:
      if (axisX.isRunning()) break;
      // A faulted axis means something is wrong with the RIG, not just with
      // this cycle. Don't start a second motor on top of it.
      if (axisX.state() == Axis::FAULT) {
        cancelSequence("axis X FAULTED - axis Y not started");
        break;
      }
      Serial.println(F("=== axis X clamped -> starting axis Y ==="));
      axisY.start();
      seqStage = SEQ_CLAMP_Y;
      break;

    case SEQ_CLAMP_Y:
      if (axisY.isRunning()) break;
      if (axisY.state() == Axis::FAULT) {
        cancelSequence("axis Y FAULTED");
        break;
      }
      // Only assert the signal once BOTH axes are actually holding. A single
      // clamped axis is not a held trolley.
      if (axisX.isClamped() && axisY.isClamped()) {
        seqStage = SEQ_HOLD;
        Serial.println(F("\n=== BOTH AXES CLAMPED - trolley held ==="));
        setClampSignal(true);
        Serial.println(F("=== drive to the target now. 'c' releases and stows. ==="));
      } else {
        cancelSequence("a cycle ended without clamping - check the arm limits");
      }
      break;

    case SEQ_HOLD:
      break;   // holding for the robot's move. 'c' starts the release.

    case SEQ_REL_X:
      if (axisX.isRunning()) break;
      if (axisX.state() == Axis::FAULT) {
        cancelSequence("axis X FAULTED during release - axis Y not released");
        break;
      }
      if (axisY.isClamped()) {
        Serial.println(F("=== axis X stowed -> releasing axis Y ==="));
        axisY.release();
        seqStage = SEQ_REL_Y;
      } else {
        seqStage = SEQ_IDLE;
        Serial.println(F("=== RELEASE complete ==="));
      }
      break;

    case SEQ_REL_Y:
      if (axisY.isRunning()) break;
      if (axisY.state() == Axis::FAULT) {
        cancelSequence("axis Y FAULTED during release");
        break;
      }
      seqStage = SEQ_IDLE;
      Serial.println(F("=== RELEASE complete: both axes stowed ==="));
      break;
  }
}

// Raw PCA9685 sweep, completely bypassing Axis/Arm/Flipper -- talks to the
// chip the exact same way the known-working teammate sketch did. If a servo
// doesn't move under THIS test, the fault is upstream of our code (wiring,
// V+ power, channel, etc.); if it does move here but not via '4'/'9'/'q'/'w',
// the bug is in how Axis/Arm/Flipper plumb the channel/pulse through.
//
// Side effect: this moves the horns behind Flipper's back, so its idea of where
// each servo is goes stale and the NEXT flip will jump to catch up before it
// ramps. Run a servo test ('4'/'9') after a sweep to resync.
void rawServoSweep() {
  // This blocks for ~3 s with no tick() at all -- no end-stop guard, no servo
  // ramp, nothing. It must never run while a motor could be moving.
  //
  // isJogging() matters as much as isRunning() here: a jog leaves _state at
  // IDLE, so the first two checks miss it entirely and the motor would run
  // blind for the whole sweep. jogTimeoutMs can't catch it either -- that timer
  // lives in serviceJog(), which needs the tick() this function never gives it.
  if (axisX.isRunning() || axisY.isRunning() ||
      axisX.isClamped() || axisY.isClamped() ||
      axisX.isJogging() || axisY.isJogging()) {
    Serial.println(F("[raw] refused: an axis is running, clamped or jogging. Press 's' (or 'c') first."));
    return;
  }
  const uint8_t channels[4] = {0, 1, 2, 3};
  const int angles[3] = {0, 90, 180};   // matches teammate's EXTRACTING/FLIPPING range
  for (int a = 0; a < 3; a++) {
    int pulseUs = map(angles[a], 0, 180, 500, 2500);   // teammate's SERVO_MIN/MAX_US
    int ticks = (pulseUs * 4096) / (1000000 / 50);     // teammate's pulseUsToTicks @ 50Hz
    Serial.print("[raw] angle="); Serial.print(angles[a]);
    Serial.print(" pulse="); Serial.print(pulseUs);
    Serial.print("us ticks="); Serial.println(ticks);
    for (uint8_t i = 0; i < 4; i++) {
      uint8_t rc = pwm.setPWM(channels[i], 0, ticks);
      Serial.print("  ch="); Serial.print(channels[i]);
      Serial.print(" setPWM="); Serial.println(rc == 0 ? "OK" : "I2C WRITE FAILED");
    }
    delay(1000);
  }
}

void scan(const char* label) {
  Serial.print("--- "); Serial.println(label);
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("  found 0x"); Serial.println(a, HEX);
      found++;
    }
  }
  if (!found) Serial.println(F("  (nothing)"));
}

void setup() {
  Serial.begin(115200);

  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);  // bounded, never hangs
  delay(300);                                          // let CDC enumerate before first print
  Serial.println(F("\n=== ARM SUBSYSTEM BOOT ==="));

  // Clamp signal out to the main robot controller. LOW until both axes hold.
  pinMode(CLAMP_SIGNAL_PIN, OUTPUT);
  digitalWrite(CLAMP_SIGNAL_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);   // SDA=8, SCL=9 (verified from netlist)
  Serial.println(F("[1] i2c ok"));

  // ---- PCA9685 servo driver (all 4 servos, over I2C -- no GPIO/LEDC used) ----
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ_HZ);
  delay(10);
  Serial.println(F("[2] PCA9685 servo driver ready"));

  // ---- XSHUT dance: hold EVERY sensor in reset before any begin() ----
  // Waking + renaming them one at a time avoids the default-0x29 collision.
  pinMode(XSHUT_X0, OUTPUT); digitalWrite(XSHUT_X0, LOW);
  pinMode(XSHUT_X1, OUTPUT); digitalWrite(XSHUT_X1, LOW);
  pinMode(XSHUT_Y0, OUTPUT); digitalWrite(XSHUT_Y0, LOW);
  pinMode(XSHUT_Y1, OUTPUT); digitalWrite(XSHUT_Y1, LOW);
  delay(10);
  Serial.println(F("[3] all xshut low"));

  // ---- Axis X wiring (verified) ----
  Axis::Pins pinsX;
  pinsX.arms[0].limitSwitch = LIMIT_X0; pinsX.arms[0].servoChannel = SERVO_CH_X0; pinsX.arms[0].xshut = XSHUT_X0;
  pinsX.arms[1].limitSwitch = LIMIT_X1; pinsX.arms[1].servoChannel = SERVO_CH_X1; pinsX.arms[1].xshut = XSHUT_X1;
  pinsX.motor.rpwm = RPWM_X; pinsX.motor.lpwm = LPWM_X; pinsX.motor.rEn = R_EN_X; pinsX.motor.lEn = L_EN_X;
  // The X motor turns out to be wired/geared backwards: RPWM physically RETRACTS.
  // Invert it here so "extend" means extend for the state machine, the travel
  // stops and the jog supervisor alike.
  pinsX.motor.invert = true;
  pinsX.travelMin = X_TRAVEL_MIN; pinsX.travelMax = X_TRAVEL_MAX;

  Axis::Config cfg;                 // per-axis tuning (defaults in Axis.h)
  cfg.motorDuty        = 130;       // 0-255 PWM (~51% speed, slowed down)
  // Servo flip speed. Hobby servos have no speed input, so Flipper ramps the
  // setpoint instead; this is that ramp rate. 90 deg/s = the 90 deg A->B swing
  // takes ~1 s (the servo's own full speed would be ~0.15 s -- a violent snap).
  cfg.servoSpeedDegPerSec = servoSpeedDps;
  cfg.airConfirmCount  = 3;         // both arms must read air 3 ticks in a row
  cfg.extendTimeoutMs  = 8000;      // safety timeouts (real motors attached)
  cfg.retractTimeoutMs = 8000;

  if (!axisX.begin("X", &pwm, pinsX, cfg, 0x30, 0x31)) {
    Serial.println(F("Axis X failed -- halting."));
    while (1) delay(100);
  }

  // ---- Axis Y wiring (verified) ----
  Axis::Pins pinsY;
  pinsY.arms[0].limitSwitch = LIMIT_Y0; pinsY.arms[0].servoChannel = SERVO_CH_Y0; pinsY.arms[0].xshut = XSHUT_Y0;
  pinsY.arms[1].limitSwitch = LIMIT_Y1; pinsY.arms[1].servoChannel = SERVO_CH_Y1; pinsY.arms[1].xshut = XSHUT_Y1;
  pinsY.motor.rpwm = RPWM_Y; pinsY.motor.lpwm = LPWM_Y; pinsY.motor.rEn = R_EN_Y; pinsY.motor.lEn = L_EN_Y;
  // NOT inverted, unlike X: verified on the rig that RPWM_Y really does extend.
  // (The old mirrored E/R keys were only a key-ordering choice, not a hint that
  // this motor ran backwards.) Leaving invert at its false default.
  pinsY.travelMin = Y_TRAVEL_MIN; pinsY.travelMax = Y_TRAVEL_MAX;

  if (!axisY.begin("Y", &pwm, pinsY, cfg, 0x32, 0x33)) {
    Serial.println(F("Axis Y failed -- halting."));
    while (1) delay(100);
  }
  scan("after X (0x30 0x31) and Y (0x32 0x33) begin");

  Serial.println(F("\nReady. Serial commands:"));
  Serial.println(F("  x / y   : run a full cycle on axis X / axis Y"));
  Serial.println(F("  b       : CLAMP - all of X, then all of Y, then hold + signal"));
  Serial.println(F("            (stops after X if X faults; 's' cancels the sequence)"));
  Serial.print(F("  c       : RELEASE - unload, flip down, stow to travel MIN (clamp signal = GPIO "));
  Serial.print(CLAMP_SIGNAL_PIN); Serial.println(F(")"));
  Serial.println(F("  e / r   : jog X motor Extend / Retract  (auto-stops on the end-stop)"));
  Serial.println(F("  E / R   : jog Y motor Extend / Retract  (auto-stops on the end-stop)"));
  Serial.println(F("  s       : E-STOP all motors + halt"));
  Serial.println(F("  ---- axis X tests ----  '1'=ToF  2=Lmt  3=Travel  4=Servo  5=Motor"));
  Serial.println(F("  ---- axis Y tests ----  '6'=ToF  7=Lmt  8=Travel  9=Servo  0=Motor"));
  Serial.println(F("  ---- limit-switch tests (no ToF needed) ----"));
  Serial.println(F("  m / M   : live switch monitor, motor OFF - press switches by hand (X / Y)"));
  Serial.println(F("  f / F   : FLIP arms up, then RETRACT until a limit switch stops it (X / Y)"));
  Serial.println(F("            (the tail of a real cycle; needs no ToF. Extend with 'e' first.)"));
  Serial.println(F("  t / T   : drive RETRACT into 'Arm Min' and prove it stops the motor (X / Y)"));
  Serial.println(F("  g / G   : drive EXTEND  into 'Arm Max' and prove it stops the motor (X / Y)"));
  Serial.println(F("  q = Y servo 1 only (PCA9685 ch0), w = Y servo 2 only (PCA9685 ch1)"));
  Serial.println(F("  p = RAW PCA9685 sweep, all 4 channels, bypasses Axis/Arm/Flipper entirely"));
  Serial.print(F("  - / +   : servo flip SPEED down / up (15 deg/s steps), now "));
  Serial.print((int)servoSpeedDps); Serial.println(F(" deg/s"));
}

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  while (Serial.available()) Serial.read();   // flush the rest of the line

  // Every key listed here either drives a motor or BLOCKS loop() -- and while
  // loop() is blocked, nothing samples travel MIN/MAX on EITHER axis. So none
  // of them may start while any motor is still moving. 's' (E-STOP), 'c', 'b'
  // and the speed keys are deliberately NOT in this set: they must stay
  // available at all times.
  if (c && strchr("1234567890mMfFtTgGqwp", c) &&
      anyAxisBusy("blocking test")) return;

  switch (c) {
    // A manual single-axis cycle overrides any sequence in progress -- but
    // never starts a SECOND motor on top of one that is already moving.
    case 'x':
      if (axisBusy(axisY, "axis X cycle")) break;
      cancelSequence("manual 'x'"); axisX.start(); break;
    case 'y':
      if (axisBusy(axisX, "axis Y cycle")) break;
      cancelSequence("manual 'y'"); axisY.start(); break;
    case 'b': startSequence();           break;
    case 'c': startRelease();            break;   // clamped -> unload, flip down, stow
    case 'e': if (!axisBusy(axisY, "X extend jog"))  axisX.jogExtend();  break;
    case 'r': if (!axisBusy(axisY, "X retract jog")) axisX.jogRetract(); break;
    // Y is no longer mirrored: E/R mean the same as e/r on X. (The old mapping
    // had capital E retract; that was a key-ordering choice, nothing to do with
    // motor wiring, and it made the two axes behave differently for no reason.)
    case 'E': if (!axisBusy(axisX, "Y extend jog"))  axisY.jogExtend();  break;
    case 'R': if (!axisBusy(axisX, "Y retract jog")) axisY.jogRetract(); break;
    // E-STOP must kill the sequence too. estop() parks the axis in STOPPED,
    // which is exactly what "cycle finished" looks like -- without this, hitting
    // 's' during axis X would immediately launch axis Y.
    case 's':
      cancelSequence("E-STOP");
      setClampSignal(false);   // unconditional: never leave "safe to move" asserted
      axisX.estop(); axisY.estop();
      break;

    // Axis X tests
    case '1': axisX.testSensor();  break;
    case '2': axisX.testLimit();   break;
    case '3': axisX.testTravel();  break;
    case '4': axisX.testServo();   break;
    case '5': axisX.testMotor();   break;
    // Axis Y tests
    case '6': axisY.testSensor();  break;
    case '7': axisY.testLimit();   break;
    case '8': axisY.testTravel();  break;
    case '9': axisY.testServo();   break;
    case '0': axisY.testMotor();   break;
    // Limit-switch bring-up (works with no ToF fitted)
    case 'm': axisX.monitorLimits();      break;   // motor OFF, press by hand
    case 'M': axisY.monitorLimits();      break;
    case 'f': axisX.testFlipRetract();      break;  // flip up -> retract -> limit stops it
    case 'F': axisY.testFlipRetract();      break;
    case 't': axisX.testTravelStop(true);  break;  // retract into "X Arm Min"
    case 'T': axisY.testTravelStop(true);  break;  // retract into "Y Arm Min"
    case 'g': axisX.testTravelStop(false); break;  // extend  into "X Arm Max"
    case 'G': axisY.testTravelStop(false); break;  // extend  into "Y Arm Max"

    case 'q': axisY.testServoArm(0); break;
    case 'w': axisY.testServoArm(1); break;
    case 'p': rawServoSweep();       break;

    // Servo flip speed, both axes. Clamped inside Flipper::setSpeed().
    case '-': applyServoSpeed(servoSpeedDps - SERVO_SPEED_STEP_DPS); break;
    case '+':
    case '=': applyServoSpeed(servoSpeedDps + SERVO_SPEED_STEP_DPS); break;
    default: break;
  }
}

void loop() {
  handleSerial();
  axisX.tick();
  axisY.tick();
  serviceSequence();   // after both ticks, so it reads this loop's states
  // NO delay() here. Only ONE axis is ever cycling under 'b' now, so the idle
  // axis falls straight through IDLE and the moving one gets serviced twice as
  // often as it did when both ran concurrently.
}
