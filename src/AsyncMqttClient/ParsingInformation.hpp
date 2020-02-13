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
  bool read(uint8_t& param, const uint8_t* data, size_t len, size_t& currentBytePosition) {
    if (len == currentBytePosition)
      return false;

    param = data[currentBytePosition++];
    return true;
  }

  bool read(uint16_t& param, const uint8_t* data, size_t len, size_t& currentBytePosition);

  bool readVbi(uint32_t& param, const uint8_t* data, size_t len, size_t& currentBytePosition);
};
}  // namespace AsyncMqttClientInternals
