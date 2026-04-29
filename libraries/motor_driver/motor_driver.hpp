#pragma once

#include <Arduino.h>

class MotoronI2C;

namespace MotorDriver {

enum class ChassisMotor : uint8_t {
    FrontLeft,
    FrontRight,
    RearLeft,
    RearRight,
};

void begin();

auto motoron1() -> MotoronI2C&;
auto motoron2() -> MotoronI2C&;

void set_speed(uint8_t controller, uint8_t motor, int16_t speed);
void set_chassis_speed(ChassisMotor motor, int16_t speed);
void set_chassis_speeds(
ChassisMotor first_motor, int16_t first_speed,
ChassisMotor second_motor, int16_t second_speed,
ChassisMotor third_motor, int16_t third_speed,
ChassisMotor fourth_motor, int16_t fourth_speed);
void stop_chassis();
void stop_all();

}; // namespace MotorDriver
