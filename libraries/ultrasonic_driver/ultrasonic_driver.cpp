#include "ultrasonic_driver.hpp"

#include <stdio.h>

Ultrasonic::Ultrasonic(uint8_t trig_pin, uint8_t echo_pin)
    : sensor_(trig_pin, echo_pin) {}

auto Ultrasonic::measure_cm() -> float {
    return sensor_.measureDistanceCm();
}

auto Ultrasonic::measure_cm(float temperature) -> float {
    return sensor_.measureDistanceCm(temperature);
}
