#include "Flipper.h"
#include "Config.h"

void Flipper::begin(Adafruit_PWMServoDriver* pwm, uint8_t channel, float degPerSec) {
  _pwm = pwm;
  _channel = channel;
  setSpeed(degPerSec);
  Serial.print("[Flipper] servo on PCA9685 ch="); Serial.println(_channel);

  // Park at HOME immediately (no ramp): at boot we have no idea where the horn
  // actually is, so there is nothing to ramp FROM. Every later move ramps.
  _currentDeg = _targetDeg = ANGLE_STATE_A;
  _lastStepMs = millis();
  writeAngle(ANGLE_STATE_A, true);
  Serial.print("[Flipper] parked at home "); Serial.print(ANGLE_STATE_A);
  Serial.print(F(" deg, slew ")); Serial.print((int)_degPerSec);
  Serial.println(F(" deg/s"));
}

void Flipper::setSpeed(float degPerSec) {
  if (degPerSec < FLIPPER_MIN_SPEED_DPS) degPerSec = FLIPPER_MIN_SPEED_DPS;
  if (degPerSec > FLIPPER_MAX_SPEED_DPS) degPerSec = FLIPPER_MAX_SPEED_DPS;
  _degPerSec = degPerSec;
}

// --- diagnostics ---
void Flipper::report() {
  Serial.print("  [Flipper] ch="); Serial.print(_channel);
  Serial.print(" home A="); Serial.print(ANGLE_STATE_A);
  Serial.print("deg/"); Serial.print(angleToUs(ANGLE_STATE_A));
  Serial.print("us  flip B="); Serial.print(ANGLE_STATE_B);
  Serial.print("deg/"); Serial.print(angleToUs(ANGLE_STATE_B));
  Serial.print("us  slew="); Serial.print((int)_degPerSec);
  Serial.print("deg/s (swing ");
  Serial.print((unsigned long)(fabsf((float)(ANGLE_STATE_A - ANGLE_STATE_B)) * 1000.0f / _degPerSec));
  Serial.println("ms)");
}

void Flipper::toStateA() {
  moveTo(ANGLE_STATE_A);
}

void Flipper::toStateB() {
  moveTo(ANGLE_STATE_B);
}

// Arms a ramp. Returns immediately -- tick() does the work.
void Flipper::moveTo(int deg) {
  _targetDeg  = deg;
  _lastStepMs = millis();      // first step lands one full frame from now
  Serial.print("  [Flipper] ch="); Serial.print(_channel);
  Serial.print(" ramp "); Serial.print((int)_currentDeg);
  Serial.print(" -> "); Serial.print(deg);
  Serial.print(F(" deg @ ")); Serial.print((int)_degPerSec);
  Serial.print(F(" deg/s (~"));
  Serial.print((unsigned long)(fabsf(_targetDeg - _currentDeg) * 1000.0f / _degPerSec));
  Serial.println(F(" ms)"));
}

void Flipper::tick() {
  if (!isMoving()) return;

  unsigned long now     = millis();
  unsigned long elapsed = now - _lastStepMs;
  if (elapsed < (unsigned long)STEP_MS) return;
  _lastStepMs = now;

  // Step by the time that ACTUALLY passed, not by a fixed increment. The loop
  // can stretch (a VL53L0X ranging read is tens of ms), and this keeps the
  // swing taking the same wall-clock time either way.
  float step = _degPerSec * (float)elapsed / 1000.0f;

  if (_currentDeg < _targetDeg) {
    _currentDeg += step;
    if (_currentDeg > _targetDeg) _currentDeg = _targetDeg;   // exact -> isMoving() clears
  } else {
    _currentDeg -= step;
    if (_currentDeg < _targetDeg) _currentDeg = _targetDeg;
  }

  writeAngle(_currentDeg, false);     // quiet: ~50 steps/s would drown the console

  if (!isMoving()) {
    Serial.print("  [Flipper] ch="); Serial.print(_channel);
    Serial.print(F(" arrived at ")); Serial.print((int)_targetDeg);
    Serial.print(F(" deg (")); Serial.print(angleToUs(_targetDeg));
    Serial.println(F("us)"));
  }
}

uint16_t Flipper::angleToUs(float deg) {
  if (deg < 0)   deg = 0;
  if (deg > 180) deg = 180;
  return (uint16_t)(SERVO_MIN_US + deg * (float)(SERVO_MAX_US - SERVO_MIN_US) / 180.0f);
}

void Flipper::writeAngle(float deg, bool verbose) {
  if (verbose) {
    Serial.print("  [Flipper] ch="); Serial.print(_channel);
    Serial.print(" -> "); Serial.print((int)deg); Serial.print(F(" deg"));
  }
  writeMicroseconds(angleToUs(deg), verbose);
}

void Flipper::writeMicroseconds(uint16_t us, bool verbose) {
  if (!_pwm) return;
  const uint32_t periodUs = 1000000UL / SERVO_FREQ_HZ;      // 20000us @ 50Hz
  uint16_t ticks = (uint32_t)us * 4096UL / periodUs;         // PCA9685 is 12-bit (0-4095)
  uint8_t rc = _pwm->setPWM(_channel, 0, ticks);
  if (verbose) {
    Serial.print(" ("); Serial.print(us); Serial.print("us, ticks=");
    Serial.print(ticks); Serial.print(") setPWM=");
    Serial.println(rc == 0 ? "OK" : "I2C WRITE FAILED");
  } else if (rc != 0) {
    // Ramp steps are silent, but never swallow a dead bus.
    Serial.print("  [Flipper] ch="); Serial.print(_channel);
    Serial.println(F(" I2C WRITE FAILED mid-ramp"));
  }
}
