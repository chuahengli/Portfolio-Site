#include "Arm.h"

bool Arm::begin(const char* label, Adafruit_PWMServoDriver* pwm, const Pins& pins,
                uint16_t airThresholdMm, uint8_t i2cAddress,
                float servoSpeedDegPerSec) {
  strncpy(_label, label, sizeof(_label) - 1);
  _label[sizeof(_label) - 1] = '\0';
  _pins = pins;
  _airThresholdMm = airThresholdMm;

  pinMode(_pins.limitSwitch, INPUT_PULLUP);
  // servo parks in State A (instantly); later flips ramp at this speed
  _flipper.begin(pwm, _pins.servoChannel, servoSpeedDegPerSec);

  // Wake THIS sensor (still at default 0x29) and reassign it to i2cAddress.
  digitalWrite(_pins.xshut, HIGH);
  delay(50);                       // t_boot for the VL53L0X
  if (!_lox.begin(i2cAddress)) {
    Serial.print("Arm "); Serial.print(_label);
    Serial.println(F(": VL53L0X failed to boot! Check wiring / XSHUT."));
#ifdef SENSORLESS_BENCH
    Serial.print("Arm "); Serial.print(_label);
    Serial.println(F("  -> SENSORLESS_BENCH: continuing WITHOUT this sensor (servo/limit/motor test only)."));
    return true;      // bench mode: keep arm usable without a ToF
#else
    return false;
#endif
  }
  _sensorOk = true;
  Serial.print("Arm "); Serial.print(_label);
  Serial.print(F(" ready at I2C 0x")); Serial.println(i2cAddress, HEX);
  return true;
}

bool Arm::seesAir() {
  if (!_sensorOk) return false;              // no ToF: treat as SOLID, never auto-advance
  VL53L0X_RangingMeasurementData_t m;
  _lox.rangingTest(&m, false);
  if (m.RangeStatus == 4) return true;                 // out of range -> air
  return (m.RangeMilliMeter > _airThresholdMm);
}

bool Arm::limitPressed() { return digitalRead(_pins.limitSwitch) == LOW; }

void Arm::toStateA() { _flipper.toStateA(); }
void Arm::toStateB() { _flipper.toStateB(); }

// Blocking, but it pumps the ramp: a plain delay() here would freeze the servo
// mid-swing, since the motion now lives in Flipper::tick().
void Arm::settle(unsigned long ms) {
  const unsigned long t0  = millis();
  const unsigned long cap = ms + 5000;        // never wait forever
  while (millis() - t0 < cap) {
    _flipper.tick();
    if (millis() - t0 >= ms && !_flipper.isMoving()) return;
    delay(1);
  }
}

void Arm::testServo() {
  // The Flipper prints the exact angle/pulse it writes, so don't restate them here.
  Serial.print("["); Serial.print(_label); Serial.println(F("] servo -> A (home)"));
  _flipper.toStateA();
  settle(1000);
  Serial.print("["); Serial.print(_label); Serial.println(F("] servo -> B (flipped up)"));
  _flipper.toStateB();
  settle(1000);
  Serial.print("["); Serial.print(_label); Serial.println(F("] servo -> A (home)"));
  _flipper.toStateA();
  settle(0);          // no extra dwell, but do let it finish travelling
}

void Arm::testSensor() {
  if (!_sensorOk) {
    Serial.print("["); Serial.print(_label); Serial.println(F("] no ToF fitted (SENSORLESS_BENCH)"));
    return;
  }
  VL53L0X_RangingMeasurementData_t m;
  _lox.rangingTest(&m, false);
  Serial.print("["); Serial.print(_label); Serial.print("] ");
  if (m.RangeStatus == 4) { Serial.println("out of range -> AIR"); return; }
  Serial.print("dist="); Serial.print(m.RangeMilliMeter);
  Serial.print("mm  -> "); Serial.println(seesAir() ? "AIR" : "solid");
}

void Arm::testLimit() {
  Serial.print("["); Serial.print(_label); Serial.print("] limit = ");
  Serial.println(limitPressed() ? "PRESSED" : "open");
}
