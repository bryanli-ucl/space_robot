#pragma once

#include <Arduino.h>
#include <HCSR04.h>

class Ultrasonic {
public:
    explicit Ultrasonic(uint8_t trig_pin, uint8_t echo_pin);

    // Returns distance in cm, or negative value if out of range / timeout.
    auto measure_cm() -> float;

    // Temperature-compensated measurement (temperature in °C).
    auto measure_cm(float temperature) -> float;

private:
    UltraSonicDistanceSensor sensor_;
};
