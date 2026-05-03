#include <Arduino.h>
#include "USBHost/USBHost.h"
#include "USBHostHID/USBHostMouse.h"

USBHostMouse mouse;

void setup() {
    Serial3.begin(115200);
    while (!Serial3);
    Serial3.print("sizeof(USBHost) = "); Serial3.println(sizeof(USBHost));
    Serial3.print("sizeof(USBEndpoint) = "); Serial3.println(sizeof(USBEndpoint));
    Serial3.print("sizeof(USBDeviceConnected) = "); Serial3.println(sizeof(USBDeviceConnected));
    Serial3.print("sizeof(USBHostMouse) = "); Serial3.println(sizeof(USBHostMouse));
    Serial3.print("sizeof(HCTD) = "); Serial3.println(sizeof(HCTD));
    Serial3.print("sizeof(HCED) = "); Serial3.println(sizeof(HCED));
    Serial3.print("USB_THREAD_STACK = "); Serial3.println(USB_THREAD_STACK);
}

void loop() { delay(1000); }
