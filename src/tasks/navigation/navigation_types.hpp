#pragma once

#include "tinyfsm.hpp"

#include <stdint.h>

namespace navigation {

// ------------------------------------------------------------------
// Config
// ------------------------------------------------------------------

struct LineFollowConfig {
    bool end_in_T_junction = true;
    bool end_in_no_line    = true;
    float end_in_cm        = 10.f;
};

struct WallFollowConfig {
    float gap_cm    = 10.f;
    float end_in_cm = 10.f;
};

// ------------------------------------------------------------------
// Context
// ------------------------------------------------------------------

struct NavContext {
    LineFollowConfig line_cfg;
    WallFollowConfig wall_cfg;
};

// ------------------------------------------------------------------
// Events
// ------------------------------------------------------------------

struct IntoLineFollow : tinyfsm::Event {
    LineFollowConfig cfg;
};

struct IntoWallFollow : tinyfsm::Event {
    WallFollowConfig cfg;
};

struct IntoIdle : tinyfsm::Event {};

struct RfidDone : tinyfsm::Event {
    uint32_t tag_id = 0;
    bool fertile    = false;
    RfidDone(uint32_t id = 0, bool f = false) : tag_id(id), fertile(f) {}
};

} // namespace navigation
