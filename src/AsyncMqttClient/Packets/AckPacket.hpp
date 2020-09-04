#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class AckPacket : public Packet {
  enum class ParsingState : uint8_t {
    PACKET_IDENTIFIER,
    REASON,
    PROPERTIES_LENGTH,
    PROPERTIES
  };

 public:
  explicit AckPacket(ParsingInformation* parsingInformation, OnAckInternalCallback callback);
  ~AckPacket() override;

  void parseData(uint8_t* data, size_t len, size_t& currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnAckInternalCallback _callback;

  uint16_t _packetId;
  AckReason _reason;

  uint32_t propertiesLength;
  std::vector<uint8_t> properties;

  ParsingState state;
};
}  // namespace AsyncMqttClientInternals
