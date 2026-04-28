#pragma once

#include <Arduino.h>

#include "singleton.hpp"
#include <Motoron.h>

class chassis : public singleton<chassis> {
    MAKE_SINGLETON(chassis)
};