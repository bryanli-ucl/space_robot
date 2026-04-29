#pragma once

#include <Arduino.h>

#include <Udp.h>
#include <WiFi.h>

namespace WifiDriver {

void begin();
auto is_connected() -> bool;
auto local_ip() -> IPAddress;

} // namespace WifiDriver
