#include "Arduino.h"

#include "ParsingInformation.hpp"

namespace AsyncMqttClientInternals {

bool ParsingInformation::read(uint16_t& param, const uint8_t* data, size_t len, size_t& currentBytePosition) {
  while(len > currentBytePosition) {
    param |= data[currentBytePosition] << 8 * (sizeof(uint16_t) - _read - 1);
    currentBytePosition++;
    _read++;

    if (_read >= sizeof(uint16_t)) {
      _read = 0;
      return true;
    }
  }
  return false;
}

bool ParsingInformation::readVbi(uint32_t& param, const uint8_t* data, size_t len, size_t& currentBytePosition) {
  while (len > currentBytePosition) {
    uint8_t currentByte = data[currentBytePosition];

    param |= (currentByte & 0x7F) << 7 * _read;
    currentBytePosition++;

    if ((currentByte & 0x80) == 0) {
      _read = 0;
      return true;
    } else {
      _read++;
    }
  }
};
}
