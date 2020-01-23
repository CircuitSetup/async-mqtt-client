#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class ConnAckPacket : public Packet {
 public:
  explicit ConnAckPacket(ParsingInformation* parsingInformation, OnConnAckInternalCallback callback);
  ~ConnAckPacket() override;

  void parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) override;
  void parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnConnAckInternalCallback _callback;

  uint8_t _bytePosition;
  bool _sessionPresent;
  uint8_t _connectReturnCode;
};
}  // namespace AsyncMqttClientInternals
