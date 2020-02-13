#pragma once

#include "Arduino.h"
#include "Packet.hpp"
#include "../Flags.hpp"
#include "../ParsingInformation.hpp"
#include "../Callbacks.hpp"

namespace AsyncMqttClientInternals {
class PublishPacket : public Packet {
  enum class ParsingState : uint8_t {
    TOPIC_NAME_LENGTH_HIGH,
    TOPIC_NAME_LENGTH_LOW,
    TOPIC_NAME,
    PACKET_IDENTIFIER_HIGH,
    PACKET_IDENTIFIER_LOW,
    PROPERTIES_LENGTH,
    PROPERTIES,
    PAYLOAD
  };

 public:
  explicit PublishPacket(ParsingInformation* parsingInformation, OnMessageInternalCallback dataCallback, OnPublishInternalCallback completeCallback);
  ~PublishPacket() override;

  void parseData(uint8_t* data, size_t len, size_t& currentBytePosition) override;

 private:
  ParsingInformation* _parsingInformation;
  OnMessageInternalCallback _dataCallback;
  OnPublishInternalCallback _completeCallback;

  void _preparePayloadHandling();

  bool _dup;
  MQTTQOS _qos;
  bool _retain;

  ParsingState state = ParsingState::TOPIC_NAME_LENGTH_HIGH;
  uint32_t bytePosition;

  uint16_t _topicLength;
  bool _ignore;
  uint16_t _packetId;

  uint32_t propertiesLength;
  Properties props{{}};

  uint32_t _payloadLength;
  uint32_t _payloadBytesRead;
};
}  // namespace AsyncMqttClientInternals
