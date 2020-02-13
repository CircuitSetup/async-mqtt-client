#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class SubUnsubAckPacket : public Packet {
  constexpr static size_t POS_PACKET_ID_HIGH = 0;
  constexpr static size_t POS_PACKET_ID_LOW  = 1;
  constexpr static size_t POS_PROPERTIES     = 2;
 public:
  explicit SubUnsubAckPacket(ParsingInformation* parsingInformation, OnSubUnsubAckInternalCallback callback);
  ~SubUnsubAckPacket() override;

  void parseData(uint8_t* data, size_t len, size_t* currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnSubUnsubAckInternalCallback _callback;

  uint8_t _bytePosition;
  uint16_t _packetId;

  uint32_t propertiesLength;
  bool propertyLengthRead;
  std::vector<uint8_t> properties;
};
}  // namespace AsyncMqttClientInternals
