#pragma once

#include <Arduino.h>

#include <array>
#include <queue>

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

std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;
