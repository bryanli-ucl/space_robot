#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "ICM_20948.h"

namespace IMUDriver {

class ICM20948 {
public:
    // wire   : I2C bus instance (Wire or Wire1)
    // ad0_high: true if AD0 pin is tied to VCC (address 0x69),
    //           false if AD0 is grounded (address 0x68, default)
    ICM20948(TwoWire& wire, bool ad0_high = false);

    auto begin() -> bool;

    // Poll for new data. Returns true if a fresh sample was read.
    auto update() -> bool;

    // Returns true if the device responded during begin()
    auto is_connected() const -> bool;

    // Returns true when new sensor data is available
    auto data_ready() -> bool;

    // --- Accelerometer (milli-g) ---
    auto acc_x_mg() -> float;
    auto acc_y_mg() -> float;
    auto acc_z_mg() -> float;

    // --- Gyroscope (degrees per second) ---
    auto gyro_x_dps() -> float;
    auto gyro_y_dps() -> float;
    auto gyro_z_dps() -> float;

    // --- Magnetometer (micro-tesla) ---
    auto mag_x_ut() -> float;
    auto mag_y_ut() -> float;
    auto mag_z_ut() -> float;

    // --- Temperature (Celsius) ---
    auto temp_c() -> float;

    // --- Raw 16-bit readings (LSB) ---
    auto raw_acc_x() -> int16_t;
    auto raw_acc_y() -> int16_t;
    auto raw_acc_z() -> int16_t;

    auto raw_gyro_x() -> int16_t;
    auto raw_gyro_y() -> int16_t;
    auto raw_gyro_z() -> int16_t;

    auto raw_mag_x() -> int16_t;
    auto raw_mag_y() -> int16_t;
    auto raw_mag_z() -> int16_t;

    // Last operation status (from SparkFun library)
    auto last_status() -> ICM_20948_Status_e;

private:
    ICM_20948_I2C device_;
    TwoWire& wire_;
    bool ad0_high_;
    bool initialized_ = false;
};

} // namespace IMUDriver
