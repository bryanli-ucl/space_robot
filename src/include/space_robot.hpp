#pragma once

#include <Arduino.h>

#include <array>
#include <queue>

namespace i2c_addr {

constexpr uint8_t Motoron1 = 0x20;
constexpr uint8_t Motoron2 = 0x21;
constexpr uint8_t RFID     = 0x28;

}; // namespace i2c_addr

enum class grid_slot_t : uint8_t {
    Unknow,
    Fertile,
    Infertile,
};

// ------------------------------------------------------------------
// Pin definitions — single source of truth for all wiring
// ------------------------------------------------------------------
namespace pins {

// RGB LED (active-low on Giga R1)
constexpr int LED_R = D86;
constexpr int LED_G = D87;
constexpr int LED_B = D88;

// Ultrasonic sensors
constexpr int US_FRONT_TRIG = D44;
constexpr int US_FRONT_ECHO = D45;
constexpr int US_LEFT_TRIG  = D46;
constexpr int US_LEFT_ECHO  = D47;
constexpr int US_RIGHT_TRIG = D48;
constexpr int US_RIGHT_ECHO = D49;

// L298N #1 (front motors)
constexpr int L298N_FRONT_EN_A = D8;
constexpr int L298N_FRONT_IN1  = D25;
constexpr int L298N_FRONT_IN2  = D27;
constexpr int L298N_FRONT_IN3  = D29;
constexpr int L298N_FRONT_IN4  = D31;
constexpr int L298N_FRONT_EN_B = D9;

// L298N #2 (rear motors)
constexpr int L298N_REAR_EN_A = D10;
constexpr int L298N_REAR_IN1  = D24;
constexpr int L298N_REAR_IN2  = D26;
constexpr int L298N_REAR_IN3  = D28;
constexpr int L298N_REAR_IN4  = D30;
constexpr int L298N_REAR_EN_B = D11;

// L298N #3 (5th motor)
constexpr int L298N_EXTRA_EN_A = D12;
constexpr int L298N_EXTRA_IN1  = D50;
constexpr int L298N_EXTRA_IN2  = D51;
constexpr int L298N_EXTRA_IN3  = D52;
constexpr int L298N_EXTRA_IN4  = D53;
constexpr int L298N_EXTRA_EN_B = D13;

// Encoders (A = interrupt pin, B = direction pin)
constexpr int ENC_FL_A = D32;
constexpr int ENC_FL_B = D33;
constexpr int ENC_FR_A = D34;
constexpr int ENC_FR_B = D35;
constexpr int ENC_RL_A = D36;
constexpr int ENC_RL_B = D37;
constexpr int ENC_RR_A = D38;
constexpr int ENC_RR_B = D39;
constexpr int ENC_EX_A = D42;
constexpr int ENC_EX_B = D43;

} // namespace pins

// Backwards-compatible aliases (deprecated, use pins::)
constexpr int BOARD_LED_R = pins::LED_R;
constexpr int BOARD_LED_G = pins::LED_G;
constexpr int BOARD_LED_B = pins::LED_B;

constexpr int GRID_WIDTH  = 9;
constexpr int GRID_HRIGHT = 9;

extern std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;

// ------------------------------------------------------------------
// Fleet WiFi Client Configuration
// ------------------------------------------------------------------
constexpr int ROBOT_ID = 12;              // Robot ID in fleet (1~14)
constexpr char PC_IP[] = "192.168.0.211"; // PC static IP (CHANGE THIS)
constexpr int PC_PORT  = 8080;            // PC Server port
