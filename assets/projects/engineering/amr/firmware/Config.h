#pragma once
#include <Arduino.h>

// ============================================================================
// Arm Subsystem PCB  -  VERIFIED PINOUT
// Source: "Arm Subsystem PCB.net" (exported from KiCad, Aug 2026)
// ESP32-S3-DevKitC  |  2 axes (X, Y) x 2 arms  |  dual external BTS7960 drivers
//
// Signal net names come from the schematic; the GPIO numbers are the
// footprint-pad pinfunctions from the KiCad netlist (the ground truth).
// ============================================================================

// ---- I2C bus (shared by all 4 ToF sensors) ----
const int SDA_PIN = 8;
const int SCL_PIN = 9;

// ---- Clamp signal out to the main robot controller ----
// HIGH = both axes are clamped and holding the trolley, safe to drive to the
// target. LOW = not clamped (idle, mid-cycle, or released).
//
// GPIO 10 is one of the four pins freed when the servos moved to the PCA9685
// (see the note below) -- it is the PCB's "Servo X1" header, which this
// firmware no longer drives. 47, 4 and 14 are free the same way if you need
// more signals later.
const int CLAMP_SIGNAL_PIN = 10;

// ---- PCA9685 servo driver (shared by all 4 servos, over the I2C bus above) ----
// GPIO servo control was dropped in favor of a PCA9685 so the 4 servos no
// longer fight the BTS7960 motor PWM for the ESP32's LEDC channels/timers.
//
// CONSEQUENCE FOR WIRING: the PCB's own servo headers S1..S4 (netlist nets
// "Servo X1/X2/Y1/Y2" -> GPIO 10 / 47 / 4 / 14) are NO LONGER USED by this
// firmware. Those 4 GPIOs are left free; plug the servos into PCA9685 outputs
// 0..3 instead (channel map below), and feed the PCA9685 V+ from the 5V rail
// with its GND common to the board.
//
// CHANNEL MAP IS SWAPPED vs. the obvious order, on purpose: on the rig the Y
// arms' servos are plugged into PCA9685 ch 0/1 and the X arms' into ch 2/3.
// Before this swap, '4' (axis X servo test) moved the Y servos and '9' moved
// the X servos. Fixed here in the channel map, NOT by renaming the tests, so
// the automatic cycle flips the right pair of arms too.
//   PCA9685 ch 0 -> Y arm 0      PCA9685 ch 2 -> X arm 0
//   PCA9685 ch 1 -> Y arm 1      PCA9685 ch 3 -> X arm 1
const uint8_t PCA9685_ADDR = 0x40;
const int     SERVO_FREQ_HZ = 50;

// ==============================  AXIS X  ===================================
// Arm 0  = ToF X1
const int LIMIT_X0 = 11;    // arm retract-home limit  -> GND, INPUT_PULLUP
const uint8_t SERVO_CH_X0 = 2;  // PCA9685 output channel (X is on ch 2/3, see note above)
const int XSHUT_X0 = 18;    // VL53L0X #1 XSHUT
// Arm 1  = ToF X2
const int LIMIT_X1 = 48;
const uint8_t SERVO_CH_X1 = 3;
const int XSHUT_X1 = 21;
// Axis X travel hard-stops (mechanical end switches on the motor thread)
const int X_TRAVEL_MIN = 2;     // "X Arm Min" - retracted end
const int X_TRAVEL_MAX = 42;    // "X Arm Max" - extended  end
// Axis X motor: external BTS7960 via J1 (pin1=+5V pin2=GND p3=L_EN p4=R_EN p5=LPWM p6=RPWM)
// NOTE: on the rig this motor runs BACKWARDS -- RPWM physically retracts. The
// pin numbers below stay faithful to the netlist; the reversal is corrected by
// `pinsX.motor.invert = true` in main.cpp. Do not "fix" it by swapping these two.
const int RPWM_X = 17;
const int LPWM_X = 16;
const int R_EN_X = 15;   // driver enable (right)
const int L_EN_X = 7;    // driver enable (left)

// ==============================  AXIS Y  ===================================
// Arm 0  = ToF Y1
const int LIMIT_Y0 = 5;
const uint8_t SERVO_CH_Y0 = 0;  // Y is on ch 0/1, see note above
const int XSHUT_Y0 = 6;
// Arm 1  = ToF Y2
const int LIMIT_Y1 = 13;
const uint8_t SERVO_CH_Y1 = 1;
const int XSHUT_Y1 = 12;
// Axis Y travel hard-stops
const int Y_TRAVEL_MIN = 3;     // "Y Arm Min" - retracted end
const int Y_TRAVEL_MAX = 1;     // "Y Arm Max" - extended  end
// Axis Y motor: external BTS7960 via J2
// Unlike X, this one is wired the right way round: RPWM extends. No invert.
const int RPWM_Y = 38;   // extend  direction
const int LPWM_Y = 39;   // retract direction
const int R_EN_Y = 40;
const int L_EN_Y = 41;

// ---- Unique I2C addresses assigned in setup() via the XSHUT dance ----
//   axis X, arm 0 -> 0x30     axis X, arm 1 -> 0x31
//   axis Y, arm 0 -> 0x32     axis Y, arm 1 -> 0x33
// All sensors boot at 0x29; we rename them one at a time so they share the bus.
