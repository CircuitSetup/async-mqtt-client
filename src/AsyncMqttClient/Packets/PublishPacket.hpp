#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../Flags.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class PublishPacket : public Packet {
 public:
  explicit PublishPacket(ParsingInformation* parsingInformation, OnMessageInternalCallback dataCallback, OnPublishInternalCallback completeCallback);
  ~PublishPacket() override;

  void parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) override;
  void parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnMessageInternalCallback _dataCallback;
  OnPublishInternalCallback _completeCallback;

  void _preparePayloadHandling(uint32_t payloadLength);

  bool _dup;
  MQTTQOS _qos;
  bool _retain;

  uint8_t _bytePosition;
  uint16_t _topicLength;
  bool _ignore;
  uint16_t _packetId;
  uint32_t _payloadLength;
  uint32_t _payloadBytesRead;
};
}  // namespace AsyncMqttClientInternals
