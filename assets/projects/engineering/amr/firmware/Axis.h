#pragma once
#include <Arduino.h>
#include "Arm.h"
#include "Motor.h"

// One arm axis = TWO arms sharing ONE motor + thread.
//   - Each arm has its own ToF sensor, servo flipper, and retract-home limit (class Arm).
//   - One BTS7960 motor extends/retracts BOTH arms together on the shared thread.
//   - Each axis also has TWO mechanical travel hard-stops (travelMin / travelMax):
//       * travelMax (the "extended" end) ENDS the extend and triggers the flip
//       * travelMin (the "retracted" end) is a safety stop during RETRACTING
//     These are belt-and-braces on top of the ToF-"air" logic and the per-arm
//     retract-home limits, so a failed sensor can never drive a motor to a stop.
//
// The state machine:
//   EXTENDING -> PRE_FLIP : BOTH arms' ToF see air (AND)  OR  travelMax trips.
//                           Either way the motor stops and the arms flip: air
//                           = boundary found, travelMax = end of travel, flip
//                           at max range. Only a timeout is a FAULT here.
//   RETRACTING -> STOPPED : EITHER arm's limit is hit (OR), OR travelMin trips (halt)
//
// Non-blocking: call tick() as fast as possible from loop(). Each Axis is a
// self-contained, independent object, so axis X and axis Y tick concurrently.
class Axis {
public:
  // The cycle now ends HOLDING the trolley, not finished:
  //   ... RETRACTING -> CLAMPED        (arms hooked, motor off, trolley held)
  // CLAMPED persists until release() is called -- that is the window in which
  // the robot drives to the target location. Then:
  //   UNLOADING -> RELEASING -> STOWING -> STOPPED
  //     UNLOADING : brief extend to take the trolley's weight off the hooks,
  //                 so the servos aren't asked to rotate down under load
  //     RELEASING : flippers back down to State A (the actual let-go)
  //     STOWING   : retract all the way to travel MIN, past the arm limits
  enum State { IDLE, HOMING, EXTENDING, PRE_FLIP, FLIPPING, RETRACTING,
               CLAMPED, UNLOADING, RELEASING, STOWING, STOPPED, FAULT };

  static const uint8_t NUM_ARMS = 2;

  // Wiring for the whole axis: two arms + one motor + two travel stops.
  struct Pins {
    Arm::Pins   arms[NUM_ARMS];   // arm 0, arm 1 (each: limitSwitch, servo, xshut)
    Motor::Pins motor;            // shared-thread motor. -1 pins = SIM mode.
    int travelMin = -1;           // axis retracted end-stop switch -> GND, INPUT_PULLUP
    int travelMax = -1;           // axis extended  end-stop switch -> GND, INPUT_PULLUP
  };

  // Per-axis tuning. Both arms of an axis share these.
  struct Config {
    uint16_t      airThresholdMm    = 300;
    uint8_t       airConfirmCount   = 3;      // consecutive POLLS with BOTH arms in air
    // How often to range the ToF sensors while extending. A VL53L0X read BLOCKS
    // for ~30-50 ms, and nothing can watch the end-stops while it does. Ranging
    // every tick left the travel switch unsampled for ~100 ms at a time, long
    // enough for the carriage to drive through it. Polling on a timer instead
    // lets the loop spin free (and watch the switch) between reads.
    unsigned long airPollMs         = 50;
    uint8_t       motorDuty         = 200;    // 0-255 PWM drive strength (~78%)
    // Servo flip slew rate. THIS is the knob for "the arms whip round too fast".
    // Lower = gentler. See the notes in Flipper.h.
    float         servoSpeedDegPerSec = FLIPPER_DEFAULT_SPEED_DPS;
    unsigned long flipSettleMs      = 2000;   // servo swing / settle time around the flip
    unsigned long homeSettleMs      = 2000;   // settle after homing, before extend
    // Release phase. unloadMs is a short EXTEND that takes the trolley's weight
    // off the hooks before the flippers rotate down -- without it the servos
    // stall against the load. Tune it up if the flip-down still binds.
    unsigned long unloadMs          = 400;
    unsigned long stowTimeoutMs     = 8000;   // retract-to-travelMIN guard: 0 = disabled
    unsigned long extendTimeoutMs   = 8000;   // safety: 0 = disabled
    unsigned long retractTimeoutMs  = 8000;   // safety: 0 = disabled
    // Manual-jog runaway guard. Deliberately SHORT: a jog is a human-supervised
    // nudge, and an extend jog watches only travelMax -- so if that switch
    // cannot trip, this timer is the ONLY thing that stops the motor. At 10 s
    // and duty 200 that was long enough to drive a clamped axis into damage.
    unsigned long jogTimeoutMs      = 3000;   // 0 = disabled
    // The t/T/g/G limit tests intentionally drive a full travel, so they need
    // longer than a hand jog. Keep this above your measured travel time.
    unsigned long limitTestTimeoutMs = 10000;
  };

  // pwm: the shared PCA9685 driver (all 4 servos across both axes live on one chip).
  // addrArm0 / addrArm1: the UNIQUE I2C addresses to assign each arm's sensor.
  bool begin(const char* name, Adafruit_PWMServoDriver* pwm, const Pins& pins, const Config& cfg,
             uint8_t addrArm0, uint8_t addrArm1);

  void tick();          // advance the state machine + both servo ramps (non-blocking)
  void start();         // begin a fresh cycle (refused while CLAMPED -- release first)
  void release();       // CLAMPED -> unload, flip down, stow. No-op in any other state.
  void estop();         // immediately stop the motor and halt the state machine

  // Change the servo flip speed on both arms at runtime (bench tuning).
  void  setServoSpeed(float degPerSec);
  float servoSpeed() const { return _arms[0].servoSpeed(); }

  // CLAMPED counts as NOT running: the clamp cycle is finished and the axis is
  // just holding. The sequencer uses this to know axis X is done and axis Y may
  // start. UNLOADING/RELEASING/STOWING DO count as running.
  bool  isRunning() const {
    return _state != IDLE && _state != STOPPED && _state != FAULT && _state != CLAMPED;
  }
  bool  isClamped() const { return _state == CLAMPED; }
  bool  isDone()    const { return _state == STOPPED; }
  State state()     const { return _state; }
  const char* name()      const { return _name; }
  const char* stateName() const;

  // ---- Block-level self-tests ----
  void testSensor();    // print both arms' distance readings + air/solid verdicts
  void testLimit();     // print both arms' retract-home limit states
  void testTravel();    // print both travel hard-stops (min/max)
  void testServo();     // flip both arms A -> B -> A together
  void testServoArm(uint8_t arm); // test only arm 0 or arm 1
  void testMotor();     // extend -> stop -> retract -> stop (the shared motor)

  // Live switch monitor: prints every edge on all 4 switches of this axis
  // (travel min/max + both arms' retract-home limits) so you can press them by
  // hand and confirm the wiring with NO motor running. Blocking; any serial
  // key ends it early.
  void monitorLimits(unsigned long durationMs = 15000);

  // THE flip-then-retract chain test: flips BOTH arms up, waits for the servos
  // to actually finish swinging, then retracts and proves the retract-home
  // limit switch cuts the motor. This is the real tail of a cycle
  // (PRE_FLIP -> FLIPPING -> RETRACTING -> STOPPED) with the ToF-driven extend
  // taken out, so it runs on a bench with no sensors fitted.
  //
  // Non-blocking: it arms the sequence and returns. tick() drives it and prints
  // PASS as soon as a limit trips, or FAIL on jogTimeoutMs. 's' e-stops it.
  void testFlipRetract();

  // Supervised drive INTO an end-stop: runs the motor toward travelMin
  // (retract=true) or travelMax (retract=false) and lets tick() stop it the
  // instant the switch trips. Prints PASS + the travel time, or FAIL on
  // timeout. Non-blocking: it just arms the jog and returns.
  void testTravelStop(bool retract);

  // ---- Manual motor jog (bench use; bypasses the state machine) ----
  // Jogs are still supervised by tick(): they stop themselves on the end-stop
  // for the direction they are travelling, on any arm limit when retracting,
  // and on jogTimeoutMs so a jog can never run away unattended.
  void jogExtend();
  void jogRetract();
  void jogStop();
  bool isJogging() const { return _jog != JOG_NONE; }

private:
  enum Jog { JOG_NONE, JOG_EXTEND, JOG_RETRACT };

  // ---- THE end-stop guard ----------------------------------------------
  // Runs FIRST in every tick(), and inside every blocking loop in this class,
  // regardless of what started the motor -- state machine, manual jog, or a
  // self-test. It looks only at which way the motor is actually being driven
  // and whether the stop for THAT direction is made, then cuts the motor.
  //
  // Invariant it enforces:
  //   travel MAX is absolute while extending  -- no state is exempt
  //   travel MIN is absolute while retracting -- no state is exempt
  //   arm limits stop a retract too, EXCEPT in STOWING, which travels past
  //   them by design (they mark the clamp position, not the end of the axis)
  //
  // It only stops the motor. State transitions stay in tick(), which sees the
  // same switch on the same pass and moves the machine on.
  void guardEndStops();
  // delay() replacement for the blocking self-tests: keeps the guard and the
  // servo ramps running instead of driving blind.
  void guardedDelay(unsigned long ms);
  // Shared refusal for the BLOCKING self-tests. They stall loop() for seconds,
  // so they must never run while a cycle is live or the trolley is clamped.
  // Also clears any manual jog, so nothing is left driving underneath them.
  bool blockingTestRefused(const char* what);

  void enterState(State s);
  // Checks travelMax; if it is made, stops the motor and goes to PRE_FLIP.
  // Returns true if it fired. Called REPEATEDLY inside EXTENDING, including
  // between the two blocking ToF reads, so the stop is never more than one
  // ranging call away.
  bool travelMaxStopAndFlip();
  bool servosMoving() const;       // either arm's servo still mid-ramp
  void tickServos();               // pump both arms' ramps
  void serviceFlipRetract();       // testFlipRetract(): fire the retract once the flip lands
  void settleServos(unsigned long ms);  // blocking wait that keeps ramps running
  void startJog(Jog dir, bool verbose);
  void serviceJog();               // end-stop supervision for manual jogs
  bool anyArmLimitPressed();       // either arm's retract-home limit
  void extendMotor();
  void retractMotor();
  void stopMotor();
  bool travelMinPressed() const;   // axis retracted end-stop (true = at/past min)
  bool travelMaxPressed() const;   // axis extended  end-stop (true = at/past max)

  const char* _name = "?";
  Pins   _pins;
  Config _cfg;
  Arm    _arms[NUM_ARMS];
  Motor  _motor;
  State _state = IDLE;
  unsigned long _stateStart = 0;
  uint8_t _airCount = 0;
  unsigned long _lastAirPollMs = 0;   // throttles the blocking ToF reads

  Jog           _jog        = JOG_NONE;   // manual jog currently running
  unsigned long _jogStart   = 0;
  bool          _jogVerbose = false;      // true = testTravelStop(), print PASS/FAIL

  // testFlipRetract(): waiting on the servo swing before the retract starts
  bool          _flipRetractPending = false;
  unsigned long _flipRetractStart   = 0;
  unsigned long _flipRetractCapMs   = 0;  // swing timeout, scaled to the servo speed
};
