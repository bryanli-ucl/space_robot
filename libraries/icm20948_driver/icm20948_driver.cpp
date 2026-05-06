#include "icm20948_driver.hpp"

namespace IMUDriver {

ICM20948::ICM20948(TwoWire& wire, bool ad0_high)
    : wire_(wire), ad0_high_(ad0_high) {}

auto ICM20948::begin() -> bool {
    device_.begin(wire_, ad0_high_);
    if (device_.status != ICM_20948_Stat_Ok) {
        return false;
    }

    device_.startupDefault();
    initialized_ = (device_.status == ICM_20948_Stat_Ok);
    return initialized_;
}

auto ICM20948::update() -> bool {
    if (!initialized_) {
        return false;
    }
    if (!device_.dataReady()) {
        return false;
    }
    device_.getAGMT();
    return true;
}

auto ICM20948::is_connected() const -> bool {
    return initialized_;
}

auto ICM20948::data_ready() -> bool {
    if (!initialized_) {
        return false;
    }
    return device_.dataReady();
}

// --- Accelerometer (milli-g) ---
auto ICM20948::acc_x_mg() -> float { return device_.accX(); }
auto ICM20948::acc_y_mg() -> float { return device_.accY(); }
auto ICM20948::acc_z_mg() -> float { return device_.accZ(); }

// --- Gyroscope (degrees per second) ---
auto ICM20948::gyro_x_dps() -> float { return device_.gyrX(); }
auto ICM20948::gyro_y_dps() -> float { return device_.gyrY(); }
auto ICM20948::gyro_z_dps() -> float { return device_.gyrZ(); }

// --- Magnetometer (micro-tesla) ---
auto ICM20948::mag_x_ut() -> float { return device_.magX(); }
auto ICM20948::mag_y_ut() -> float { return device_.magY(); }
auto ICM20948::mag_z_ut() -> float { return device_.magZ(); }

// --- Temperature (Celsius) ---
auto ICM20948::temp_c() -> float { return device_.temp(); }

// --- Raw 16-bit readings (LSB) ---
auto ICM20948::raw_acc_x() -> int16_t { return device_.agmt.acc.axes.x; }
auto ICM20948::raw_acc_y() -> int16_t { return device_.agmt.acc.axes.y; }
auto ICM20948::raw_acc_z() -> int16_t { return device_.agmt.acc.axes.z; }

auto ICM20948::raw_gyro_x() -> int16_t { return device_.agmt.gyr.axes.x; }
auto ICM20948::raw_gyro_y() -> int16_t { return device_.agmt.gyr.axes.y; }
auto ICM20948::raw_gyro_z() -> int16_t { return device_.agmt.gyr.axes.z; }

auto ICM20948::raw_mag_x() -> int16_t { return device_.agmt.mag.axes.x; }
auto ICM20948::raw_mag_y() -> int16_t { return device_.agmt.mag.axes.y; }
auto ICM20948::raw_mag_z() -> int16_t { return device_.agmt.mag.axes.z; }

auto ICM20948::last_status() -> ICM_20948_Status_e {
    return device_.status;
}

} // namespace IMUDriver
