#include "rfid_driver.hpp"

#include "space_robot.hpp"

#include <MFRC522_I2C.h>
#include <Wire.h>
#include <stdio.h>

static MFRC522_I2C mfrc522(i2c_addr::RFID, -1);

namespace RFID {

void begin() {
    mfrc522.PCD_Init();

    if (!mfrc522.PCD_PerformSelfTest()) {
        printf("RAID Cannot Pass Self Test\n");
    }
}

uint32_t update() {
}

}; // namespace RFID
