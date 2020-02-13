#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class SubUnsubAckPacket : public Packet {
  enum class ParsingState : uint8_t {
    PACKET_IDENTIFIER,
    PROPERTIES_LENGTH,
    PROPERTIES,
    PAYLOAD
  };
 public:
  explicit SubUnsubAckPacket(ParsingInformation* parsingInformation, OnSubUnsubAckInternalCallback callback);
  ~SubUnsubAckPacket() override;

  void parseData(uint8_t* data, size_t len, size_t& currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnSubUnsubAckInternalCallback _callback;
  uint16_t _packetId;

  uint32_t propertiesLength;
  std::vector<uint8_t> properties;
  ParsingState state;
};
}  // namespace AsyncMqttClientInternals
