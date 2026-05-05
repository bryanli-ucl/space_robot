#include "position_motor.hpp"

namespace MotorDriver {

PositionMotor::PositionMotor(L298N& driver, uint8_t motor_channel,
                             uint8_t enc_a, uint8_t enc_b)
    : driver_(driver), channel_(motor_channel), encoder_(enc_a, enc_b) {}

void PositionMotor::begin() {
    encoder_.begin();
}

void PositionMotor::set_pid(float kp, float ki, float kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

void PositionMotor::set_target_ticks(int32_t ticks) {
    target_ = ticks;
}

void PositionMotor::set_target_angle(float degrees, float ticks_per_rev) {
    target_ = static_cast<int32_t>(degrees / 360.0f * ticks_per_rev);
}

void PositionMotor::update() {
    int32_t pos   = inverted_ ? -encoder_.count() : encoder_.count();
    int32_t error = target_ - pos;

    // Integral windup protection: only accumulate when error is small
    if (error < 1000 && error > -1000) {
        integral_ += error;
        if (integral_ > MAX_OUTPUT * 2) integral_ = MAX_OUTPUT * 2;
        if (integral_ < -MAX_OUTPUT * 2) integral_ = -MAX_OUTPUT * 2;
    } else {
        // Clear integral when far from target to avoid saturation overshoot
        integral_ = 0;
    }

    int32_t derivative = error - last_error_;
    last_error_        = error;

    float output_f = kp_ * static_cast<float>(error)
                     + ki_ * static_cast<float>(integral_)
                     + kd_ * static_cast<float>(derivative);
    int16_t output = static_cast<int16_t>(output_f);

    if (output > MAX_OUTPUT) output = MAX_OUTPUT;
    if (output < -MAX_OUTPUT) output = -MAX_OUTPUT;

    last_output_ = output;

    if (channel_ == 0) {
        driver_.set_motor_a(output);
    } else {
        driver_.set_motor_b(output);
    }
}

void PositionMotor::set_pwm(int16_t pwm) {
    last_output_ = pwm;
    if (channel_ == 0) {
        driver_.set_motor_a(pwm);
    } else {
        driver_.set_motor_b(pwm);
    }
}

void PositionMotor::stop() {
    target_    = encoder_.count();
    integral_  = 0;
    last_error_ = 0;
    if (channel_ == 0) {
        driver_.stop_a();
    } else {
        driver_.stop_b();
    }
}

bool PositionMotor::is_at_target(int32_t deadzone) const {
    int32_t err = target_ - encoder_.count();
    return err <= deadzone && err >= -deadzone;
}

int32_t PositionMotor::get_position() const {
    return encoder_.count();
}

int32_t PositionMotor::get_target() const {
    return target_;
}

} // namespace MotorDriver
