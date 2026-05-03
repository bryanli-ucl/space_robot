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

constexpr int BOARD_LED_R = D86;
constexpr int BOARD_LED_G = D87;
constexpr int BOARD_LED_B = D88;

constexpr int GRID_WIDTH  = 9;
constexpr int GRID_HRIGHT = 9;

extern std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;

// ------------------------------------------------------------------
// Fleet WiFi Client Configuration
// ------------------------------------------------------------------
constexpr int ROBOT_ID = 12;              // Robot ID in fleet (1~14)
constexpr char PC_IP[] = "192.168.1.120"; // PC static IP (CHANGE THIS)
constexpr int PC_PORT  = 8080;            // PC Server port
