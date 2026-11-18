#pragma once
#include <stddef.h>
#include <stdint.h>

// Arduino-ESP32 3.3.0 NetworkClient::readBytes() stops at a negative read,
// while NetworkClientSecure::read() can return -1 for a temporary empty TLS
// buffer. ArduinoJson's default Stream adapter therefore sees premature EOF.
// Supply a custom reader that waits for the next fragment and reads in blocks.
// HTTP/1.0 + identity encoding are required: this reader consumes JSON, not
// HTTP chunk framing. Keep memory and both idle/overall wait times bounded.
template <class Client> class OtaJsonReader {
 public:
  explicit OtaJsonReader(Client& client, size_t limit = 65536,
                         uint32_t idleMs = 15000, uint32_t totalMs = 30000)
      : client_(client), limit_(limit), idleMs_(idleMs), totalMs_(totalMs),
        started_(millis()), lastData_(started_) {}

  int read() {
    if (cursor_ < length_) return buffer_[cursor_++];
    for (;;) {
      if (received_ >= limit_) { tooLarge = true; return -1; }
      const uint32_t now = millis();
      if (uint32_t(now - started_) >= totalMs_ || uint32_t(now - lastData_) >= idleMs_) {
        timedOut = true; return -1;
      }
      const int available = client_.available();
      if (available > 0) {
        size_t want = size_t(available);
        if (want > sizeof(buffer_)) want = sizeof(buffer_);
        if (want > limit_ - received_) want = limit_ - received_;
        const int n = client_.read(buffer_, want);
        if (n > 0) {
          length_ = size_t(n); cursor_ = 1; received_ += size_t(n);
          lastData_ = millis(); return buffer_[0];
        }
        // available() and read() can disagree while TLS processes a record.
      } else if (!client_.connected()) return -1;
      delay(1);
    }
  }

  size_t readBytes(char* out, size_t count) {
    size_t n = 0;
    while (n < count) { const int c = read(); if (c < 0) break; out[n++] = char(c); }
    return n;
  }
  size_t received() const { return received_; }
  bool timedOut = false, tooLarge = false;

 private:
  Client& client_;
  size_t limit_, received_ = 0, cursor_ = 0, length_ = 0;
  uint32_t idleMs_, totalMs_, started_, lastData_;
  uint8_t buffer_[512];
};
