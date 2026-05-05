#pragma once

#include <Arduino.h>

namespace MotorDriver {

class Encoder {
public:
    Encoder(uint8_t pin_a, uint8_t pin_b);
    void begin();
    int32_t count() const;
    void reset();

private:
    uint8_t pin_a_, pin_b_;
    volatile int32_t count_;
    int index_;

    static constexpr int MAX_ENCODERS = 5;
    static Encoder* instances_[MAX_ENCODERS];
    static void isr0();
    static void isr1();
    static void isr2();
    static void isr3();
    static void isr4();
    static void (*isrs_[MAX_ENCODERS])();

    void handle_isr();
};

} // namespace MotorDriver
