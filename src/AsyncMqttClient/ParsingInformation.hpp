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
};
}  // namespace AsyncMqttClientInternals
