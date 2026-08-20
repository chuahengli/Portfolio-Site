#include "Motor.h"

void Motor::begin(const Pins& pins, const char* label, uint8_t duty) {
  _pins  = pins;
  _label = label;
  _duty  = duty;

  if (!wired()) {
    Serial.print(">> ["); Serial.print(_label);
    Serial.println("] motor: no PWM pins set -> SIM mode (prints only)");
    return;
  }

  // Enable both half-bridges. If an EN pin is -1 we assume it's tied HIGH in
  // hardware (a common way to save two GPIOs on the BTS7960).
  if (_pins.rEn >= 0) { pinMode(_pins.rEn, OUTPUT); digitalWrite(_pins.rEn, HIGH); }
  if (_pins.lEn >= 0) { pinMode(_pins.lEn, OUTPUT); digitalWrite(_pins.lEn, HIGH); }

  // Writing 0 attaches each pin to a free LEDC channel and parks it stopped.
  analogWrite(_pins.rpwm, 0);
  analogWrite(_pins.lpwm, 0);

  Serial.print(">> ["); Serial.print(_label);
  Serial.print("] motor ready (RPWM="); Serial.print(_pins.rpwm);
  Serial.print(" LPWM="); Serial.print(_pins.lpwm);
  Serial.print(" duty="); Serial.print(_duty);
  if (_pins.invert) {
    Serial.print(" INVERTED: extend drives LPWM="); Serial.print(_pins.lpwm);
  }
  Serial.println(")");
}

void Motor::extend() {
  Serial.print(">> ["); Serial.print(_label); Serial.println("] motor: EXTEND");
  _dir = DIR_EXTEND;                 // tracked even in SIM mode
  if (!wired()) return;
  analogWrite(retractPin(), 0);      // kill the other side FIRST (no shoot-through)
  analogWrite(extendPin(), _duty);
}

void Motor::retract() {
  Serial.print(">> ["); Serial.print(_label); Serial.println("] motor: RETRACT");
  _dir = DIR_RETRACT;
  if (!wired()) return;
  analogWrite(extendPin(), 0);       // kill the other side FIRST
  analogWrite(retractPin(), _duty);
}

// Safe to call as often as you like -- the end-stop guard does, every tick.
// Already-stopped is a silent no-op so the console isn't flooded.
void Motor::stop() {
  if (_dir == DIR_NONE) return;
  Serial.print(">> ["); Serial.print(_label); Serial.println("] motor: STOP");
  _dir = DIR_NONE;
  if (!wired()) return;
  analogWrite(_pins.rpwm, 0);
  analogWrite(_pins.lpwm, 0);
}
