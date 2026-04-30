#include "rfid_driver.hpp"

#include <stdio.h>

RFIDReader::RFIDReader(uint8_t i2c_addr, int8_t rst_pin, TwoWire* wire)
    : mfrc522_(i2c_addr, rst_pin, wire) {}

void RFIDReader::begin() {
    mfrc522_.PCD_Init();

    if (!mfrc522_.PCD_PerformSelfTest()) {
        printf("RFID Cannot Pass Self Test\n");
    }
}

auto RFIDReader::update() -> uint32_t {
    return 0;
}
