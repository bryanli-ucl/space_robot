#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Wire.h>
#include <mbed.h>

#include <stdio.h>

#include "space_robot.hpp"

#include "USBHostHID/USBHostMouse.h"
#include "l298n.hpp"
#include "macanum_chassis.hpp"
#include "mouse_control_config.hpp"
#include "position_motor.hpp"
#include "rfid_driver.hpp"
#include "ultrasonic_driver.hpp"
#include "wifi_driver.hpp"

#include "tasks/navigation/navigation_fsm.hpp"
FSM_INITIAL_STATE(navigation::NavigationFSM, navigation::Idle);


using namespace rtos;
using namespace std::chrono_literals;

// Ultrasonic sensor instances
static Ultrasonic us_front(pins::US_FRONT_TRIG, pins::US_FRONT_ECHO);
static Ultrasonic us_left(pins::US_LEFT_TRIG, pins::US_LEFT_ECHO);
static Ultrasonic us_right(pins::US_RIGHT_TRIG, pins::US_RIGHT_ECHO);

// RFID reader instance
RFIDReader rfid(i2c_addr::RFID);

// L298N constructor: (en_a, in1, in2, in3, in4, en_b)
// EN pins MUST be PWM-capable on Giga R1.

// L298N #1 (front motors)
static L298N l298n_front(pins::L298N_FRONT_EN_A, pins::L298N_FRONT_IN1, pins::L298N_FRONT_IN2,
                         pins::L298N_FRONT_IN3, pins::L298N_FRONT_IN4, pins::L298N_FRONT_EN_B);

// L298N #2 (rear motors)
static L298N l298n_rear(pins::L298N_REAR_EN_A, pins::L298N_REAR_IN1, pins::L298N_REAR_IN2,
                        pins::L298N_REAR_IN3, pins::L298N_REAR_IN4, pins::L298N_REAR_EN_B);

// L298N #3 (5th motor, only motor_a used)
// EN_B = D13 (PWM-capable, unused motor_b channel)
static L298N l298n_extra(pins::L298N_EXTRA_EN_A, pins::L298N_EXTRA_IN1, pins::L298N_EXTRA_IN2,
                         pins::L298N_EXTRA_IN3, pins::L298N_EXTRA_IN4, pins::L298N_EXTRA_EN_B);

// Mecanum chassis
static MotorDriver::MacanumChassis chassis(l298n_front, l298n_rear);

// Position-controlled motors
static MotorDriver::PositionMotor motor_fl(l298n_front, 0, pins::ENC_FL_A, pins::ENC_FL_B);
static MotorDriver::PositionMotor motor_fr(l298n_front, 1, pins::ENC_FR_A, pins::ENC_FR_B);
static MotorDriver::PositionMotor motor_rl(l298n_rear, 0, pins::ENC_RL_A, pins::ENC_RL_B);
static MotorDriver::PositionMotor motor_rr(l298n_rear, 1, pins::ENC_RR_A, pins::ENC_RR_B);
static MotorDriver::PositionMotor motor_ex(l298n_extra, 0, pins::ENC_EX_A, pins::ENC_EX_B);

static volatile bool g_position_mode = false;

// Mouse target position on the giant screen (pixels) and heading (radians)
static volatile int32_t g_mouse_target_x   = 0;
static volatile int32_t g_mouse_target_y   = 0;
static volatile float g_mouse_target_theta = 0.0f;

// Last raw mouse event for debug
static volatile int32_t g_last_raw_dx      = 0;
static volatile int32_t g_last_raw_dy      = 0;
static volatile int32_t g_last_raw_dz      = 0;
static volatile uint8_t g_last_buttons     = 0;
static volatile uint32_t g_mouse_event_cnt = 0;

// Estimated chassis pose (world frame)
static volatile int32_t g_pose_x   = 0;
static volatile int32_t g_pose_y   = 0;
static volatile float g_pose_theta = 0.0f;

// Wheel target ticks for position tracking
static volatile int32_t g_wheel_target_fl = 0;
static volatile int32_t g_wheel_target_fr = 0;
static volatile int32_t g_wheel_target_rl = 0;
static volatile int32_t g_wheel_target_rr = 0;

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
    g_last_buttons = buttons;
    g_last_raw_dx  = dx;
    g_last_raw_dy  = dy;
    g_last_raw_dz  = dz;
    g_mouse_event_cnt++;

    g_mouse_active = (buttons & 0x01) != 0;

    // Closed-loop: mouse X directly accumulates FL wheel target ticks
    g_wheel_target_fl += static_cast<int32_t>(dx) * WHEEL_TICKS_PER_MOUSE_DX;

    int32_t new_x = g_mouse_target_x + static_cast<int32_t>(static_cast<float>(dx) * MOUSE_SCALE);
    int32_t new_y = g_mouse_target_y + static_cast<int32_t>(static_cast<float>(dy) * MOUSE_SCALE);

    if (new_x > MOUSE_MAX_POS) new_x = MOUSE_MAX_POS;
    if (new_x < -MOUSE_MAX_POS) new_x = -MOUSE_MAX_POS;
    if (new_y > MOUSE_MAX_POS) new_y = MOUSE_MAX_POS;
    if (new_y < -MOUSE_MAX_POS) new_y = -MOUSE_MAX_POS;

    g_mouse_target_x = new_x;
    g_mouse_target_y = new_y;
    g_mouse_target_theta += dz * WHEEL_SCALE_DEG * (PI / 180.0f);
}

Thread task_wifi(osPriorityNormal);
Thread task_motor(osPriorityHigh1);
Thread task_fsm(osPriorityAboveNormal3);
Thread task_rfid(osPriorityBelowNormal4);
Thread task_position_control(osPriorityHigh);

std::array<std::array<grid_slot_t, GRID_WIDTH>, GRID_HRIGHT> g_grid;

// Functional Declear

// Shared state between WiFi thread (producer) and RFID thread (consumer)
static volatile bool g_rfid_response_ready   = false;
static volatile bool g_rfid_response_fertile = false;

auto func_task_rfid() -> void {
    while (1) {
        // uint32_t uid = rfid.read();
        uint32_t uid = 0;
        if (uid != 0) {
            // Clear any stale flag before sending new query
            g_rfid_response_ready = false;
            wifi_tx("QUERY:%u", uid);

            // Block waiting for server response (2 s timeout)
            bool fertile = false;
            bool ok      = false;
            for (int i = 0; i < 40; ++i) {
                if (g_rfid_response_ready) {
                    fertile               = g_rfid_response_fertile;
                    g_rfid_response_ready = false;
                    ok                    = true;
                    break;
                }
                ThisThread::sleep_for(50ms);
            }

            if (ok) {
                wifi_tx("LOG:%d:RFID tag=%lu -> %s",
                ROBOT_ID,
                static_cast<unsigned long>(uid),
                fertile ? "FERTILE" : "INFERTILE");
                navigation::NavigationFSM::dispatch(
                navigation::RfidDone{ uid, fertile });
            } else {
                wifi_tx("LOG:%d:RFID tag=%lu -> TIMEOUT",
                ROBOT_ID,
                static_cast<unsigned long>(uid));
            }
        }
        ThisThread::sleep_for(50ms);
    }
}

auto func_task_position_control() -> void {
    // Self-test on boot: run FL motor at fixed PWM for 2 s to verify wiring
    wifi_tx("LOG:%d:SELFTEST begin", ROBOT_ID);
    for (int i = 0; i < 2000; ++i) {
        int16_t test_pwm = (i < 1000) ? 150 : -150;  // 1 s forward, 1 s backward
        motor_fl.set_pwm(test_pwm);
        if ((i % 100) == 0) {
            wifi_tx("LOG:%d:SELFTEST i=%d pwm=%d enc=%ld",
                    ROBOT_ID, i, test_pwm,
                    static_cast<long>(motor_fl.get_position()));
        }
        ThisThread::sleep_for(1ms);
    }
    motor_fl.stop();
    wifi_tx("LOG:%d:SELFTEST done", ROBOT_ID);

    while (1) {
        if (!g_position_mode) {
            // Closed-loop single-wheel control: mouse X -> FL wheel target ticks
            motor_fl.set_target_ticks(g_wheel_target_fl);
            motor_fl.update();
            motor_fr.stop();
            motor_rl.stop();
            motor_rr.stop();
            motor_ex.stop();

            // Print at 10 Hz
            static int print_cnt = 0;
            if (++print_cnt >= 100) {
                print_cnt = 0;
                wifi_tx("LOG:%d:CLOSEDLOOP enc=%ld target=%ld pwm=%d",
                        ROBOT_ID,
                        static_cast<long>(motor_fl.get_position()),
                        static_cast<long>(motor_fl.get_target()),
                        motor_fl.get_output());
            }
        } else {
            // Auto-program mode: stop all motors for now
            motor_fl.stop();
            motor_fr.stop();
            motor_rl.stop();
            motor_rr.stop();
            motor_ex.stop();
        }
        ThisThread::sleep_for(1ms);
    }
}

auto func_task_motor() -> void {
    while (1) {
        // Keep mouse enumerated
        if (!mouse.connected()) {
            if (mouse.connect()) {
                wifi_tx("LOG:%d:Mouse connected", ROBOT_ID);
            }
        }
        // All motion control is now handled by task_position_control
        ThisThread::sleep_for(20ms);
    }
}

auto func_task_wifi() -> void {
    wifi_tx("LOG:%d:WiFi task started", ROBOT_ID);

    // Wait for WiFi to come up
    int wifi_wait_cnt = 0;
    while (!WifiDriver::is_connected()) {
        wifi_tx("LOG:%d:WiFi waiting (%d)", ROBOT_ID, ++wifi_wait_cnt);
        ThisThread::sleep_for(500ms);
    }

    const IPAddress ip = WifiDriver::local_ip();
    wifi_tx("LOG:%d:WiFi connected IP=%s", ROBOT_ID, ip.toString().c_str());

    char rx_buffer[64];
    int rx_len              = 0;
    int report_cnt          = 0;
    int disconnect_cooldown = 0;

    while (1) {
        // LED connection state indicator
        if (!WifiDriver::is_connected()) {
            digitalWrite(BOARD_LED_B, (report_cnt % 4 < 2) ? LOW : HIGH);
            digitalWrite(BOARD_LED_G, HIGH);
        } else if (!g_wifi_client.connected()) {
            digitalWrite(BOARD_LED_B, LOW);
            digitalWrite(BOARD_LED_G, HIGH);
        } else {
            digitalWrite(BOARD_LED_B, HIGH);
            digitalWrite(BOARD_LED_G, LOW);
        }

        // Try to connect to PC Server if not connected
        if (!g_wifi_client.connected()) {
            g_wifi_client.stop();
            if (disconnect_cooldown > 0) {
                disconnect_cooldown--;
                ThisThread::sleep_for(100ms);
                continue;
            }
            wifi_tx("LOG:%d:Connecting to PC %s:%d", ROBOT_ID, PC_IP, PC_PORT);
            if (g_wifi_client.connect(PC_IP, PC_PORT)) {
                wifi_tx("LOG:%d:PC Server connected", ROBOT_ID);
                char hello_buf[32];
                snprintf(hello_buf, sizeof(hello_buf), "HELLO:%d", ROBOT_ID);
                g_wifi_client.println(hello_buf);
                rx_len = 0;
                wifi_tx("LOG:%d:Robot online", ROBOT_ID);
            } else {
                wifi_tx("LOG:%d:PC connect failed, retry 5s", ROBOT_ID);
                disconnect_cooldown = 50;
                continue;
            }
        }

        // Non-blocking read line-by-line from PC
        while (g_wifi_client.available() && rx_len < 63) {
            char c = static_cast<char>(g_wifi_client.read());
            if (c == '\n') {
                rx_buffer[rx_len] = '\0';
                wifi_tx("LOG:%d:TCP RX: %s", ROBOT_ID, rx_buffer);

                // Parse CMD:<id>:<command> or CMD:ALL:<command>
                if (strncmp(rx_buffer, "CMD:", 4) == 0) {
                    char* p     = rx_buffer + 4;
                    char* colon = strchr(p, ':');
                    if (colon) {
                        *colon      = '\0';
                        bool for_me = (strcmp(p, "ALL") == 0) || (atoi(p) == ROBOT_ID);
                        char* cmd   = colon + 1;

                        if (for_me) {
                            wifi_tx("LOG:%d:CMD for me: %s", ROBOT_ID, cmd);

                            if (strcmp(cmd, "fertile") == 0) {
                                g_rfid_response_fertile = true;
                                g_rfid_response_ready   = true;
                                wifi_tx("LOG:%d:Server says FERTILE", ROBOT_ID);
                            } else if (strcmp(cmd, "infertile") == 0) {
                                g_rfid_response_fertile = false;
                                g_rfid_response_ready   = true;
                                wifi_tx("LOG:%d:Server says INFERTILE", ROBOT_ID);
                            } else if (strcmp(cmd, "warning") == 0) {
                                wifi_tx("LOG:%d:WARNING received", ROBOT_ID);
                                digitalWrite(BOARD_LED_R, LOW);
                                digitalWrite(BOARD_LED_G, HIGH);
                                digitalWrite(BOARD_LED_B, LOW);
                                wifi_tx("LOG:%d:WARNING executed", ROBOT_ID);

                                // if (!mail_fsm_event.full()) {
                                //     auto mail = mail_fsm_event.try_alloc();
                                //     if (mail != nullptr) {
                                //         mail->type = RobotEvent::Type::EmergencyWarning;
                                //         mail_fsm_event.put(mail);
                                //     }
                                // }
                            } else if (strcmp(cmd, "shutdown") == 0) {
                                wifi_tx("LOG:%d:SHUTDOWN received", ROBOT_ID);
                                digitalWrite(BOARD_LED_R, LOW);
                                digitalWrite(BOARD_LED_G, HIGH);
                                digitalWrite(BOARD_LED_B, HIGH);
                                wifi_tx("LOG:%d:SHUTDOWN executed", ROBOT_ID);
                            } else {
                                wifi_tx("LOG:%d:Unknown cmd '%s'", ROBOT_ID, cmd);
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

            // Extra debug: mouse position control state
            int theta_deg = static_cast<int>(g_mouse_target_theta * 180.0f / PI);
            int pose_deg  = static_cast<int>(g_pose_theta * 180.0f / PI);
            wifi_tx("LOG:%d:POS tgt=(%ld,%ld,%d) pose=(%ld,%ld,%d)",
            ROBOT_ID,
            static_cast<long>(g_mouse_target_x),
            static_cast<long>(g_mouse_target_y),
            theta_deg,
            static_cast<long>(g_pose_x),
            static_cast<long>(g_pose_y),
            pose_deg);
            wifi_tx("LOG:%d:MOUSE conn=%u evt=%lu raw=(%ld,%ld,%ld) btns=%u",
            ROBOT_ID,
            mouse.connected() ? 1u : 0u,
            static_cast<unsigned long>(g_mouse_event_cnt),
            static_cast<long>(g_last_raw_dx),
            static_cast<long>(g_last_raw_dy),
            static_cast<long>(g_last_raw_dz),
            static_cast<unsigned>(g_last_buttons));
            wifi_tx("LOG:%d:MOTORS FL:t=%ld,o=%d,enc=%ld FR:t=%ld,o=%d RL:t=%ld,o=%d RR:t=%ld,o=%d",
            ROBOT_ID,
            static_cast<long>(motor_fl.get_target()), motor_fl.get_output(), static_cast<long>(motor_fl.get_position()),
            static_cast<long>(motor_fr.get_target()), motor_fr.get_output(),
            static_cast<long>(motor_rl.get_target()), motor_rl.get_output(),
            static_cast<long>(motor_rr.get_target()), motor_rr.get_output());
        }

        // Flush outbound messages from other tasks (single-threaded WiFi TX)
        while (!mail_wifi_tx.empty()) {
            wifi_msg_t* msg = mail_wifi_tx.try_get();
            if (g_wifi_client.connected()) {
                g_wifi_client.println(msg->buf);
            }
            mail_wifi_tx.free(msg);
        }

        ThisThread::sleep_for(100ms);
    }
}

auto func_task_fsm() -> void {
    wifi_tx("LOG:%d:FSM task started", ROBOT_ID);

    while (1) {

        navigation::NavigationFSM::update_current();

        ThisThread::sleep_for(5ms);
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
        printf("Serial3 begin\n");

        printf("All Serials begin\n");

        Wire.begin();
        Wire1.begin();
        printf("Wires begin\n");

        delay(100);
        printf("System Begin\n");
    }

    { // wifi begin FIRST so debug messages can be sent ASAP
        printf(">>> About to init wifi...\n");
        WifiDriver::begin();
        printf(">>> wifi init done\n");
    }

    { // motor driver begin
        printf(">>> About to init motor driver...\n");
        chassis.begin();
        l298n_extra.begin();
        motor_fl.begin();
        motor_fr.begin();
        motor_rl.begin();
        motor_rr.begin();
        motor_ex.begin();

        motor_fl.set_pid(-0.1f, 0.0f, 0.0f);  // negative P to match reversed wiring
        motor_fr.set_pid(1, 0, 0);
        motor_rl.set_pid(1, 0, 0);
        motor_rr.set_pid(1, 0, 0);

        mouse.attachEvent(on_mouse_event);
        printf(">>> Motor driver init done\n");
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

    { // fsm init
        printf(">>> About to init FSM...\n");
        navigation::NavigationFSM::start();
        printf(">>> FSM init done\n");
    }

    { // tasks begin
        printf("\n========== TASK SETUP ==========\n\n");
        task_wifi.start(func_task_wifi);
        task_fsm.start(func_task_fsm);
        task_motor.start(func_task_motor);
        task_rfid.start(func_task_rfid);
        task_position_control.start(func_task_position_control);
    }

    printf("\n========== SETUP COMPLETE ==========\n\n");
}

auto loop() -> void {
    // Yield CPU to let RTOS threads run; main thread must not spin busy.
    ThisThread::sleep_for(10ms);
}
