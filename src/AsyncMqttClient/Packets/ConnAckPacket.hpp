#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class ConnAckPacket : public Packet {
  enum class ParsingState : uint8_t {
    FLAGS,
    REASON,
    PROPERTIES_LENGTH,
    PROPERTIES
  };

 public:
  explicit ConnAckPacket(ParsingInformation* parsingInformation, OnConnAckInternalCallback callback);
  ~ConnAckPacket() override;

  void parseData(uint8_t* data, size_t len, size_t& currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnConnAckInternalCallback _callback;

  uint8_t _flags;
  AsyncMqttClientInternals::ConnectReason _reason;

  uint32_t propertiesLength;
  std::vector<uint8_t> properties;

  ParsingState state;
};
}  // namespace AsyncMqttClientInternals
