#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class UnsubAckPacket : public Packet {
 public:
  explicit UnsubAckPacket(ParsingInformation* parsingInformation, OnUnsubAckInternalCallback callback);
  ~UnsubAckPacket() override;

  void parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) override;
  void parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnUnsubAckInternalCallback _callback;

  uint8_t _bytePosition;
  uint16_t _packetId;
};
}  // namespace AsyncMqttClientInternals
