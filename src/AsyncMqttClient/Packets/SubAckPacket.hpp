#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class SubAckPacket : public Packet {
 public:
  explicit SubAckPacket(ParsingInformation* parsingInformation, OnSubAckInternalCallback callback);
  ~SubAckPacket() override;

  void parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) override;
  void parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnSubAckInternalCallback _callback;

  uint8_t _bytePosition;
  uint16_t _packetId;
};
}  // namespace AsyncMqttClientInternals
