#include "Axis.h"

bool Axis::begin(const char* name, Adafruit_PWMServoDriver* pwm, const Pins& pins, const Config& cfg,
                 uint8_t addrArm0, uint8_t addrArm1) {
  _name = name;
  _pins = pins;
  _cfg  = cfg;

  _motor.begin(_pins.motor, _name, _cfg.motorDuty);   // real BTS7960, or SIM if unwired

  // Travel hard-stops (axis end switches) -> GND, INPUT_PULLUP (active-low).
  if (_pins.travelMin >= 0) pinMode(_pins.travelMin, INPUT_PULLUP);
  if (_pins.travelMax >= 0) pinMode(_pins.travelMax, INPUT_PULLUP);

  // Bring up each arm's sensor one at a time (XSHUT dance). The caller must have
  // already held EVERY arm's XSHUT LOW, so exactly one sensor is awake at 0x29
  // when each begin() runs. arm1 stays in reset while arm0 is being renamed.
  char l0[12], l1[12];
  snprintf(l0, sizeof(l0), "%s0", _name);
  snprintf(l1, sizeof(l1), "%s1", _name);

  if (!_arms[0].begin(l0, pwm, _pins.arms[0], _cfg.airThresholdMm, addrArm0,
                      _cfg.servoSpeedDegPerSec)) {
    Serial.print("Axis "); Serial.print(_name); Serial.println(F(": arm 0 failed."));
    return false;
  }
  if (!_arms[1].begin(l1, pwm, _pins.arms[1], _cfg.airThresholdMm, addrArm1,
                      _cfg.servoSpeedDegPerSec)) {
    Serial.print("Axis "); Serial.print(_name); Serial.println(F(": arm 1 failed."));
    return false;
  }

  _arms[0].reportFlipper();
  _arms[1].reportFlipper();
  Serial.print("Axis "); Serial.print(_name); Serial.println(F(" ready (2 arms + travel stops)."));
  _state = IDLE;
  return true;
}

void Axis::start() {
  // HOMING drives the flippers back down to State A. Doing that while the
  // trolley is clamped would drop it uncontrolled, so make the caller release
  // deliberately first.
  if (_state == CLAMPED) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] CLAMPED - refusing to start a new cycle. Release first."));
    return;
  }
  if (_jog != JOG_NONE) { _motor.stop(); _jog = JOG_NONE; }   // a cycle overrides any jog
  _flipRetractPending = false;                                // ...and any armed bench test
  enterState(HOMING);
}

// CLAMPED -> UNLOADING -> RELEASING -> STOWING -> STOPPED.
void Axis::release() {
  if (_state != CLAMPED) {
    Serial.print("["); Serial.print(_name);
    Serial.print(F("] release ignored - not CLAMPED (state is "));
    Serial.print(stateName()); Serial.println(F(")"));
    return;
  }
  enterState(UNLOADING);
}

void Axis::estop() {
  stopMotor();
  _jog   = JOG_NONE;
  _flipRetractPending = false;   // don't let a pending test start the motor after an e-stop
  _state = STOPPED;
  Serial.print("["); Serial.print(_name); Serial.println(F("] E-STOP"));
}

// ---------------------------------------------------------------------------
// THE end-stop guard. Direction-aware, state-independent, unconditional.
//
// Every previous version of this logic lived inside whichever piece of code
// happened to be driving the motor -- the state machine had its checks, jogs
// had theirs, and testMotor() had none at all and drove blind for 500 ms. This
// runs everywhere the motor can be on, and does not care who started it.
// ---------------------------------------------------------------------------
void Axis::guardEndStops() {
  if (!_motor.moving()) return;              // nothing to guard

  const char* hit = nullptr;

  if (_motor.dir() == Motor::DIR_EXTEND) {
    if (travelMaxPressed()) hit = "travel MAX";
  } else {
    if (travelMinPressed()) {
      hit = "travel MIN";
    } else if (_state != STOWING && anyArmLimitPressed()) {
      // STOWING is the one documented exception: the arm limits ARE the clamp
      // position and it is retracting past them on purpose. travel MIN still
      // stops it, above -- that exception does not extend to the axis end.
      hit = "arm limit";
    }
  }

  if (!hit) return;

  _motor.stop();                             // cut it FIRST, report after
  Serial.print("["); Serial.print(_name);
  Serial.print(F("] END-STOP ")); Serial.print(hit);
  Serial.print(F(" - motor cut (was ")); Serial.print(stateName());
  Serial.println(F(")"));
}

bool Axis::blockingTestRefused(const char* what) {
  if (_state == CLAMPED) {
    Serial.print("["); Serial.print(_name); Serial.print(F("] CLAMPED - "));
    Serial.print(what); Serial.println(F(" refused. Release with 'c' first."));
    return true;
  }
  if (isRunning()) {
    Serial.print("["); Serial.print(_name); Serial.print(F("] cycle in progress ("));
    Serial.print(stateName()); Serial.print(F(") - ")); Serial.print(what);
    Serial.println(F(" refused. Press 's' first."));
    return true;
  }
  if (_jog != JOG_NONE) jogStop();   // nothing left driving underneath the test
  return false;
}

// Replaces delay() anywhere in this class that a motor might be running.
void Axis::guardedDelay(unsigned long ms) {
  const unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    guardEndStops();
    tickServos();
    delay(1);
  }
}

// The extend-side end-stop, checked often (see the note in EXTENDING). Stops
// the motor FIRST, then flips.
bool Axis::travelMaxStopAndFlip() {
  if (!travelMaxPressed()) return false;
  stopMotor();                       // motor off before anything else happens
  Serial.print("["); Serial.print(_name);
  Serial.println(F("] travel MAX reached - end of travel, flipping here"));
  _airCount = 0;
  enterState(PRE_FLIP);
  return true;
}

// ---- Servos: the flip is a ramp, so it has to be pumped every loop ----
void Axis::tickServos() { _arms[0].tick(); _arms[1].tick(); }

bool Axis::servosMoving() const { return _arms[0].servoMoving() || _arms[1].servoMoving(); }

// For the blocking self-tests only. Waits ms AND for both servos to arrive.
void Axis::settleServos(unsigned long ms) {
  const unsigned long t0  = millis();
  const unsigned long cap = ms + 5000;      // never wait forever
  while (millis() - t0 < cap) {
    guardEndStops();                        // a motor should be off here, but never assume
    tickServos();
    if (millis() - t0 >= ms && !servosMoving()) return;
    delay(1);
  }
}

void Axis::setServoSpeed(float degPerSec) {
  _arms[0].setServoSpeed(degPerSec);
  _arms[1].setServoSpeed(degPerSec);
  _cfg.servoSpeedDegPerSec = _arms[0].servoSpeed();   // read back the clamped value
  Serial.print("["); Serial.print(_name); Serial.print(F("] servo slew = "));
  Serial.print((int)_cfg.servoSpeedDegPerSec);
  Serial.print(F(" deg/s  (A<->B swing ~"));
  const float swingDeg = fabsf((float)(Flipper::ANGLE_STATE_A - Flipper::ANGLE_STATE_B));
  Serial.print((unsigned long)(swingDeg * 1000.0f / _cfg.servoSpeedDegPerSec));
  Serial.println(F(" ms)"));
}

// ---- Motor: one shared BTS7960 drives both arms on the common thread ----
void Axis::extendMotor()  { _motor.extend();  }
void Axis::retractMotor() { _motor.retract(); }
void Axis::stopMotor()    { _motor.stop();    }

// ---- Manual jog (bench use) ----
// A jog bypasses the state machine, but NOT the end-stops: startJog() refuses
// to move if we are already sitting on the stop for that direction, and
// serviceJog() (called every tick) kills the motor the moment it trips.
void Axis::jogExtend()  { startJog(JOG_EXTEND,  false); }
void Axis::jogRetract() { startJog(JOG_RETRACT, false); }

void Axis::jogStop() {
  _motor.stop();
  _jog = JOG_NONE;
}

void Axis::startJog(Jog dir, bool verbose) {
  // NEVER jog a clamped axis. The hooks are loaded and the carriage is carrying
  // the trolley, and an extend jog watches only travelMax -- so it will happily
  // drive the whole assembly into whatever is in the way.
  if (_state == CLAMPED) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] CLAMPED - jog refused. Release with 'c' first."));
    return;
  }
  // Nor one whose state machine is mid-cycle: the jog and the state machine
  // would both be driving the same motor.
  if (isRunning()) {
    Serial.print("["); Serial.print(_name);
    Serial.print(F("] cycle in progress (")); Serial.print(stateName());
    Serial.println(F(") - jog refused. Press 's' first."));
    return;
  }

  // Refuse to drive further INTO a stop that is already made.
  if (dir == JOG_RETRACT && travelMinPressed()) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] already at travel MIN - retract jog refused"));
    return;
  }
  if (dir == JOG_RETRACT && anyArmLimitPressed()) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] an arm limit is already pressed - retract jog refused"));
    return;
  }
  if (dir == JOG_EXTEND && travelMaxPressed()) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] already at travel MAX - extend jog refused"));
    return;
  }

  _jog        = dir;
  _jogVerbose = verbose;
  _jogStart   = millis();
  if (dir == JOG_EXTEND) _motor.extend();
  else                   _motor.retract();
}

// Runs every tick while a jog is active. This is what was missing: nothing
// watched the end-stops during a manual jog, so 'r' drove the X motor straight
// through "X Arm Min" without stopping.
void Axis::serviceJog() {
  if (_jog == JOG_NONE) return;

  const char* trippedBy = nullptr;
  if (_jog == JOG_RETRACT) {
    if      (travelMinPressed())   trippedBy = "travel MIN";
    else if (_arms[0].limitPressed()) trippedBy = "arm 0 limit";
    else if (_arms[1].limitPressed()) trippedBy = "arm 1 limit";
  } else {
    if (travelMaxPressed()) trippedBy = "travel MAX";
  }

  if (trippedBy) {
    unsigned long elapsed = millis() - _jogStart;
    stopMotor();
    _jog = JOG_NONE;
    Serial.print("["); Serial.print(_name); Serial.print(F("] jog stopped by "));
    Serial.print(trippedBy);
    Serial.print(F(" after ")); Serial.print(elapsed); Serial.println(F(" ms"));
    if (_jogVerbose) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] LIMIT TEST: PASS - switch cut the motor"));
      _jogVerbose = false;
    }
    return;
  }

  // The limit tests (t/T/g/G) drive a full travel on purpose, so they get the
  // longer budget. A hand jog gets the short one.
  const unsigned long budget = _jogVerbose ? _cfg.limitTestTimeoutMs : _cfg.jogTimeoutMs;
  if (budget && millis() - _jogStart > budget) {
    stopMotor();
    _jog = JOG_NONE;
    Serial.print("["); Serial.print(_name); Serial.print(F("] jog timeout after "));
    Serial.print(budget); Serial.println(F(" ms - motor stopped"));
    if (_jogVerbose) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] LIMIT TEST: FAIL - switch never tripped (check wiring/travel)"));
      _jogVerbose = false;
    }
  }
}

bool Axis::anyArmLimitPressed() { return _arms[0].limitPressed() || _arms[1].limitPressed(); }

bool Axis::travelMinPressed() const { return _pins.travelMin >= 0 && digitalRead(_pins.travelMin) == LOW; }
bool Axis::travelMaxPressed() const { return _pins.travelMax >= 0 && digitalRead(_pins.travelMax) == LOW; }

const char* Axis::stateName() const {
  switch (_state) {
    case IDLE:       return "IDLE";
    case HOMING:     return "HOMING";
    case EXTENDING:  return "EXTENDING";
    case PRE_FLIP:   return "PRE_FLIP";
    case FLIPPING:   return "FLIPPING";
    case RETRACTING: return "RETRACTING";
    case CLAMPED:    return "CLAMPED";
    case UNLOADING:  return "UNLOADING";
    case RELEASING:  return "RELEASING";
    case STOWING:    return "STOWING";
    case STOPPED:    return "STOPPED";
    case FAULT:      return "FAULT";
  }
  return "?";
}

void Axis::enterState(State s) {
  _state = s;
  _stateStart = millis();
  Serial.print("["); Serial.print(_name); Serial.print(F("] === State -> "));
  Serial.println(stateName());

  // Issue the motor command ONCE, on entry (not every tick) -- but NEVER into a
  // stop that is ALREADY made. This used to energise the motor unconditionally,
  // so entering EXTENDING while sitting on travelMax drove hard into the stop
  // for a full loop period (~200 ms with both axes ranging) before tick() got
  // its first look at the switch.
  //
  // Not starting the motor is all we have to do: tick()'s first pass sees the
  // switch and moves the state machine on by itself.
  if (s == EXTENDING) {
    if (travelMaxPressed()) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] already at travel MAX - extend NOT started"));
    } else {
      extendMotor();
    }
  }
  if (s == RETRACTING) {
    if (travelMinPressed() || anyArmLimitPressed()) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] already at a retract stop - retract NOT started"));
    } else {
      retractMotor();
    }
  }
  // Short extend to lift the trolley's weight off the hooks before the
  // flippers rotate down.
  if (s == UNLOADING) {
    if (travelMaxPressed()) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] at travel MAX - skipping the unload nudge"));
    } else {
      extendMotor();
    }
  }
  // STOWING deliberately ignores the ARM limits -- they are the clamp position
  // and we are travelling past them. Only travel MIN stops this one.
  if (s == STOWING) {
    if (travelMinPressed()) {
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] already at travel MIN - already stowed"));
    } else {
      retractMotor();
    }
  }
  if (s == RELEASING) {
    _arms[0].toStateA();       // flippers back down = let go of the trolley
    _arms[1].toStateA();
  }
  if (s == CLAMPED) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] CLAMPED - trolley held, motor off. Waiting for release."));
  }

  // START of a run: home BOTH arms' servos (State A) and clear the air counter.
  if (s == HOMING) {
    _arms[0].toStateA();
    _arms[1].toStateA();
    _airCount = 0;
  }
  if (s == STOPPED) {
    Serial.print("["); Serial.print(_name); Serial.println(F("] Cycle complete."));
  }
}

void Axis::tick() {
  guardEndStops();       // FIRST, always, whatever started the motor
  tickServos();          // advance both servo ramps (a flip takes many ticks now)
  serviceFlipRetract();  // testFlipRetract(): start the retract once the flip lands
  serviceJog();          // jog bookkeeping / PASS-FAIL reporting

  switch (_state) {

    case IDLE:
      break;   // do nothing until start() is called

    case HOMING:
      // Wait for the settle time AND for the servos to actually get home. With
      // a slow slew the ramp can outlast homeSettleMs; extending before the
      // arms are down would drag them along mid-swing.
      if (millis() - _stateStart >= _cfg.homeSettleMs && !servosMoving()) {
        enterState(EXTENDING);
      }
      break;

    case EXTENDING: {
      // TWO ways out of EXTENDING, and BOTH of them flip:
      //   1. boundary found : BOTH arms' ToF see air (the normal case)
      //   2. max range      : travelMax trips -- the arms are as far out as the
      //                       thread goes, so flip HERE rather than faulting.
      // travelMax is still a hard stop for the MOTOR; what changed is that
      // reaching it is a normal outcome, not an abort.
      //
      // ORDER MATTERS HERE. A VL53L0X read blocks for ~30-50 ms and NOTHING
      // watches the end-stop while it runs. Ranging both arms every tick left
      // the switch unsampled for ~100 ms at a time -- long enough for the
      // carriage to drive straight through it and wreck the striker. So:
      // check the stop, range at most every airPollMs, and check the stop
      // again between/after the reads.
      if (travelMaxStopAndFlip()) break;

      if (millis() - _lastAirPollMs >= _cfg.airPollMs) {
        _lastAirPollMs = millis();

        bool air0 = _arms[0].seesAir();
        if (travelMaxStopAndFlip()) break;    // switch may have closed during that read
        bool air1 = _arms[1].seesAir();
        if (travelMaxStopAndFlip()) break;

        // Advance ONLY when BOTH arms are beyond the trolley (air) for
        // airConfirmCount consecutive POLLS. If either arm still sees solid,
        // the motor keeps extending and the confirm counter resets.
        if (air0 && air1) {
          if (++_airCount >= _cfg.airConfirmCount) {
            Serial.print("["); Serial.print(_name);
            Serial.println(F("] both arms see AIR - boundary found, flipping here"));
            stopMotor();
            _airCount = 0;
            enterState(PRE_FLIP);          // buffer delay BEFORE the flip
            break;
          }
        } else {
          _airCount = 0;
        }
      }

      // Still a real fault: the motor has been running for extendTimeoutMs
      // without EITHER finding air or reaching travelMax, so something is stuck
      // or the end-stop never trips.
      if (_cfg.extendTimeoutMs && millis() - _stateStart > _cfg.extendTimeoutMs) {
        Serial.print("["); Serial.print(_name); Serial.println(F("] Extend timeout"));
        stopMotor(); enterState(FAULT);
      }
      break;
    }

    case PRE_FLIP:
      if (millis() - _stateStart >= _cfg.flipSettleMs) {
        _arms[0].toStateB();             // both arms flip together HERE
        _arms[1].toStateB();
        enterState(FLIPPING);            // buffer delay AFTER the flip
      }
      break;

    case FLIPPING:
      // Same guard as HOMING: never start retracting while an arm is still
      // swinging up. This is what makes flipSettleMs safe to leave alone when
      // you slow the servos down -- the axis just waits a bit longer.
      if (millis() - _stateStart >= _cfg.flipSettleMs && !servosMoving()) {
        enterState(RETRACTING);
      }
      break;

    case RETRACTING:
      // Safety: never drive past the retracted end-stop.
      if (travelMinPressed()) {
        Serial.print("["); Serial.print(_name); Serial.println(F("] travel MIN hit - hard stop"));
        stopMotor(); enterState(FAULT);
        break;
      }
      // motor already running. Stop as soon as EITHER arm's limit switch is hit
      // (don't wait for both). That position IS the clamp.
      if (_arms[0].limitPressed() || _arms[1].limitPressed()) {
        stopMotor(); enterState(CLAMPED);
      }
      if (_cfg.retractTimeoutMs && millis() - _stateStart > _cfg.retractTimeoutMs) {
        Serial.print("["); Serial.print(_name); Serial.println(F("] Retract timeout"));
        stopMotor(); enterState(FAULT);
      }
      break;

    case CLAMPED:
      // Hold. Motor off, flippers up, trolley held. This is the window in which
      // the robot drives to the target. Only release() moves us on.
      break;

    case UNLOADING:
      // A short timed nudge outward. Bounded by travelMax as well as the timer,
      // because this is still the motor driving toward the extended end-stop.
      if (travelMaxPressed()) {
        Serial.print("["); Serial.print(_name);
        Serial.println(F("] travel MAX during unload - stopping, flipping down here"));
        stopMotor(); enterState(RELEASING);
        break;
      }
      if (millis() - _stateStart >= _cfg.unloadMs) {
        stopMotor(); enterState(RELEASING);
      }
      break;

    case RELEASING:
      // Servos were commanded down on entry. Wait for them to ARRIVE and for
      // the settle time, so we never start stowing mid-swing.
      if (millis() - _stateStart >= _cfg.flipSettleMs && !servosMoving()) {
        enterState(STOWING);
      }
      break;

    case STOWING:
      // Retract all the way home. NOTE: the arm limits are deliberately NOT
      // checked here -- they mark the clamp position and we are travelling past
      // them on purpose. travelMin is the only switch that ends this move.
      if (travelMinPressed()) {
        Serial.print("["); Serial.print(_name);
        Serial.println(F("] travel MIN - fully stowed"));
        stopMotor(); enterState(STOPPED);
        break;
      }
      if (_cfg.stowTimeoutMs && millis() - _stateStart > _cfg.stowTimeoutMs) {
        Serial.print("["); Serial.print(_name); Serial.println(F("] Stow timeout"));
        stopMotor(); enterState(FAULT);
      }
      break;

    case STOPPED:
    case FAULT:
      break;   // idle; the main sketch decides when to start() again
  }
}

// ---------------- block-level self-tests ----------------
// Two blocking VL53L0X reads, ~50 ms each, with no tick() between them. A jog
// leaves _state at IDLE, so this test would happily run while a motor was
// driving -- ~100 ms with nothing sampling travel MIN/MAX. Kill the jog first,
// the same way monitorLimits() does.
void Axis::testSensor() {
  if (_jog != JOG_NONE) jogStop();
  _arms[0].testSensor();
  _arms[1].testSensor();
}

void Axis::testLimit() {
  _arms[0].testLimit();
  _arms[1].testLimit();
}

void Axis::testTravel() {
  Serial.print("["); Serial.print(_name);
  Serial.print(F("] travel Min = ")); Serial.println(travelMinPressed() ? "PRESSED" : "open");
  Serial.print("["); Serial.print(_name);
  Serial.print(F("] travel Max = ")); Serial.println(travelMaxPressed() ? "PRESSED" : "open");
}

// settleServos(), not delay(): the swing happens in the ramp tick, so a bare
// delay() would leave both servos frozen wherever they were.
void Axis::testServo() {
  if (blockingTestRefused("servo test")) return;
  unsigned long st = _cfg.flipSettleMs;
  Serial.print("["); Serial.print(_name); Serial.println(F("] both servos -> A"));
  _arms[0].toStateA(); _arms[1].toStateA(); settleServos(st);
  Serial.print("["); Serial.print(_name); Serial.println(F("] both servos -> B"));
  _arms[0].toStateB(); _arms[1].toStateB(); settleServos(st);
  Serial.print("["); Serial.print(_name); Serial.println(F("] both servos -> A"));
  _arms[0].toStateA(); _arms[1].toStateA(); settleServos(st);
}

void Axis::testServoArm(uint8_t arm) {
  // Arm::testServo() blocks for seconds inside Arm, which has no guard of its
  // own -- so this one has to be refused rather than supervised.
  if (blockingTestRefused("servo test")) return;
  if (arm >= NUM_ARMS) {
    Serial.print("["); Serial.print(_name); Serial.println(F("] invalid servo arm"));
    return;
  }
  Serial.print("["); Serial.print(_name); Serial.print(F("] testing servo arm "));
  Serial.println(arm);
  _arms[arm].testServo();
}

// guardedDelay(), not delay(): this used to run the motor for 500 ms with
// nothing watching a single switch.
//
// It also has to pre-check the stops, exactly like enterState() does. The guard
// inside guardedDelay() WOULD cut it on its first pass, but only AFTER the PWM
// has already gone out -- energising into a stop that is already made, for the
// length of a Serial print. Every other place that starts this motor refuses
// instead; this one used to be the exception.
void Axis::testMotor() {
  if (blockingTestRefused("motor test")) return;

  if (travelMaxPressed()) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] at travel MAX - extend half of the motor test SKIPPED"));
  } else {
    extendMotor(); guardedDelay(500);
    stopMotor();
  }
  guardedDelay(200);

  if (travelMinPressed() || anyArmLimitPressed()) {
    Serial.print("["); Serial.print(_name);
    Serial.println(F("] at a retract stop - retract half of the motor test SKIPPED"));
  } else {
    retractMotor(); guardedDelay(500);
    stopMotor();
  }
}

// Live edge monitor for all 4 switches on this axis. Motor stays OFF -- this is
// the "is my switch actually wired?" test: press each one by hand and watch the
// prints. Debounced at 5 ms so contact bounce doesn't spam the console.
void Axis::monitorLimits(unsigned long durationMs) {
  if (_jog != JOG_NONE) jogStop();   // this call blocks, so never leave a jog running

  const char* names[4] = { "travel MIN", "travel MAX", "arm 0 limit", "arm 1 limit" };
  const int   pins[4]  = { _pins.travelMin, _pins.travelMax,
                           _pins.arms[0].limitSwitch, _pins.arms[1].limitSwitch };

  Serial.print("["); Serial.print(_name);
  Serial.println(F("] LIMIT MONITOR - motor is OFF. Press each switch by hand."));
  for (int i = 0; i < 4; i++) {
    Serial.print(F("    ")); Serial.print(names[i]);
    Serial.print(F(" = GPIO ")); Serial.print(pins[i]);
    Serial.print(F("  now: "));
    Serial.println(pins[i] < 0 ? "(not wired)"
                               : (digitalRead(pins[i]) == LOW ? "PRESSED" : "open"));
  }
  Serial.println(F("    (any key ends the monitor early)"));

  bool stable[4], candidate[4];
  unsigned long changedAt[4];
  for (int i = 0; i < 4; i++) {
    stable[i]    = (pins[i] >= 0) && digitalRead(pins[i]) == LOW;
    candidate[i] = stable[i];
    changedAt[i] = millis();
  }

  unsigned long t0 = millis();
  while (millis() - t0 < durationMs) {
    guardEndStops();  // motor should be off in here, but the guard is unconditional
    tickServos();     // don't strand a servo mid-ramp for 15 s if 'm' lands during a flip
    if (Serial.available()) { while (Serial.available()) Serial.read(); break; }
    for (int i = 0; i < 4; i++) {
      if (pins[i] < 0) continue;
      bool now = digitalRead(pins[i]) == LOW;
      if (now != candidate[i]) { candidate[i] = now; changedAt[i] = millis(); continue; }
      if (now != stable[i] && millis() - changedAt[i] >= 5) {   // 5 ms debounce
        stable[i] = now;
        Serial.print(F("  t+")); Serial.print(millis() - t0);
        Serial.print(F("ms  ")); Serial.print(names[i]);
        Serial.print(F(" (GPIO ")); Serial.print(pins[i]); Serial.print(F(") -> "));
        Serial.println(now ? "PRESSED" : "released");
      }
    }
    delay(1);
  }
  Serial.print("["); Serial.print(_name); Serial.println(F("] LIMIT MONITOR done"));
}

// ---------------------------------------------------------------------------
// Flip -> retract -> limit-stop, the tail of a real cycle, with the ToF-driven
// extend removed so it works on a bench with no sensors fitted.
//
// Sequence: both servos ramp to State B (up) -> wait for them to ARRIVE (not
// just for a timer, so this stays correct at any slew rate) -> supervised
// retract -> serviceJog() kills the motor on the first arm limit / travel MIN
// and prints PASS. FAIL if jogTimeoutMs expires with no switch.
// ---------------------------------------------------------------------------
void Axis::testFlipRetract() {
  Serial.print("["); Serial.print(_name);
  Serial.println(F("] FLIP+RETRACT TEST"));

  // Check everything BEFORE flipping, so a test that cannot run doesn't leave
  // the arms up.
  if (_state == CLAMPED) {
    Serial.println(F("    -> axis is CLAMPED. Release with 'c' first. Aborted."));
    return;
  }
  if (isRunning()) {
    Serial.println(F("    -> a cycle is running. Press 's' first. Aborted."));
    return;
  }
  if (_pins.travelMin < 0 && _pins.arms[0].limitSwitch < 0) {
    Serial.println(F("    -> no retract-side switch configured. Aborted."));
    return;
  }
  if (travelMinPressed()) {
    Serial.println(F("    -> already at travel MIN. Jog extend ('e'/'E') to back off first. Aborted."));
    return;
  }
  if (anyArmLimitPressed()) {
    Serial.println(F("    -> an arm limit is already pressed (arms are home)."));
    Serial.println(F("       Jog extend ('e'/'E') a little, then run this again. Aborted."));
    return;
  }

  // Stop anything else that was moving, then flip.
  if (_jog != JOG_NONE) jogStop();

  _arms[0].toStateB();
  _arms[1].toStateB();

  // Timeout for the swing, scaled to the current slew rate -- at 5 deg/s the
  // 90 deg swing legitimately takes 18 s, so a fixed cap would false-FAIL.
  const float swingDeg = fabsf((float)(Flipper::ANGLE_STATE_A - Flipper::ANGLE_STATE_B));
  const float dps      = _arms[0].servoSpeed();
  _flipRetractCapMs   = (unsigned long)(swingDeg * 1000.0f / dps) + 3000;
  _flipRetractStart   = millis();
  _flipRetractPending = true;

  Serial.print(F("    step 1: both arms -> B (up) at ")); Serial.print((int)dps);
  Serial.println(F(" deg/s"));
  Serial.println(F("    step 2: RETRACT until a limit switch stops the motor"));
  Serial.println(F("    ('s' = E-STOP if it does not stop itself)"));
}

void Axis::serviceFlipRetract() {
  if (!_flipRetractPending) return;

  if (servosMoving()) {
    if (millis() - _flipRetractStart > _flipRetractCapMs) {
      _flipRetractPending = false;
      Serial.print("["); Serial.print(_name);
      Serial.println(F("] FLIP+RETRACT: FAIL - servos never finished the swing. Retract NOT started."));
    }
    return;
  }

  _flipRetractPending = false;
  Serial.print("["); Serial.print(_name);
  Serial.print(F("] both arms flipped up in ")); Serial.print(millis() - _flipRetractStart);
  Serial.println(F(" ms - retracting now"));

  // verbose=true: serviceJog() prints PASS the moment a switch cuts the motor,
  // or FAIL on jogTimeoutMs. It watches travel MIN and BOTH arm limits.
  startJog(JOG_RETRACT, true);
}

// Drive INTO an end-stop on purpose and prove the switch cuts the motor.
// Arms a supervised jog and returns immediately; serviceJog() reports PASS as
// soon as the switch trips, or FAIL if jogTimeoutMs expires first. 's' still
// e-stops at any point because loop() keeps running.
void Axis::testTravelStop(bool retract) {
  Serial.print("["); Serial.print(_name);
  Serial.print(F("] LIMIT TEST: driving "));
  Serial.print(retract ? "RETRACT toward travel MIN" : "EXTEND toward travel MAX");
  Serial.print(F(", timeout ")); Serial.print(_cfg.limitTestTimeoutMs); Serial.println(F(" ms"));
  Serial.println(F("    ('s' = E-STOP if it does not stop itself)"));

  int   pin     = retract ? _pins.travelMin : _pins.travelMax;
  bool  pressed = retract ? travelMinPressed() : travelMaxPressed();
  Serial.print(F("    switch GPIO ")); Serial.print(pin);
  Serial.print(F(" reads ")); Serial.println(pressed ? "PRESSED" : "open");
  if (pin < 0) {
    Serial.println(F("    -> no end-stop configured for this direction. Aborted."));
    return;
  }
  if (pressed) {
    Serial.println(F("    -> already on the stop; back it off first. Aborted."));
    return;
  }

  startJog(retract ? JOG_RETRACT : JOG_EXTEND, true);
}
