#include "macanum_chassis.hpp"

namespace MotorDriver {

namespace {
    inline auto abs_f(float v) -> float { return v < 0.0f ? -v : v; }
} // namespace

MacanumChassis::MacanumChassis(L298N& front, L298N& rear)
    : front_(front), rear_(rear) {}

auto MacanumChassis::begin() -> void {
    front_.begin();
    rear_.begin();
}

auto MacanumChassis::stop() -> void {
    front_.stop_all();
    rear_.stop_all();
}

auto MacanumChassis::set_wheel_speeds(int16_t fl, int16_t fr, int16_t rl, int16_t rr) -> void {
    // Front board: motor_a = FL, motor_b = FR
    // Rear  board: motor_a = RL, motor_b = RR
    cur_fl_ = fl; cur_fr_ = fr; cur_rl_ = rl; cur_rr_ = rr;
    front_.set_speeds(fl, fr);
    rear_.set_speeds(rl, rr);
}

auto MacanumChassis::get_wheel_speeds(int16_t& fl, int16_t& fr, int16_t& rl, int16_t& rr) const -> void {
    fl = cur_fl_; fr = cur_fr_; rl = cur_rl_; rr = cur_rr_;
}

auto MacanumChassis::drive(float vx, float vy, float w) -> void {
    // Standard X-configuration mecanum inverse kinematics.
    // If the robot moves in an unexpected direction, swap the sign of
    // individual wheels in set_wheel_speeds() above.
    float fl = vx - vy - w;
    float fr = vx + vy + w;
    float rl = vx + vy - w;
    float rr = vx - vy + w;

    // Find the largest absolute wheel speed.
    float max_val = abs_f(fl);
    if (abs_f(fr) > max_val) max_val = abs_f(fr);
    if (abs_f(rl) > max_val) max_val = abs_f(rl);
    if (abs_f(rr) > max_val) max_val = abs_f(rr);

    // Scale down proportionally if any wheel would exceed the limit.
    if (max_val > static_cast<float>(max_speed_)) {
        float scale = static_cast<float>(max_speed_) / max_val;
        fl *= scale;
        fr *= scale;
        rl *= scale;
        rr *= scale;
    }

    set_wheel_speeds(static_cast<int16_t>(fl),
                     static_cast<int16_t>(fr),
                     static_cast<int16_t>(rl),
                     static_cast<int16_t>(rr));
}

} // namespace MotorDriver
