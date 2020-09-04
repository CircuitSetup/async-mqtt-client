#pragma once

namespace AsyncMqttClientInternals {
class Helpers {
 public:
  static uint32_t decodeVariableByteInteger(const uint8_t* bytes) {
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

  static uint8_t encodeVariableByteInteger(uint32_t integer, uint8_t* destination) {
    uint8_t currentByte = 0;
    uint8_t bytesNeeded = 0;

    do {
      uint8_t encodedByte = integer % 128;
      integer /= 128;
      if (integer > 0) {
        encodedByte = encodedByte | 128u;
      }

      destination[currentByte++] = encodedByte;
      bytesNeeded++;
    } while (integer > 0);

    return bytesNeeded;
  }

  __always_inline inline static uint16_t bigEndian(uint16_t i) {
    return (i >> 8u) | (i << 8u);
  }

  static uint8_t variableByteIntegerSize(uint32_t i) {
    if (i < (1u << 7u)) {
      return 1;
    } else if (i < (1u << 14u)) {
      return 2;
    } else if (i < (1u << 21u)) {
      return 3;
    } else {
      return 4;
    }
  };
};
}  // namespace AsyncMqttClientInternals
