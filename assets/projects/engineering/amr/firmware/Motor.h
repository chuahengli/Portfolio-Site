#pragma once
#include <Arduino.h>

// One brushed DC motor on a BTS7960 / IBT-2 style H-bridge.
//
// Direction convention (matches the Axis state machine):
//   extend()  -> RPWM = PWM, LPWM = 0   (drives one way)
//   retract() -> LPWM = PWM, RPWM = 0   (drives the other way)
//   stop()    -> both = 0               (coast)
// Only ONE of RPWM/LPWM is ever non-zero. Never both -> no shoot-through.
//
// ...unless `invert` is set, which swaps those two roles. Use it when the motor
// is wired or geared so that RPWM physically RETRACTS the axis. Inverting here
// keeps Config.h honest about which GPIO lands on which driver pad (that file
// is the netlist ground truth) and keeps "extend" meaning extend everywhere
// else -- the state machine, the end-stops and the jog supervisor all rely on
// travelMax being the end that extend() drives toward.
//
// This is fully self-contained so you can drop a second Motor onto axis Y later
// just by filling in another set of pins. If the pins are left at -1 the motor
// runs in SIM mode (Serial prints only), so an axis with no motor wired yet
// still compiles and runs exactly like before.
//
// PWM note: uses analogWrite(), which on the ESP32 core is backed by the LEDC
// peripheral (0-255 duty, like a classic Arduino). The 4 servos are driven off
// a separate PCA9685 over I2C, not LEDC, so these motor pins have the ESP32's
// LEDC channels/timers entirely to themselves -- no contention. If the motor
// whines audibly you can raise the PWM frequency in the sketch; ~1 kHz (the
// default) is fine for bring-up.
class Motor {
public:
  struct Pins {
    int rpwm = -1;   // RPWM: "extend" direction PWM. -1 = not wired (SIM mode)
    int lpwm = -1;   // LPWM: "retract" direction PWM
    int rEn  = -1;   // R_EN enable. -1 = tied HIGH in hardware / not driven by MCU
    int lEn  = -1;   // L_EN enable. -1 = tied HIGH in hardware / not driven by MCU
    bool invert = false;  // true = RPWM retracts and LPWM extends (see note above)
  };

  // Which way the motor is being driven RIGHT NOW. The axis end-stop guard
  // needs this: which switch is dangerous depends entirely on the direction of
  // travel, and the guard has to work without knowing which bit of code (state
  // machine, jog, self-test) started the motor.
  enum Dir { DIR_NONE, DIR_EXTEND, DIR_RETRACT };

  // label : used only for Serial prints (pass the axis name).
  // duty  : 0-255 default drive strength.
  void begin(const Pins& pins, const char* label = "motor", uint8_t duty = 200);

  void extend();                 // drive the "extend" direction at the set duty
  void retract();                // drive the "retract" direction at the set duty
  void stop();                   // stop (no-op, and silent, if already stopped)

  Dir  dir() const { return _dir; }
  bool moving() const { return _dir != DIR_NONE; }

  void    setDuty(uint8_t d) { _duty = d; }
  uint8_t duty() const       { return _duty; }
  bool    wired() const      { return _pins.rpwm >= 0 && _pins.lpwm >= 0; }

private:
  // Which physical pin actually drives each direction, after `invert`.
  int extendPin()  const { return _pins.invert ? _pins.lpwm : _pins.rpwm; }
  int retractPin() const { return _pins.invert ? _pins.rpwm : _pins.lpwm; }

  Pins        _pins;
  const char* _label = "motor";
  uint8_t     _duty  = 200;
  Dir         _dir   = DIR_NONE;
};
