#include <Arduino.h>
#include <Wire.h>

#include "icm20948_driver.hpp"

// ------------------------------------------------------------------
// ICM-20948 Demo with magnetometer calibration & tilt-compensated yaw
// ------------------------------------------------------------------

static IMUDriver::ICM20948 imu(Wire);

// Magnetometer calibration parameters (filled by calibrate_mag)
static float mag_off_x = 0.0f, mag_off_y = 0.0f, mag_off_z = 0.0f;
static float mag_scale_x = 1.0f, mag_scale_y = 1.0f, mag_scale_z = 1.0f;

// Compute tilt-compensated yaw using calibrated magnetometer data.
// Assumes Z-up (az ≈ +1g), X-forward, Y-right.
auto compute_yaw(float ax, float ay, float az,
                 float mx, float my, float mz) -> float {
    // 1. Normalize accelerometer to get gravity vector
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1.0f) norm = 1.0f;
    ax /= norm; ay /= norm; az /= norm;

    // 2. Roll and pitch from accelerometer (radians)
    float roll  = atan2f(ay, az);
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

    // 3. Tilt-compensated magnetometer
    float sin_r = sinf(roll), cos_r = cosf(roll);
    float sin_p = sinf(pitch), cos_p = cosf(pitch);

    float mx_comp = mx * cos_p + my * sin_r * sin_p + mz * cos_r * sin_p;
    float my_comp = my * cos_r - mz * sin_r;

    float yaw = atan2f(-my_comp, mx_comp) * (180.0f / PI);
    if (yaw < 0.0f) yaw += 360.0f;
    return yaw;
}

// Simple hard-iron + soft-iron calibration for the magnetometer.
// Rotate the IMU slowly in all orientations (figure-8 or at least 360° horizontal)
// during the calibration window.
auto calibrate_mag() -> void {
    SerialUSB.println("========================================");
    SerialUSB.println("  Magnetometer Calibration");
    SerialUSB.println("========================================");
    SerialUSB.println("Slowly rotate the IMU in a figure-8 pattern");
    SerialUSB.println("(or at least 360° horizontally) for 10 seconds...");
    SerialUSB.println();

    float min_x = 1e9f, max_x = -1e9f;
    float min_y = 1e9f, max_y = -1e9f;
    float min_z = 1e9f, max_z = -1e9f;

    unsigned long start = millis();
    while (millis() - start < 10000) {
        if (imu.update()) {
            float mx = imu.mag_x_ut();
            float my = imu.mag_y_ut();
            float mz = imu.mag_z_ut();

            if (mx < min_x) min_x = mx;
            if (mx > max_x) max_x = mx;
            if (my < min_y) min_y = my;
            if (my > max_y) max_y = my;
            if (mz < min_z) min_z = mz;
            if (mz > max_z) max_z = mz;
        }
        delay(20);
    }

    // Hard-iron offset = center of the ellipsoid
    mag_off_x = (min_x + max_x) / 2.0f;
    mag_off_y = (min_y + max_y) / 2.0f;
    mag_off_z = (min_z + max_z) / 2.0f;

    // Soft-iron scale = make each axis radius equal
    float radius_x = (max_x - min_x) / 2.0f;
    float radius_y = (max_y - min_y) / 2.0f;
    float radius_z = (max_z - min_z) / 2.0f;
    float avg_r = (radius_x + radius_y + radius_z) / 3.0f;

    if (avg_r > 0.1f) {
        mag_scale_x = avg_r / radius_x;
        mag_scale_y = avg_r / radius_y;
        mag_scale_z = avg_r / radius_z;
    }

    SerialUSB.println("Calibration complete!");
    SerialUSB.print("Offset (uT):  X="); SerialUSB.print(mag_off_x, 2);
    SerialUSB.print(" Y=");              SerialUSB.print(mag_off_y, 2);
    SerialUSB.print(" Z=");              SerialUSB.println(mag_off_z, 2);

    SerialUSB.print("Radius (uT):  X="); SerialUSB.print(radius_x, 2);
    SerialUSB.print(" Y=");              SerialUSB.print(radius_y, 2);
    SerialUSB.print(" Z=");              SerialUSB.println(radius_z, 2);
    SerialUSB.println();
}

void setup() {
    SerialUSB.begin(115200);
    while (!SerialUSB) { delay(10); }

    delay(500);
    SerialUSB.println("\n========================================");
    SerialUSB.println("  ICM-20948 9-DoF IMU Demo");
    SerialUSB.println("========================================\n");

    Wire.begin();
    SerialUSB.print("Initializing IMU... ");

    if (!imu.begin()) {
        SerialUSB.println("FAILED");
        SerialUSB.print("Status: 0x");
        SerialUSB.println(imu.last_status(), HEX);
        while (1) { delay(100); }
    }
    SerialUSB.println("OK\n");

    // Run magnetometer calibration once at startup
    calibrate_mag();
}

void loop() {
    if (imu.update()) {
        float ax = imu.acc_x_mg();
        float ay = imu.acc_y_mg();
        float az = imu.acc_z_mg();

        float gx = imu.gyro_x_dps();
        float gy = imu.gyro_y_dps();
        float gz = imu.gyro_z_dps();

        // Apply calibration to magnetometer
        float mx = (imu.mag_x_ut() - mag_off_x) * mag_scale_x;
        float my = (imu.mag_y_ut() - mag_off_y) * mag_scale_y;
        float mz = (imu.mag_z_ut() - mag_off_z) * mag_scale_z;

        float yaw = compute_yaw(ax, ay, az, mx, my, mz);

        SerialUSB.print("ACC(mg):  ");
        SerialUSB.print(ax, 1);
        SerialUSB.print(", ");
        SerialUSB.print(ay, 1);
        SerialUSB.print(", ");
        SerialUSB.print(az, 1);

        SerialUSB.print(" | GYR(dps): ");
        SerialUSB.print(gx, 1);
        SerialUSB.print(", ");
        SerialUSB.print(gy, 1);
        SerialUSB.print(", ");
        SerialUSB.print(gz, 1);

        SerialUSB.print(" | MAG(uT): ");
        SerialUSB.print(mx, 1);
        SerialUSB.print(", ");
        SerialUSB.print(my, 1);
        SerialUSB.print(", ");
        SerialUSB.print(mz, 1);

        SerialUSB.print(" | YAW(deg): ");
        SerialUSB.print(yaw, 1);

        SerialUSB.print(" | T(C): ");
        SerialUSB.println(imu.temp_c(), 1);
    }

    delay(50); // 20 Hz
}
