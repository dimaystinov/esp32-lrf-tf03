#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum class ParseResult { None, Good, Bad };

class LrfParser {
 public:
  void reset() { used_ = 0; }
  bool partial() const { return used_ != 0; }
  ParseResult feed(uint8_t byte, uint16_t &cm) {
    if (used_ == 0 && byte != 0x5C) return ParseResult::None;
    bytes_[used_++] = byte;
    if (used_ < 4) return ParseResult::None;
    const uint8_t check = static_cast<uint8_t>(~(bytes_[1] + bytes_[2]));
    if (check == bytes_[3]) {
      cm = uint16_t(bytes_[1]) | (uint16_t(bytes_[2]) << 8);
      reset();
      return ParseResult::Good;
    }
    // Keep any embedded header: it may be the next real frame.
    uint8_t offset = 1;
    while (offset < 4 && bytes_[offset] != 0x5C) ++offset;
    used_ = 4 - offset;
    memmove(bytes_, bytes_ + offset, used_);
    return ParseResult::Bad;
  }
 private:
  uint8_t bytes_[4]{};
  uint8_t used_ = 0;
};

class Measurement {
 public:
  uint16_t cm = 0;
  uint32_t time = 0;
  bool valid = false;
  void accept(uint16_t value, uint32_t now, uint16_t min, uint16_t max) {
    cm = value;
    time = now;
    valid = value >= min && value <= max;
  }
  bool fresh(uint32_t now, uint32_t timeout) const {
    return valid && uint32_t(now - time) < timeout;
  }
};

