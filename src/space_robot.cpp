#include <Arduino.h>
#include <stdio.h>

#include <Wire.h>

#include "space_robot.hpp"

#include "motor_driver.hpp"
#include "rfid_driver.hpp"
#include "ultrasonic_driver.hpp"
#include "wifi_driver.hpp"

// Ultrasonic sensor instances
static Ultrasonic us_front(D53, D52);
static Ultrasonic us_left (D4, D5);
static Ultrasonic us_right(D6, D7);

// RFID reader instance
static RFIDReader rfid(i2c_addr::RFID);

#include "tasks/robot_fsm.hpp"

std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;

auto setup() -> void {

    { // pin setup
        pinMode(BOARD_LED_R, OUTPUT);
        pinMode(BOARD_LED_G, OUTPUT);
        pinMode(BOARD_LED_B, OUTPUT);

        digitalWrite(BOARD_LED_R, HIGH);
        digitalWrite(BOARD_LED_G, HIGH);
        digitalWrite(BOARD_LED_B, HIGH);
    }

    { // system begin
        Serial1.begin(115200);
        while (!Serial1) delay(100);
        printf("Serial1 begin\n");

        // SerialUSB.begin(115200);
        // while (!SerialUSB) delay(100);
        // printf("SerialUSB begin\n");

        printf("All Serials begin\n");

        Wire.begin();
        Wire1.begin();
        printf("Wires begin\n");

        delay(100);
        printf("System Begin\n");
    }

    // { // scan i2c
    //     printf("Wire scan begin\n");
    //     for (byte i = 1; i < 127; i++) {
    //         Wire.beginTransmission(i);
    //         if (Wire.endTransmission() == 0) {
    //             printf("Found: 0x%x\n", i);
    //         }
    //     }
    //     printf("Wire scan finished\n");

    //     printf("Wire1 scan begin\n");
    //     for (byte i = 1; i < 127; i++) {
    //         Wire1.beginTransmission(i);
    //         if (Wire1.endTransmission() == 0) {
    //             printf("Found: 0x%x\n", i);
    //         }
    //     }
    //     printf("Wire1 scan finished\n");
    // }

    { // motor driver begin
        MotorDriver::begin();
    }

    { // wifi begin
        WifiDriver::begin();
    }

    { // rfid begin
        rfid.begin();
    }

    { // ultrasonic sensors begin
        if (us_front.measure_cm() != -1.f) {
            printf("Front ultrasound success\n");
        } else {
            printf("Front ultrasound error\n");
        }
        printf("Ultrasound inited");
    }
}

auto loop() -> void {
    const float d_front = us_front.measure_cm();
    const float d_left  = us_left.measure_cm();
    const float d_right = us_right.measure_cm();

    printf(
    "Distances  F: %6.1f cm  L: %6.1f cm  R: %6.1f cm\n",
    d_front, d_left, d_right);

    delay(500);
}
