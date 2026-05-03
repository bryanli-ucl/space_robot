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

auto RFIDReader::read() -> uint32_t {
    // 1. Check if a new card is present in the RF field
    if (!mfrc522_.PICC_IsNewCardPresent()) {
        return 0;
    }

    // 2. Read the card serial number (UID)
    if (!mfrc522_.PICC_ReadCardSerial()) {
        return 0;
    }

    // 3. Boundary check: we only support 4-byte UIDs
    const uint8_t uid_size = mfrc522_.uid.size;
    if (uid_size != 4) {
        printf("RFID unsupported UID size: %u (expected 4)\n", uid_size);
        mfrc522_.PICC_HaltA();
        return 0;
    }

    // 4. Assemble 4 bytes into uint32_t with explicit bounds checking
    uint32_t uid = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        uid = (uid << 8) | static_cast<uint32_t>(mfrc522_.uid.uidByte[i]);
    }

    // 5. Halt the PICC so it can be detected again later
    mfrc522_.PICC_HaltA();

    return uid;
}
