#include "motor_driver.hpp"

#include <Motoron.h>
#include <Wire.h>
#include <stdio.h>

#include "space_robot.hpp"

static MotoronI2C mc1(i2c_addr::Motoron1);
static MotoronI2C mc2(i2c_addr::Motoron2);

namespace {

struct MotorEndpoint {
    uint8_t controller;
    uint8_t channel;
};

constexpr uint8_t k_first_chassis_motor_channel = 2;
constexpr uint8_t k_last_chassis_motor_channel  = 3;
constexpr uint16_t k_default_acceleration      = 80;
constexpr uint16_t k_default_deceleration      = 300;

auto get_controller(uint8_t controller) -> MotoronI2C* {
    switch (controller) {
    case 1:
        return &mc1;
    case 2:
        return &mc2;
    default:
        return nullptr;
    }
}

auto get_chassis_endpoint(MotorDriver::ChassisMotor motor) -> MotorEndpoint {
    switch (motor) {
    case MotorDriver::ChassisMotor::FrontLeft:
        return {1, 2};
    case MotorDriver::ChassisMotor::FrontRight:
        return {1, 3};
    case MotorDriver::ChassisMotor::RearLeft:
        return {2, 2};
    case MotorDriver::ChassisMotor::RearRight:
        return {2, 3};
    }

    return {0, 0};
}

void setup_motoron(MotoronI2C& controller) {
    controller.setBus(&Wire1);
    controller.reinitialize();
    controller.disableCrc();
    controller.clearResetFlag();
    controller.disableCommandTimeout();

    for (uint8_t motor = k_first_chassis_motor_channel; motor <= k_last_chassis_motor_channel; ++motor) {
        controller.setMaxAcceleration(motor, k_default_acceleration);
        controller.setMaxDeceleration(motor, k_default_deceleration);
    }
}

} // namespace

namespace MotorDriver {

void begin() {
    setup_motoron(mc1);
    setup_motoron(mc2);
    printf("Motor drivers initialized at 0x%02x and 0x%02x on Wire1\n",
    i2c_addr::Motoron1, i2c_addr::Motoron2);
}

auto motoron1() -> MotoronI2C& {
    return mc1;
}

auto motoron2() -> MotoronI2C& {
    return mc2;
}

void set_speed(uint8_t controller, uint8_t motor, int16_t speed) {
    auto* driver = get_controller(controller);
    if (driver == nullptr) {
        printf("Invalid Motoron controller index: %u\n", controller);
        return;
    }

    driver->setSpeed(motor, speed);
}

void set_chassis_speed(ChassisMotor motor, int16_t speed) {
    const auto endpoint = get_chassis_endpoint(motor);
    set_speed(endpoint.controller, endpoint.channel, speed);
}

void set_chassis_speeds(
ChassisMotor first_motor, int16_t first_speed,
ChassisMotor second_motor, int16_t second_speed,
ChassisMotor third_motor, int16_t third_speed,
ChassisMotor fourth_motor, int16_t fourth_speed) {
    set_chassis_speed(first_motor, first_speed);
    set_chassis_speed(second_motor, second_speed);
    set_chassis_speed(third_motor, third_speed);
    set_chassis_speed(fourth_motor, fourth_speed);
}

void stop_chassis() {
    set_chassis_speed(ChassisMotor::FrontLeft, 0);
    set_chassis_speed(ChassisMotor::FrontRight, 0);
    set_chassis_speed(ChassisMotor::RearLeft, 0);
    set_chassis_speed(ChassisMotor::RearRight, 0);
}

void stop_all() {
    for (uint8_t controller = 1; controller <= 2; ++controller) {
        auto* driver = get_controller(controller);
        for (uint8_t motor = k_first_chassis_motor_channel; motor <= k_last_chassis_motor_channel; ++motor) {
            driver->setSpeed(motor, 0);
        }
    }
}

}; // namespace MotorDriver
