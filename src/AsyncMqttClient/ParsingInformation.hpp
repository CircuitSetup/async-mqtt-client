#pragma once

#include <vector>

#include "Flags.hpp"
#include "Headers.hpp"

namespace AsyncMqttClientInternals {
enum class BufferState : uint8_t {
  NONE = 0,
  REMAINING_LENGTH = 2, DATA = 3
};

struct ParsingInformation {
  BufferState bufferState = BufferState::NONE;

  std::vector<uint8_t> topicBuffer{};
  PacketType type;
  uint8_t flags;
  uint32_t size;

  size_t _read = 0;
  bool read(uint8_t& param, uint8_t* data, size_t len, size_t& currentBytePosition) {
    while(len > currentBytePosition) {
      param |= data[currentBytePosition] << 8 * (sizeof(uint8_t) - _read - 1);
      currentBytePosition++;
      _read++;

      if (_read >= sizeof(uint8_t)) {
        _read = 0;
        return true;
      }
    }
    return false;
  }

  bool read(uint16_t& param, uint8_t* data, size_t len, size_t& currentBytePosition) {
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

  bool readVbi(uint32_t& param, uint8_t* data, size_t len, size_t& currentBytePosition) {
    while(len > currentBytePosition) {
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
  }
};
}  // namespace AsyncMqttClientInternals
