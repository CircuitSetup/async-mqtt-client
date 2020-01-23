#pragma once

namespace AsyncMqttClientInternals {
class Helpers {
 public:
  static uint32_t decodeRemainingLength(const uint8_t* bytes) {
    uint32_t multiplier = 1;
    uint32_t value = 0;
    uint8_t currentByte = 0;
    uint8_t encodedByte;
    do {
      encodedByte = bytes[currentByte++];
      value += (encodedByte & 127u) * multiplier;
      multiplier *= 128;
    } while ((encodedByte & 128u) != 0);

    return value;
  }

  static uint8_t encodeRemainingLength(uint32_t remainingLength, uint8_t* destination) {
    uint8_t currentByte = 0;
    uint8_t bytesNeeded = 0;

    do {
      uint8_t encodedByte = remainingLength % 128;
      remainingLength /= 128;
      if (remainingLength > 0) {
        encodedByte = encodedByte | 128u;
      }

      destination[currentByte++] = encodedByte;
      bytesNeeded++;
    } while (remainingLength > 0);

    return bytesNeeded;
  }

  __always_inline inline static uint16_t bigEndian(uint16_t i) {
    return (i >> 8u) | (i << 8u);
  }
};
}  // namespace AsyncMqttClientInternals
