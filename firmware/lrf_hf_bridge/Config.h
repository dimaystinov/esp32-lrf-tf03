#pragma once
#include <stdint.h>

namespace Config {
constexpr int LIDAR_RX = 6;  // Green sensor TX
constexpr int LIDAR_TX = 7;  // Blue sensor RX (no commands sent)
constexpr int FC_TX = 10;   // To flight controller RX
constexpr int FC_RX = 3;    // From flight controller TX
constexpr uint16_t HTTP_PORT = 80;
constexpr uint32_t WIFI_AUTO_OFF_MS = 5 * 60 * 1000;
constexpr uint32_t FC_BAUD = 115200;
constexpr bool SIMULATE_DISTANCE = false;  // Bench test only; disable before real operation
constexpr uint16_t SIMULATED_CM = 15000;
constexpr uint16_t MIN_CM = 5;
constexpr uint16_t MAX_CM = 5000;  // 50m model; 150m:15000, 200m:20000
constexpr uint32_t STALE_MS = 250;
constexpr uint32_t SCAN_MS = 1500;
constexpr uint32_t LOST_MS = 2000;
constexpr uint32_t DISTANCE_PERIOD_MS = 20;
constexpr uint32_t BAUDS[] = {460800, 921600, 115200, 256000, 19200, 9600};
}
