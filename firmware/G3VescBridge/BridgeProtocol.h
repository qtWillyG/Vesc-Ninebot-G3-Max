#pragma once

#include <Arduino.h>

namespace protocol {

constexpr size_t kI2cFrameSize = 8;
constexpr size_t kI2cPayloadSize = 6;

inline uint16_t checksum(const uint8_t* bytes, size_t length) {
  uint16_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum = static_cast<uint16_t>(sum + bytes[i]);
  }
  return static_cast<uint16_t>(~sum);
}

inline bool validateFrame(const uint8_t* frame, size_t size) {
  if (size < 9 || frame[0] != 0x5a || frame[1] != 0xa5) return false;
  const size_t bodySize = static_cast<size_t>(frame[2]) + 7;
  if (size != bodySize + 2) return false;
  const size_t checkedLength = static_cast<size_t>(frame[2]) + 5;
  const uint8_t* body = frame + 2;
  const uint16_t expected = static_cast<uint16_t>(body[checkedLength]) |
                            (static_cast<uint16_t>(body[checkedLength + 1]) << 8);
  return checksum(body, checkedLength) == expected;
}

inline void appendChecksum(uint8_t* bytes, size_t payloadLength) {
  const uint16_t value = checksum(bytes, payloadLength);
  bytes[payloadLength] = static_cast<uint8_t>(value & 0xff);
  bytes[payloadLength + 1] = static_cast<uint8_t>(value >> 8);
}

}  // namespace protocol

