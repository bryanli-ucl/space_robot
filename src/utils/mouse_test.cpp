#include <Arduino.h>

#include "USBHostHID/USBHostMouse.h"

USBHostMouse mouse;

// Mouse reports relative movement, so we accumulate to get a position.
static int32_t pos_x = 0;
static int32_t pos_y = 0;

void on_mouse_event(uint8_t buttons, int8_t dx, int8_t dy, int8_t dz) {
    pos_x += dx;
    pos_y += dy;

    Serial3.print("Buttons: 0b");
    Serial3.print(buttons, BIN);
    Serial3.print(" | dX: ");
    Serial3.print(dx);
    Serial3.print(" | dY: ");
    Serial3.print(dy);
    Serial3.print(" | dZ: ");
    Serial3.print(dz);
    Serial3.print(" | Pos(");
    Serial3.print(pos_x);
    Serial3.print(", ");
    Serial3.print(pos_y);
    Serial3.println(")");
}

void setup() {
    Serial3.begin(115200);
    while (!Serial3);

    mouse.attachEvent(on_mouse_event);
    Serial3.println("USB Host Mouse test started. Plug in a mouse...");
}

static uint32_t last_heartbeat = 0;

void loop() {
    if (!mouse.connected()) {
        if (mouse.connect()) {
            Serial3.println("Mouse connected!");
        }
        delay(100);
        return;
    }

    uint32_t now = millis();
    if (now - last_heartbeat > 1000) {
        Serial3.println("[Loop] Heartbeat, mouse still connected");
        last_heartbeat = now;
    }

    // Events are handled in the background via attachEvent callback.
    delay(10);
}
