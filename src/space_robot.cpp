#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Wire.h>
#include <mbed.h>

#include <stdio.h>

#include "space_robot.hpp"

#include "tasks/robot_fsm_gen.hpp"

#include "l298n.hpp"
#include "macanum_chassis.hpp"
#include "rfid_driver.hpp"
#include "ultrasonic_driver.hpp"
#include "wifi_driver.hpp"

#include "USBHostHID/USBHostMouse.h"

using namespace rtos;
using namespace std::chrono_literals;

// Ultrasonic sensor instances
static Ultrasonic us_front(D44, D45);
static Ultrasonic us_left(D46, D47);
static Ultrasonic us_right(D48, D49);

// RFID reader instance
static RFIDReader rfid(i2c_addr::RFID);

// L298N constructor: (en_a, in1, in2, in3, in4, en_b)
// EN pins MUST be PWM-capable on Giga R1.

// L298N #1 (front motors)
static L298N l298n_front(D8, D25, D27, D29, D31, D9);

// L298N #2 (rear motors)
static L298N l298n_rear(D10, D24, D26, D28, D63, D11);

// Mecanum chassis
static MotorDriver::MacanumChassis chassis(l298n_front, l298n_rear);

// USB wireless mouse
static USBHostMouse mouse;

// Mouse-controlled chassis velocities (updated in ISR callback)
static volatile float g_vx = 0.0f; // forward(+)/back(-)
static volatile float g_vy = 0.0f; // right(+)/left(-)
static volatile float g_w  = 0.0f; // CCW(+)/CW(-)

static void on_mouse_event(uint8_t buttons, int8_t dx, int8_t dy, int8_t dz) {
    (void)buttons;
    // Sensitivity tuning: XY motion controls translation, wheel controls rotation.
    g_vy = dx * 3.0f;  // mouse X  → chassis lateral
    g_vx = dy * 3.0f;  // mouse Y  → chassis forward
    g_w  = dz * 60.0f; // wheel    → rotation
}

Thread task_wifi(osPriorityNormal);
Thread task_motor(osPriorityHigh1);
Thread task_fsm(osPriorityAboveNormal3);
Thread task_debug(osPriorityBelowNormal4);
Thread task_rfid(osPriorityBelowNormal4);

Mail<RobotEvent, 16> mail_fsm_event;

std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;

// Functional Declear

auto func_task_rfid() -> void {
    while (1) {
        uint32_t uid = rfid.read();
        if (uid != 0 && !mail_fsm_event.full()) {
            auto mail  = mail_fsm_event.try_alloc();
            mail->type = RobotEvent::Type::RFIDDetected;
            mail->uid  = uid;
            mail_fsm_event.put(mail);
        }

        ThisThread::sleep_for(2s);
    }
}

auto func_task_motor() -> void {
    using namespace std::chrono_literals;

    while (1) {
        // Keep mouse enumerated
        if (!mouse.connected()) {
            if (mouse.connect()) {
                printf("[Motor] Wireless mouse connected!\n");
            }
        } else {
            // Aggressive keep-alive for 2.4G receivers that watchdog-reset
            // after ~2 s of host inactivity.  SET_IDLE + GET_REPORT every 100 ms.
            static auto last_ka = Kernel::Clock::now();
            if (Kernel::Clock::now() - last_ka >= 100ms) {
                USBHost* host = USBHost::getHostInst();
                if (host) {
                    USBDeviceConnected* dev = host->getDevice(0);
                    if (dev) {
                        uint8_t dummy[8];
                        host->controlWrite(dev, 0x21, 0x0A, 0x0000, 0, nullptr, 0);
                        host->controlRead (dev, 0xA1, 0x01, 0x0100, 0, dummy, sizeof(dummy));
                    }
                }
                last_ka = Kernel::Clock::now();
            }
        }

        // Drive chassis from latest mouse deltas
        float vx = g_vx;
        float vy = g_vy;
        float w  = g_w;
        chassis.drive(vx, vy, w);

        ThisThread::sleep_for(20ms);
    }
}

auto func_task_debug() -> void {
    while (1) {
        int16_t fl, fr, rl, rr;
        chassis.get_wheel_speeds(fl, fr, rl, rr);
        printf("[Chassis] FL=%4d FR=%4d RL=%4d RR=%4d | Mouse(vx=%6.1f vy=%6.1f w=%6.1f)\n",
        fl, fr, rl, rr, g_vx, g_vy, g_w);
        ThisThread::sleep_for(500ms);
    }
}

auto func_task_wifi() -> void {
    printf("[WiFi] Task started\n");
    digitalWrite(BOARD_LED_B, LOW); // 蓝灯亮，表示 WiFi 线程已启动

    // Wait for WiFi to come up
    int wifi_wait_cnt = 0;
    while (!WifiDriver::is_connected()) {
        printf("[WiFi] Waiting for connection... (%d)\n", ++wifi_wait_cnt);
        ThisThread::sleep_for(500ms);
    }

    const IPAddress ip = WifiDriver::local_ip();
    printf("[WiFi] Connected, IP: %s\n", ip.toString().c_str());

    WiFiServer server(8080);
    server.begin();
    printf("[TCP] Server started on %s:8080\n", ip.toString().c_str());
    digitalWrite(BOARD_LED_B, HIGH); // 蓝灯灭
    digitalWrite(BOARD_LED_G, LOW);  // 绿灯亮，表示 Server 就绪

    WiFiClient client;
    char rx_buffer[64];
    int rx_len = 0;

    while (1) {
        // Accept new client if current one dropped
        if (!client || !client.connected()) {
            client = server.available();
            if (client) {
                printf("[TCP] Client connected from %s\n",
                client.remoteIP().toString().c_str());
                rx_len = 0;
            }
        }

        // Non-blocking read line-by-line
        if (client && client.connected()) {
            while (client.available() && rx_len < 63) {
                char c = static_cast<char>(client.read());
                if (c == '\n') {
                    rx_buffer[rx_len] = '\0';
                    printf("[TCP] RX: %s\n", rx_buffer);

                    if (strcmp(rx_buffer, "warning") == 0) {
                        printf("[TCP] !!! WARNING RECEIVED !!!\n");
                        digitalWrite(BOARD_LED_R, LOW);  // ON  (active-low)
                        digitalWrite(BOARD_LED_G, HIGH); // OFF
                        digitalWrite(BOARD_LED_B, LOW);  // OFF

                        if (!mail_fsm_event.full()) {
                            auto mail  = mail_fsm_event.alloc();
                            mail->type = RobotEvent::Type::EmergencyWarning;
                            mail_fsm_event.put(mail);
                        }
                    }

                    if (strcmp(rx_buffer, "shutdown") == 0) {
                        printf("[TCP] !!! SHUTDOWN RECEIVED !!!\n");
                        digitalWrite(BOARD_LED_R, LOW);  // ON  (active-low)
                        digitalWrite(BOARD_LED_G, HIGH); // OFF
                        digitalWrite(BOARD_LED_B, HIGH); // OFF
                    }

                    rx_len = 0;
                } else if (c != '\r') {
                    rx_buffer[rx_len++] = c;
                }
            }
        }

        ThisThread::sleep_for(50ms);
    }
}

auto func_task_fsm() -> void {
    printf("[FSM] Task started\n");
    FSM_Robot::start();

    while (1) {
        osEvent evt = mail_fsm_event.get(0);

        if (evt.status == osEventMail) {
            RobotEvent* ev = (RobotEvent*)evt.value.p;
            printf("[FSM] Dispatch event %d\n", static_cast<int>(ev->type));
            FSM_Robot::dispatch(*ev);
            mail_fsm_event.free(ev);
        }

        ThisThread::sleep_for(50ms);
    }
}

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
        Serial3.begin(115200);
        while (!Serial3) delay(100);
        printf("Serial3 begin\n");

        printf("All Serials begin\n");

        Wire.begin();
        Wire1.begin();
        printf("Wires begin\n");

        delay(100);
        printf("System Begin\n");
    }

    { // motor driver begin
        printf(">>> About to init motor driver...\n");
        chassis.begin();
        mouse.attachEvent(on_mouse_event);
        printf(">>> Motor driver init done\n");
    }

    { // wifi begin
        printf(">>> About to init wifi...\n");
        // WifiDriver::begin();
        printf(">>> wifi init done\n");
    }

    { // rfid begin
        printf(">>> About to init RFID...\n");
        // rfid.begin();
        printf(">>> RFID init done\n");
    }

    { // ultrasonic sensors begin
        printf(">>> About to init ultrasound...\n");
        float d = us_front.measure_cm();
        if (d != -1.f) {
            printf("Front ultrasound success: %.1f cm\n", d);
        } else {
            printf("Front ultrasound error / timeout\n");
        }
        printf("Ultrasound inited\n");
        printf(">>> Ultrasound init done\n");
    }

    { // tasks begin
        printf("\n========== TASK SETUP ==========\n\n");
        // task_wifi.start(func_task_wifi);
        task_fsm.start(func_task_fsm);
        task_motor.start(func_task_motor);
        // task_rfid.start(func_task_rfid);
        task_debug.start(func_task_debug);
    }

    printf("\n========== SETUP COMPLETE ==========\n\n");
}

auto loop() -> void {
    // Yield CPU to let RTOS threads run; main thread must not spin busy.
    ThisThread::sleep_for(10ms);
}
