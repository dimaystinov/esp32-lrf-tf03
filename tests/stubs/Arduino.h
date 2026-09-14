#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <vector>

inline uint32_t fakeNow = 0;
inline uint32_t millis() { return fakeNow; }
inline void delay(uint32_t n) { fakeNow += n; }
constexpr int INPUT_PULLUP = 2;
inline void pinMode(int, int) {}
struct FakeUsb {
  void begin(int) {}
  void setTxTimeoutMs(int) {}
  explicit operator bool() const { return false; } // No host ever connected
  int availableForWrite() { return 0; }
  size_t write(const uint8_t *, size_t) { return 0; }
  size_t write(uint8_t) { return 0; }
};
inline FakeUsb Serial;
struct FakeEsp { const char *getChipModel() { return "ESP32-C3"; } };
inline FakeEsp ESP;
