#pragma once

namespace AsyncMqttClientInternals {
class Packet {
 public:
  virtual ~Packet() = default;

  virtual void parseData(uint8_t* data, size_t len, size_t& currentBytePosition) = 0;
};
}  // namespace AsyncMqttClientInternals
