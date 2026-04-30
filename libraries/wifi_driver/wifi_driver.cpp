#include "wifi_driver.hpp"

#include <stdio.h>

#include "wifi_pwd.hpp"

namespace {

constexpr uint32_t k_connect_timeout_ms   = 15000;
constexpr uint32_t k_retry_interval_ms    = 500;
constexpr uint8_t  k_max_connect_attempts = 3;

auto credentials_configured() -> bool {
    return WIFI_SSID[0] != '\0';
}

auto wifi_status_str(uint8_t status) -> const char* {
    switch (status) {
        case WL_IDLE_STATUS:     return "IDLE";
        case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
        case WL_CONNECTED:       return "CONNECTED";
        case WL_CONNECT_FAILED:  return "CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "CONNECTION_LOST";
        case WL_DISCONNECTED:    return "DISCONNECTED";
        default:                 return "UNKNOWN";
    }
}

} // namespace

namespace WifiDriver {

void begin() {
    if (!credentials_configured()) {
        printf("WiFi credentials are empty, skip connection\n");
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        printf("WiFi already connected, IP: %s\n", WiFi.localIP().toString().c_str());
        return;
    }

    // --- Scan phase ---------------------------------------------------------
    printf("Scanning nearby WiFi networks...\n");
    const int num_networks = WiFi.scanNetworks();
    if (num_networks < 0) {
        printf("WiFi scan failed (error: %d)\n", num_networks);
    } else if (num_networks == 0) {
        printf("No WiFi networks found.\n");
    } else {
        printf("Found %d network(s):\n", num_networks);
        bool target_found = false;
        for (int i = 0; i < num_networks; ++i) {
            const bool is_target = (strcmp(WiFi.SSID(i), WIFI_SSID) == 0);
            if (is_target) target_found = true;
            printf("  [%2d] %-32s RSSI: %4d dBm  %s\n",
                   i + 1,
                   WiFi.SSID(i),
                   static_cast<int>(WiFi.RSSI(i)),
                   is_target ? "<<< TARGET" : "");
        }
        if (!target_found) {
            printf("WARNING: Target SSID '%s' not seen in scan!\n", WIFI_SSID);
        }
    }
    printf("\n");

    // --- Connection phase ---------------------------------------------------
    for (uint8_t attempt = 1; attempt <= k_max_connect_attempts; ++attempt) {
        printf("[Attempt %d/%d] Connecting to '%s'...\n",
               attempt, k_max_connect_attempts, WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PWD);

        const uint32_t start_ms = millis();
        uint8_t prev_status     = 0xFF;
        uint8_t status          = WiFi.status();
        while (status != WL_CONNECTED && (millis() - start_ms) < k_connect_timeout_ms) {
            delay(k_retry_interval_ms);
            status = WiFi.status();
            if (status != prev_status) {
                printf("  status: %s (%d)\n", wifi_status_str(status), status);
                Serial1.flush();
                prev_status = status;
            } else {
                printf(".");
                Serial1.flush();
            }
        }

        if (status == WL_CONNECTED) {
            printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
            return;
        }

        printf("  Attempt %d failed. Final status: %s (%d)\n",
               attempt, wifi_status_str(status), status);

        if (attempt < k_max_connect_attempts) {
            printf("  Retrying in 2 s...\n");
            delay(2000);
        }
    }

    printf("\nWiFi connection failed after %d attempts.\n", k_max_connect_attempts);
}

auto is_connected() -> bool {
    return WiFi.status() == WL_CONNECTED;
}

auto local_ip() -> IPAddress {
    return WiFi.localIP();
}

} // namespace WifiDriver
