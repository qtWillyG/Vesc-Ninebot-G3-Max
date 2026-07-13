#pragma once

#include <Arduino.h>

namespace config {

// Keep false until a real MAX G3 capture confirms the packet map and the
// throttle/brake byte ranges. False forces zero throttle and brake over I2C.
constexpr bool kG3ProtocolVerified = false;
constexpr bool kAllowDriveOutput = false;

// Arduino Nano ESP32 pins. Do not connect these to the scooter until voltage
// levels and connector pins have been measured.
constexpr int8_t kDashboardRxPin = D4;
// Discovery firmware is receive-only so it cannot electrically contend with
// the stock controller. A future verified bridge may assign D3 here.
constexpr int8_t kDashboardTxPin = -1;
constexpr uint8_t kI2cAddress = 0x45;
constexpr uint32_t kDashboardBaud = 115200;
constexpr uint32_t kDebugBaud = 115200;

constexpr uint32_t kInputTimeoutUs = 80000;
constexpr uint32_t kFrameTimeoutUs = 15000;
constexpr size_t kMaxFrameSize = 96;

// Candidate values inherited from the MAX G2 project. They are intentionally
// marked unverified and are used only to annotate captures while drive output
// is disabled.
constexpr uint8_t kCandidateSourceDashboard = 0x21;
constexpr uint8_t kCandidateDestinationEsc = 0x20;
constexpr uint8_t kCandidatePollCommand = 0x64;
constexpr size_t kCandidateThrottleIndex = 8;  // full-frame index
constexpr size_t kCandidateBrakeIndex = 9;
constexpr size_t kCandidateAuxIndex = 10;

}  // namespace config
