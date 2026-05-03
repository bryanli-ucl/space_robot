#include "l298n.hpp"

#include <stdio.h>

L298N::L298N(uint8_t en_a, uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4, uint8_t en_b)
: en_a_(en_a), in1_(in1), in2_(in2),
  en_b_(en_b), in3_(in3), in4_(in4) {}

void L298N::begin() {
    pinMode(en_a_, OUTPUT);
    pinMode(in1_, OUTPUT);
    pinMode(in2_, OUTPUT);
    pinMode(en_b_, OUTPUT);
    pinMode(in3_, OUTPUT);
    pinMode(in4_, OUTPUT);

    digitalWrite(in1_, LOW);
    digitalWrite(in2_, LOW);
    digitalWrite(in3_, LOW);
    digitalWrite(in4_, LOW);
    analogWrite(en_a_, 0);
    analogWrite(en_b_, 0);

    printf(
    "L298N initialized  ENA=D%u IN1=D%u IN2=D%u | ENB=D%u IN3=D%u IN4=D%u\n",
    en_a_, in1_, in2_, en_b_, in3_, in4_);
}

auto L298N::clamp_speed(int16_t v) -> uint8_t {
    if (v < 0) v = -v;
    if (v > 255) v = 255;
    return static_cast<uint8_t>(v);
}

void L298N::write_direction(uint8_t in1, uint8_t in2, int16_t speed) {
    if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
    } else if (speed < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
    }
}

void L298N::set_motor_a(int16_t speed) {
    write_direction(in1_, in2_, speed);
    analogWrite(en_a_, clamp_speed(speed));
}

void L298N::set_motor_b(int16_t speed) {
    write_direction(in3_, in4_, speed);
    analogWrite(en_b_, clamp_speed(speed));
}

void L298N::set_speeds(int16_t speed_a, int16_t speed_b) {
    set_motor_a(speed_a);
    set_motor_b(speed_b);
}

void L298N::stop_a() {
    set_motor_a(0);
}

void L298N::stop_b() {
    set_motor_b(0);
}

void L298N::stop_all() {
    stop_a();
    stop_b();
}
