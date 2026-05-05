#pragma once

#include <Arduino.h>

#include "encoder.hpp"
#include "l298n.hpp"

namespace MotorDriver {

class PositionMotor {
public:
    // driver: L298N instance reference
    // motor_channel: 0 = motor_a, 1 = motor_b
    // enc_a, enc_b: encoder A/B phase pins (A = interrupt, B = digital input)
    PositionMotor(L298N& driver, uint8_t motor_channel,
                  uint8_t enc_a, uint8_t enc_b);

    void begin();

    void set_pid(float kp, float ki, float kd);

    // Invert encoder direction to match motor wiring
    void set_inverted(bool inverted) { inverted_ = inverted; }

    // Target in encoder ticks
    void set_target_ticks(int32_t ticks);

    // Target angle (degrees). ticks_per_rev depends on encoder PPR and gear ratio.
    void set_target_angle(float degrees, float ticks_per_rev);

    // Call at regular interval (e.g. 1 kHz)
    void update();

    // Direct open-loop PWM output (bypasses PID)
    void set_pwm(int16_t pwm);

    // Stop and clear target
    void stop();

    bool is_at_target(int32_t deadzone = 5) const;

    int32_t get_position() const;
    int32_t get_target() const;
    int16_t get_output() const { return last_output_; }

private:
    L298N& driver_;
    uint8_t channel_;
    Encoder encoder_;

    int32_t target_ = 0;
    int32_t last_error_ = 0;
    int32_t integral_ = 0;
    int16_t last_output_ = 0;

    float kp_ = 2.0f;
    float ki_ = 0.1f;
    float kd_ = 0.5f;
    bool inverted_ = false;

    static constexpr int16_t MAX_OUTPUT = 255;
};

} // namespace MotorDriver
