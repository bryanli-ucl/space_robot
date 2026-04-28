#include <Arduino.h>

#include "space_robot.hpp"

#include "motor_driver.hpp"
#include "rfid_driver.hpp"

#include "tasks/robot_fsm.hpp"

#include "MFRC552_I2C.hpp"

auto setup() -> void {
    pinMode(BOARD_LED_R, OUTPUT);
    pinMode(BOARD_LED_G, OUTPUT);
    pinMode(BOARD_LED_B, OUTPUT);
}

auto loop() -> void {


    digitalWrite(BOARD_LED_R, 1);
    digitalWrite(BOARD_LED_G, 1);
    digitalWrite(BOARD_LED_B, 1);
    delay(1000);
    digitalWrite(BOARD_LED_R, 0);
    digitalWrite(BOARD_LED_G, 0);
    digitalWrite(BOARD_LED_B, 0);
    delay(1000);
}