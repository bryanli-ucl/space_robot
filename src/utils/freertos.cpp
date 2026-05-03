#include <Arduino.h>
#include <mbed.h>

using namespace rtos;
using namespace std::chrono_literals;

// 创建线程
Thread ledThread;
Thread serialThread;

// LED引脚（GIGA内置LED一般是 LED_BUILTIN）
const int ledPin = LED_BUILTIN;

// LED任务
void ledTask() {
    while (true) {
        digitalWrite(ledPin, HIGH);
        ThisThread::sleep_for(500ms);

        digitalWrite(ledPin, LOW);
        ThisThread::sleep_for(500ms);
    }
}

// 串口任务
void serialTask() {
    int count = 0;
    while (true) {
        Serial3.print("Hello from RTOS! Count = ");
        Serial3.println(count++);
        ThisThread::sleep_for(1000ms);
    }
}
 
void setup() {
    Serial3.begin(115200);
    pinMode(ledPin, OUTPUT);

    // 启动线程
    ledThread.start(ledTask);
    serialThread.start(serialTask);
}

void loop() {
    // 留空（RTOS在跑）
}