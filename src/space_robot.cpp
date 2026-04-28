#include <Arduino.h>
#include <stdio.h>

#include "space_robot.hpp"

#include "motor_driver.hpp"
#include "rfid_driver.hpp"

#include "tasks/robot_fsm.hpp"

#include <Wire.h>

auto setup() -> void {

    { // system begin
        Serial.begin(115200);
        Wire.begin();
        delay(1000);
    }

    { // scan i2c
        for (byte i = 1; i < 127; i++) {
            Wire.beginTransmission(i);
            if (Wire.endTransmission() == 0) {
                printf("Found: 0x%x\n", i);
            }
        }
        printf("i2c scan finished\n");
    }
}

auto loop() -> void {
}
