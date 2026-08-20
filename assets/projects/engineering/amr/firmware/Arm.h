#pragma once
#include <Arduino.h>
#include "Adafruit_VL53L0X.h"
#include <Adafruit_PWMServoDriver.h>
#include "Flipper.h"

// One arm within an axis: its OWN ToF distance sensor, servo flipper, and limit
// switch. An axis has two of these. They do NOT own a motor -- both arms of an
// axis extend together on a single thread driven by one shared Axis motor.
//
// Each arm's VL53L0X gets a unique I2C address via the XSHUT dance (the caller
// must hold every arm's XSHUT LOW before the first begin(), so only one sensor
// is awake at the default 0x29 at a time).
class Arm {
public:
  struct Pins {
    int     limitSwitch;    // switch to GND, INPUT_PULLUP
    uint8_t servoChannel;   // PCA9685 output channel (0-15) for this arm's servo
    int     xshut;          // VL53L0X XSHUT line (for the unique-address dance)
  };

  // label: short name for Serial prints (e.g. "X0"). Copied internally.
  // pwm: the shared PCA9685 driver (all 4 arms' servos live on one chip).
  // i2cAddress: the UNIQUE address to assign THIS arm's sensor.
  // servoSpeedDegPerSec: flip slew rate (see FLIPPER_DEFAULT_SPEED_DPS).
  bool begin(const char* label, Adafruit_PWMServoDriver* pwm, const Pins& pins,
             uint16_t airThresholdMm, uint8_t i2cAddress,
             float servoSpeedDegPerSec = FLIPPER_DEFAULT_SPEED_DPS);

  // Servo ramping is non-blocking, so SOMETHING must pump it: call tick() every
  // loop or toStateA/B will set a target the servo never travels to.
  void tick() { _flipper.tick(); }

  bool seesAir();        // true if this arm's ToF reads beyond the trolley (air)
  bool limitPressed();   // true if this arm's limit switch is pressed
  void toStateA();       // servo -> home  (arm-extended state), ramped
  void toStateB();       // servo -> flipped, ramped
  bool servoMoving() const { return _flipper.isMoving(); }
  void setServoSpeed(float degPerSec) { _flipper.setSpeed(degPerSec); }
  float servoSpeed() const { return _flipper.speed(); }
  void testServo();      // move this arm A -> B -> A

  // ---- per-arm self-test prints ----
  void testSensor();     // one distance reading + air/solid verdict
  void testLimit();      // limit switch state

  const char* label() const { return _label; }
  void reportFlipper() { _flipper.report(); }   // servo diagnostics

private:
  // Blocking wait used ONLY by testServo(): keeps ticking the ramp so the servo
  // still travels while we sit here. Returns after ms AND once the servo has
  // arrived, whichever is later.
  void settle(unsigned long ms);

  char     _label[12] = "?";
  Pins     _pins;
  uint16_t _airThresholdMm = 300;
  Adafruit_VL53L0X _lox;
  Flipper  _flipper;
  bool     _sensorOk = false;   // false in SENSORLESS_BENCH when no ToF is fitted
};
