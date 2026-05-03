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

// Global WiFi client for PC Server connection
static WiFiClient g_wifi_client;

struct wifi_msg_t {
    char buf[128];
};
static Mail<wifi_msg_t, 32> mail_wifi_tx;

auto wifi_tx(const char* fmt, ...) -> void {
    auto mail = mail_wifi_tx.try_alloc();
    if (!mail) return; // queue full, drop
    va_list args;
    va_start(args, fmt);
    vsnprintf(mail->buf, sizeof(mail->buf), fmt, args);
    va_end(args);
    mail_wifi_tx.put(mail);
}

// Mouse-controlled chassis velocities and accelerations
static volatile float g_vx          = 0.0f;  // forward(+)/back(-) speed
static volatile float g_vy          = 0.0f;  // right(+)/left(-) speed
static volatile float g_w           = 0.0f;  // CCW(+)/CW(-) angular speed
static volatile float g_ax          = 0.0f;  // forward acceleration
static volatile float g_ay          = 0.0f;  // lateral acceleration
static volatile bool g_mouse_active = false; // left button held

static void on_mouse_event(uint8_t buttons, int8_t dx, int8_t dy, int8_t dz) {
    g_mouse_active = (buttons & 0x01) != 0;
    if (g_mouse_active) {
        // Mouse movement acts as acceleration only while left button held
        g_ay = dx * 2.0f; // lateral acceleration
        g_ax = dy * 2.0f; // forward acceleration
    } else {
        g_ax = 0.0f;
        g_ay = 0.0f;
    }
    // Wheel controls rotation directly, always active (no left-button required)
    g_w = dz * 60.0f;
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
        // uint32_t uid = rfid.read();
        uint32_t uid = 0x11451419;
        if (uid != 0 && !mail_fsm_event.full()) {
            auto mail = mail_fsm_event.try_alloc();
            if (mail != nullptr) {
                mail->type = RobotEvent::Type::RFIDDetected;
                mail->uid  = uid;
                mail_fsm_event.put(mail);
                wifi_tx("EVT:%d:RFID,0x%08X", ROBOT_ID, uid);
            }
        }
        ThisThread::sleep_for(100ms);
    }
}

auto func_task_motor() -> void {
    using namespace std::chrono_literals;
    constexpr float dt        = 0.02f;
    constexpr float max_speed = 200.0f;
    constexpr float max_w     = 120.0f;

    while (1) {
        // Keep mouse enumerated
        if (!mouse.connected()) {
            if (mouse.connect()) {
                printf("[Motor] Wireless mouse connected!\n");
            }
        }

        bool active = g_mouse_active;

        if (active) {
            // Integrate mouse movement into translational velocity
            g_vx += g_ax * dt;
            g_vy += g_ay * dt;
        } else {
            // Friction braking on translation when left button released
            g_vx *= 0.85f;
            g_vy *= 0.85f;
            if (fabsf(g_vx) < 0.5f) g_vx = 0.0f;
            if (fabsf(g_vy) < 0.5f) g_vy = 0.0f;
        }
        // g_w is set directly by wheel (always active)

        // Clamp to safe limits
        if (g_vx > max_speed) g_vx = max_speed;
        if (g_vx < -max_speed) g_vx = -max_speed;
        if (g_vy > max_speed) g_vy = max_speed;
        if (g_vy < -max_speed) g_vy = -max_speed;
        if (g_w > max_w) g_w = max_w;
        if (g_w < -max_w) g_w = -max_w;

        chassis.drive(g_vx, g_vy, g_w);
        ThisThread::sleep_for(20ms);
    }
}

auto func_task_debug() -> void {
    while (1) {
        int16_t fl, fr, rl, rr;
        chassis.get_wheel_speeds(fl, fr, rl, rr);
        const char* state = g_mouse_active ? "ACTIVE" : "idle  ";
        printf("[Chassis] FL=%4d FR=%4d RL=%4d RR=%4d | %s | v=(%6.1f %6.1f %6.1f) a=(%6.1f %6.1f)\n",
        fl, fr, rl, rr, state, g_vx, g_vy, g_w, g_ax, g_ay);

        ThisThread::sleep_for(500ms);
    }
}

auto func_task_wifi() -> void {
    printf("[WiFi] Task started, Robot ID = %d\n", ROBOT_ID);
    digitalWrite(BOARD_LED_B, LOW); // 蓝灯亮，表示 WiFi 线程已启动

    // Wait for WiFi to come up
    int wifi_wait_cnt = 0;
    while (!WifiDriver::is_connected()) {
        printf("[WiFi] Waiting for connection... (%d)\n", ++wifi_wait_cnt);
        ThisThread::sleep_for(500ms);
    }

    const IPAddress ip = WifiDriver::local_ip();
    printf("[WiFi] Connected, IP: %s\n", ip.toString().c_str());
    digitalWrite(BOARD_LED_B, HIGH); // 蓝灯灭

    char rx_buffer[64];
    int rx_len              = 0;
    int report_cnt          = 0;
    int disconnect_cooldown = 0;

    while (1) {
        // Try to connect to PC Server if not connected
        if (!g_wifi_client.connected()) {
            digitalWrite(BOARD_LED_G, HIGH); // 绿灯灭
            g_wifi_client.stop();            // 清理旧 socket，避免 readSocket 线程崩溃
            if (disconnect_cooldown > 0) {
                disconnect_cooldown--;
                ThisThread::sleep_for(100ms);
                continue;
            }
            printf("[WiFi] Connecting to PC %s:%d ...\n", PC_IP, PC_PORT);
            if (g_wifi_client.connect(PC_IP, PC_PORT)) {
                printf("[WiFi] Connected to PC Server\n");
                char hello_buf[32];
                snprintf(hello_buf, sizeof(hello_buf), "HELLO:%d", ROBOT_ID);
                g_wifi_client.println(hello_buf);
                digitalWrite(BOARD_LED_G, LOW); // 绿灯亮，表示已连上PC
                rx_len = 0;
                wifi_tx("LOG:%d:Robot online", ROBOT_ID);
            } else {
                printf("[WiFi] Connection failed, retry in 5s\n");
                disconnect_cooldown = 50; // 50 * 100ms = 5s cooldown
                continue;
            }
        }

        // Non-blocking read line-by-line from PC
        while (g_wifi_client.available() && rx_len < 63) {
            char c = static_cast<char>(g_wifi_client.read());
            if (c == '\n') {
                rx_buffer[rx_len] = '\0';
                printf("[TCP] RX: %s\n", rx_buffer);

                // Parse CMD:<id>:<command> or CMD:ALL:<command>
                if (strncmp(rx_buffer, "CMD:", 4) == 0) {
                    char* p     = rx_buffer + 4;
                    char* colon = strchr(p, ':');
                    if (colon) {
                        *colon      = '\0';
                        bool for_me = (strcmp(p, "ALL") == 0) || (atoi(p) == ROBOT_ID);
                        char* cmd   = colon + 1;

                        if (for_me) {
                            printf("[TCP] Command for me: %s\n", cmd);

                            if (strcmp(cmd, "warning") == 0) {
                                printf("[TCP] !!! WARNING RECEIVED !!!\n");
                                digitalWrite(BOARD_LED_R, LOW);
                                digitalWrite(BOARD_LED_G, HIGH);
                                digitalWrite(BOARD_LED_B, LOW);
                                wifi_tx("LOG:%d:WARNING executed", ROBOT_ID);

                                if (!mail_fsm_event.full()) {
                                    auto mail = mail_fsm_event.try_alloc();
                                    if (mail != nullptr) {
                                        mail->type = RobotEvent::Type::EmergencyWarning;
                                        mail_fsm_event.put(mail);
                                    }
                                }
                            } else if (strcmp(cmd, "shutdown") == 0) {
                                printf("[TCP] !!! SHUTDOWN RECEIVED !!!\n");
                                digitalWrite(BOARD_LED_R, LOW);
                                digitalWrite(BOARD_LED_G, HIGH);
                                digitalWrite(BOARD_LED_B, HIGH);
                                wifi_tx("LOG:%d:SHUTDOWN executed", ROBOT_ID);
                            } else {
                                wifi_tx("LOG:%d:Unknown command '%s'", ROBOT_ID, cmd);
                            }
                        }
                    }
                }

                rx_len = 0;
            } else if (c != '\r' && rx_len < 63) {
                rx_buffer[rx_len++] = c;
            }
        }

        // Periodic chassis report (~500ms)
        report_cnt++;
        if (report_cnt >= 10) {
            report_cnt = 0;
            int16_t fl, fr, rl, rr;
            chassis.get_wheel_speeds(fl, fr, rl, rr);
            wifi_tx("DAT:%d:MOTOR,fl=%d,fr=%d,rl=%d,rr=%d", ROBOT_ID, fl, fr, rl, rr);
            wifi_tx("DAT:%d:VEL,vx=%.1f,vy=%.1f,w=%.1f,ax=%.1f,ay=%.1f,active=%d", ROBOT_ID, g_vx, g_vy, g_w, g_ax, g_ay, g_mouse_active ? 1 : 0);
        }

        // Flush outbound messages from other tasks (single-threaded WiFi TX)
        while (true) {
            osEvent evt = mail_wifi_tx.get(0);
            if (evt.status == osEventMail) {
                wifi_msg_t* msg = (wifi_msg_t*)evt.value.p;
                if (g_wifi_client.connected()) {
                    g_wifi_client.println(msg->buf);
                }
                mail_wifi_tx.free(msg);
            } else {
                break;
            }
        }

        ThisThread::sleep_for(100ms);
    }
}

auto func_task_fsm() -> void {
    printf("[FSM] Task started\n");
    FSM_Robot::start();

    while (1) {
        RobotEvent* ev = mail_fsm_event.try_get();
        if (ev != nullptr) {
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
        WifiDriver::begin();
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
        task_wifi.start(func_task_wifi);
        task_fsm.start(func_task_fsm);
        task_motor.start(func_task_motor);
        task_rfid.start(func_task_rfid);
        task_debug.start(func_task_debug);
    }

    printf("\n========== SETUP COMPLETE ==========\n\n");
}

auto loop() -> void {
    // Yield CPU to let RTOS threads run; main thread must not spin busy.
    ThisThread::sleep_for(10ms);
}
