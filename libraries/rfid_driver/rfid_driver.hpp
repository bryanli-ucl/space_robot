#pragma once

#include <Arduino.h>
#include <MFRC522_I2C.h>
#include <Wire.h>

class RFIDReader {
public:
    explicit RFIDReader(uint8_t i2c_addr, int8_t rst_pin = -1, TwoWire* wire = &Wire);

    void begin();

    // Returns the 4-byte UID as uint32_t, or 0 if no card / error.
    auto read() -> uint32_t;

private:
    MFRC522_I2C mfrc522_;
};
