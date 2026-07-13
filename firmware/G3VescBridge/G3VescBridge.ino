#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

#include "BridgeConfig.h"
#include "BridgeProtocol.h"

namespace {

struct CandidateControls {
  uint8_t throttle = 0;
  uint8_t brake = 0;
  uint8_t aux = 0x40;
  int64_t receivedUs = 0;
};

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
CandidateControls controls;
uint8_t i2cRx[protocol::kI2cFrameSize] = {};
uint8_t i2cTx[protocol::kI2cFrameSize] = {};

void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

void logFrame(const uint8_t* frame, size_t size, bool checksumOk) {
  Serial.print(millis());
  Serial.print(F(",RX,"));
  Serial.print(checksumOk ? F("OK,") : F("BAD,"));
  Serial.print(size);
  Serial.print(',');
  for (size_t i = 0; i < size; ++i) {
    printHexByte(frame[i]);
    if (i + 1 != size) Serial.print(' ');
  }
  Serial.println();
}

void inspectCandidateG2Layout(const uint8_t* frame, size_t size) {
  if (!config::kG3ProtocolVerified || !protocol::validateFrame(frame, size)) return;
  if (size <= config::kCandidateAuxIndex) return;
  if (frame[3] != config::kCandidateSourceDashboard ||
      frame[4] != config::kCandidateDestinationEsc) return;

  portENTER_CRITICAL(&stateMux);
  controls.throttle = frame[config::kCandidateThrottleIndex];
  controls.brake = frame[config::kCandidateBrakeIndex];
  controls.aux = frame[config::kCandidateAuxIndex];
  controls.receivedUs = esp_timer_get_time();
  portEXIT_CRITICAL(&stateMux);
}

class FrameParser {
 public:
  void poll() {
    while (Serial1.available() > 0) consume(static_cast<uint8_t>(Serial1.read()));
    if (used_ > 0 && static_cast<uint64_t>(esp_timer_get_time() - startedUs_) >
                         config::kFrameTimeoutUs) {
      reset();
    }
  }

 private:
  uint8_t frame_[config::kMaxFrameSize] = {};
  size_t used_ = 0;
  size_t expected_ = 0;
  int64_t startedUs_ = 0;

  void reset() {
    used_ = 0;
    expected_ = 0;
  }

  void consume(uint8_t byte) {
    if (used_ == 0) {
      if (byte != 0x5a) return;
      frame_[used_++] = byte;
      startedUs_ = esp_timer_get_time();
      return;
    }
    if (used_ == 1) {
      if (byte == 0xa5) {
        frame_[used_++] = byte;
      } else {
        reset();
        if (byte == 0x5a) consume(byte);
      }
      return;
    }

    if (used_ >= sizeof(frame_)) {
      reset();
      return;
    }
    frame_[used_++] = byte;
    if (used_ == 3) {
      expected_ = static_cast<size_t>(frame_[2]) + 9;
      if (expected_ > sizeof(frame_) || expected_ < 9) {
        reset();
        return;
      }
    }
    if (expected_ != 0 && used_ == expected_) {
      const bool valid = protocol::validateFrame(frame_, used_);
      logFrame(frame_, used_, valid);
      inspectCandidateG2Layout(frame_, used_);
      reset();
    }
  }
};

FrameParser parser;

void onI2cReceive(int count) {
  size_t received = 0;
  while (Wire.available() > 0) {
    const uint8_t value = static_cast<uint8_t>(Wire.read());
    if (received < sizeof(i2cRx)) i2cRx[received] = value;
    ++received;
  }
  (void)count;
  (void)received;
}

void onI2cRequest() {
  const int64_t nowUs = esp_timer_get_time();
  portENTER_CRITICAL(&stateMux);
  const bool fresh = controls.receivedUs > 0 && nowUs >= controls.receivedUs &&
                     static_cast<uint64_t>(nowUs - controls.receivedUs) <
                         config::kInputTimeoutUs;
  const bool armed = config::kG3ProtocolVerified && config::kAllowDriveOutput && fresh;
  i2cTx[0] = armed ? controls.throttle : 0;
  i2cTx[1] = armed ? controls.brake : 0;
  i2cTx[2] = armed ? controls.aux : 0x40;
  i2cTx[3] = 0x01;
  i2cTx[4] = armed ? 0 : 1;
  i2cTx[5] = 0;
  portEXIT_CRITICAL(&stateMux);
  protocol::appendChecksum(i2cTx, protocol::kI2cPayloadSize);
  Wire.write(i2cTx, sizeof(i2cTx));
}

}  // namespace

void setup() {
  Serial.begin(config::kDebugBaud);
  Serial1.begin(config::kDashboardBaud, SERIAL_8N1,
                config::kDashboardRxPin, config::kDashboardTxPin);
  Wire.onReceive(onI2cReceive);
  Wire.onRequest(onI2cRequest);
  Wire.begin(config::kI2cAddress);

  delay(250);
  Serial.println(F("ms,direction,checksum,length,hex"));
  Serial.println(F("MAX G3 discovery bridge ready; motor output is DISABLED"));
}

void loop() {
  parser.poll();
  delay(1);
}
