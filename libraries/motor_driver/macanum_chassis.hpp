#pragma once

#include <cstdint>

#include "l298n.hpp"

namespace MotorDriver {

/**
 * High-level mecanum chassis controller.
 *
 * Maps velocity commands (vx, vy, w) to the four wheel speeds using
 * standard X-configuration mecanum inverse kinematics.
 *
 * Wiring assumption (adjust in set_wheel_speeds() if reversed):
 *   front motor_a = FrontLeft,  front motor_b = FrontRight
 *   rear  motor_a = RearLeft,   rear  motor_b = RearRight
 */
class MacanumChassis {
public:
    MacanumChassis(L298N& front, L298N& rear);

    auto begin() -> void;
    auto stop() -> void;

    // Direct wheel control, range: -255 .. 255
    auto set_wheel_speeds(int16_t fl, int16_t fr, int16_t rl, int16_t rr) -> void;

    // Read back the last commanded wheel speeds
    auto get_wheel_speeds(int16_t& fl, int16_t& fr, int16_t& rl, int16_t& rr) const -> void;

    /**
     * Drive the chassis with mecanum kinematics.
     *
     * @param vx forward (+) / backward (-) speed
     * @param vy right (+) / left (-) speed
     * @param w  CCW rotation (+) / CW rotation (-) speed
     *
     * If any computed wheel speed exceeds max_speed, all four are scaled
     * down proportionally so the direction vector is preserved.
     */
    auto drive(float vx, float vy, float w) -> void;

    auto set_max_speed(int16_t max_speed) -> void { max_speed_ = max_speed; }

private:
    L298N& front_;
    L298N& rear_;
    int16_t max_speed_ = 255;
    int16_t cur_fl_ = 0;
    int16_t cur_fr_ = 0;
    int16_t cur_rl_ = 0;
    int16_t cur_rr_ = 0;
};

} // namespace MotorDriver
