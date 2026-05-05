#pragma once

#include <Arduino.h>
#include <limits.h>
#include <math.h>

// ------------------------------------------------------------------
// Mouse Screen Configuration
// ------------------------------------------------------------------

// Mouse dx/dy scaling factor: 1 mouse pixel -> MOUSE_SCALE screen pixels
constexpr float MOUSE_SCALE = 10.0f;  // screen pixels per mouse pixel
constexpr int32_t WHEEL_TICKS_PER_MOUSE_DX = 1;   // closed-loop: 1 mouse X pixel -> 1 wheel tick

// Screen-to-physical mapping: PIXELS_PER_METER screen pixels = 1 meter
constexpr int32_t PIXELS_PER_METER = 10000;

// Wheel (dz) scaling: 1 wheel tick -> WHEEL_SCALE_DEG degrees
constexpr float WHEEL_SCALE_DEG = 10.0f;

// Soft limit to prevent int32 overflow during normal operation
constexpr int32_t MOUSE_MAX_POS = INT_MAX / 4;

// ------------------------------------------------------------------
// Hardware Parameters (adjust to your actual robot)
// ------------------------------------------------------------------

constexpr float WHEEL_RADIUS_M    = 0.03f;   // 3 cm wheel radius
constexpr float WHEEL_BASE_HALF_M = 0.075f;  // 7.5 cm from chassis center to wheel
constexpr uint16_t ENCODER_PPR    = 16;      // Encoder pulses per revolution
constexpr uint16_t GEAR_RATIO     = 100;     // Gearbox reduction ratio
constexpr uint8_t  ENCODER_XN     = 2;       // Software counting multiplier (2x)

constexpr float TICKS_PER_REV = static_cast<float>(ENCODER_PPR * GEAR_RATIO * ENCODER_XN);
constexpr float METERS_PER_TICK = (2.0f * PI * WHEEL_RADIUS_M) / TICKS_PER_REV;

// ------------------------------------------------------------------
// Outer-loop Position PID Gains (world frame -> body frame velocity)
// ------------------------------------------------------------------

// X/Y position loop: speed [m/s] = KP_POS_XY * error [m]
constexpr float KP_POS_XY = 0.5f;
constexpr float KD_POS_XY = 0.05f;

// Theta position loop: omega [rad/s] = KP_POS_W * error [rad]
constexpr float KP_POS_W  = 2.0f;
constexpr float KD_POS_W  = 0.2f;

// Speed limits
constexpr float MAX_SPEED_MS = 0.1f;   // m/s
constexpr float MAX_OMEGA_RAD = 0.5f;  // rad/s
