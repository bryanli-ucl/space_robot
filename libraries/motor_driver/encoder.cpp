#include "encoder.hpp"

namespace MotorDriver {

Encoder* Encoder::instances_[MAX_ENCODERS] = {nullptr};
void (*Encoder::isrs_[MAX_ENCODERS])() = {
    Encoder::isr0, Encoder::isr1, Encoder::isr2,
    Encoder::isr3, Encoder::isr4
};

Encoder::Encoder(uint8_t pin_a, uint8_t pin_b)
    : pin_a_(pin_a), pin_b_(pin_b), count_(0), index_(-1) {}

void Encoder::begin() {
    for (int i = 0; i < MAX_ENCODERS; ++i) {
        if (instances_[i] == nullptr) {
            index_ = i;
            instances_[i] = this;
            break;
        }
    }
    if (index_ < 0) return; // no slot available

    pinMode(pin_a_, INPUT_PULLUP);
    pinMode(pin_b_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_a_), isrs_[index_], CHANGE);
}

int32_t Encoder::count() const {
    return count_;
}

void Encoder::reset() {
    count_ = 0;
}

void Encoder::handle_isr() {
    // A-phase changed: if A == B, count up; otherwise count down.
    if (digitalRead(pin_b_) == digitalRead(pin_a_)) {
        count_++;
    } else {
        count_--;
    }
}

void Encoder::isr0() { if (instances_[0]) instances_[0]->handle_isr(); }
void Encoder::isr1() { if (instances_[1]) instances_[1]->handle_isr(); }
void Encoder::isr2() { if (instances_[2]) instances_[2]->handle_isr(); }
void Encoder::isr3() { if (instances_[3]) instances_[3]->handle_isr(); }
void Encoder::isr4() { if (instances_[4]) instances_[4]->handle_isr(); }

} // namespace MotorDriver
