#pragma once
#include "Arduino.h"
#include <deque>
constexpr int SERIAL_8N1 = 0;
class HardwareSerial {
 public:
  explicit HardwareSerial(int) {}
  std::deque<uint8_t> rx;
  std::vector<std::vector<uint8_t>> tx;
  int room = 512;
  uint32_t baud = 0;
  void end() { rx.clear(); }
  void setRxBufferSize(size_t) {}
  void setTxBufferSize(size_t) {}
  void begin(uint32_t speed, int, int, int) { baud = speed; }
  int available() { return int(rx.size()); }
  int read() { auto b = rx.front(); rx.pop_front(); return b; }
  int availableForWrite() { return room; }
  size_t write(const uint8_t *p, size_t n) { tx.emplace_back(p, p+n); return n; }
};
