#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

// ---- Servo slew rate (degrees per second) -------------------------------
// A hobby servo has NO speed input: you send it an angle and it slews there at
// its own full speed. To move slower we ramp the SETPOINT ourselves -- lots of
// small angle steps instead of one big jump -- which is what tick() does.
//
// So this is the number to turn if the flip looks too violent. The A->B swing
// is 90 deg, so the swing time is roughly 90 / speed seconds:
//     180 deg/s -> 0.50 s      90 deg/s -> 1.00 s      45 deg/s -> 2.00 s
//
// Keep the swing comfortably shorter than Axis::Config::flipSettleMs, or the
// axis will sit waiting on the servo before it retracts (it does wait -- see
// Axis::tick() FLIPPING -- so nothing breaks, the cycle just takes longer).
//
// Namespace scope, not static class members: floats can't have in-class
// initializers under C++11, which is what the ESP32 core builds with.
const float FLIPPER_DEFAULT_SPEED_DPS = 90.0f;   // 90 deg swing in ~1 s
const float FLIPPER_MIN_SPEED_DPS     = 5.0f;    // slower than this is pointless
const float FLIPPER_MAX_SPEED_DPS     = 600.0f;  // ~ a 9g servo's own full speed

class Flipper {
public:
  // pwm: the shared PCA9685 driver (one chip, 16 channels, all 4 servos on it).
  // channel: which PCA9685 output this arm's servo is wired to (0-15).
  // degPerSec: slew rate for every later flip (the park in begin() is instant).
  void begin(Adafruit_PWMServoDriver* pwm, uint8_t channel,
             float degPerSec = FLIPPER_DEFAULT_SPEED_DPS);

  // These no longer move the servo themselves -- they just set the target. The
  // motion happens in tick(), one small step per 50 Hz servo frame.
  void toStateA();   // HOME     - 170 deg (arm down / travelling)
  void toStateB();   // FLIPPED  -  80 deg (arm up)

  // Advance the ramp. Call as fast as possible from loop(); it returns
  // immediately when the servo is already at its target.
  void tick();
  bool isMoving() const { return _currentDeg != _targetDeg; }

  void  setSpeed(float degPerSec);          // clamped to MIN/MAX above
  float speed() const { return _degPerSec; }

  void report();   // print channel + angles/pulses/speed (servo diagnostics)

  // >>> CALIBRATE THESE TWO NUMBERS ONCE, then leave them alone <
  // Positions are in DEGREES so they match what you measure on the rig.
  static const int ANGLE_STATE_A = 170;   // home
  static const int ANGLE_STATE_B = 80;    // flipped up

private:
  void moveTo(int deg);                            // arm a ramp toward deg
  void writeAngle(float deg, bool verbose);
  void writeMicroseconds(uint16_t us, bool verbose);

  // One step per 50 Hz servo frame. Stepping faster than the frame rate buys
  // nothing -- the PCA9685 only emits a new pulse every 20 ms anyway.
  static const int STEP_MS = 20;

  // 0-180 deg maps to 500-2500 us, the same convention rawServoSweep() ('p')
  // uses -- so the raw sweep and the real flip agree on what an angle means.
  static const int SERVO_MIN_US = 500;    // 0 deg
  static const int SERVO_MAX_US = 2500;   // 180 deg
  static uint16_t angleToUs(float deg);

  Adafruit_PWMServoDriver* _pwm = nullptr;
  uint8_t _channel = 0;

  // Float, not int: at 45 deg/s a 20 ms step is only 0.9 deg, so integer
  // rounding would quantise the rate badly (or stall it entirely).
  float _currentDeg = ANGLE_STATE_A;      // where we have commanded it so far
  float _targetDeg  = ANGLE_STATE_A;      // where the ramp is heading
  float _degPerSec  = FLIPPER_DEFAULT_SPEED_DPS;
  unsigned long _lastStepMs = 0;
};
