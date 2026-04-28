#pragma once

#include "tinyfsm.hpp"

#include "space_robot.hpp"

namespace fsm = tinyfsm;

struct Event_WifiData : fsm::Event {
    enum class event_t {
        Enmergency_Back,
    } value;
    Event_WifiData(event_t v) : value(v) {}
};