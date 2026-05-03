#pragma once

#include <Arduino.h>

class L298N {
    public:
    L298N(uint8_t en_a, uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4, uint8_t en_b);

    void begin();

    // Speed range: -255 .. 255.  Negative values reverse direction.
    void set_motor_a(int16_t speed);
    void set_motor_b(int16_t speed);
    void set_speeds(int16_t speed_a, int16_t speed_b);

    void stop_a();
    void stop_b();
    void stop_all();

    private:
    uint8_t en_a_, in1_, in2_, en_b_, in3_, in4_;

    static auto clamp_speed(int16_t v) -> uint8_t;
    static void write_direction(uint8_t in1, uint8_t in2, int16_t speed);
};
