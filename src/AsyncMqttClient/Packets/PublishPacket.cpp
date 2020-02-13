#include <utility>

#include "../Config.hpp"

#include "PublishPacket.hpp"

using AsyncMqttClientInternals::PublishPacket;

PublishPacket::PublishPacket(ParsingInformation* parsingInformation, OnMessageInternalCallback dataCallback, OnPublishInternalCallback completeCallback)
: _parsingInformation(parsingInformation)
, _dataCallback(std::move(dataCallback))
, _completeCallback(std::move(completeCallback))
, _dup(false)
, _qos(QOS0)
, _retain(false)
, _topicLength(0)
, _ignore(false)
, _packetId(0)
, _payloadLength(0)
, _payloadBytesRead(0)
, bytePosition(0)
, propertiesLength(0) {
  _dup = _parsingInformation->flags & HeaderFlag.PUBLISH_DUP;
  _retain = _parsingInformation->flags & HeaderFlag.PUBLISH_RETAIN;
  uint8_t qosByte = (_parsingInformation->flags & 0x06u);
  switch (qosByte) {
    case HeaderFlag.PUBLISH_QOS0:
      _qos = QOS0;
      break;
    case HeaderFlag.PUBLISH_QOS1:
      _qos = QOS1;
      break;
    case HeaderFlag.PUBLISH_QOS2:
      _qos = QOS2;
      break;
  }
}

PublishPacket::~PublishPacket() = default;

void PublishPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  (void)len;

  uint8_t currentByte = data[currentBytePosition++];
  switch (state) {
    case ParsingState::TOPIC_NAME_LENGTH_HIGH:
      _topicLength = currentByte << 8u;
      state = ParsingState::TOPIC_NAME_LENGTH_LOW;
      break;
    case ParsingState::TOPIC_NAME_LENGTH_LOW:
      _topicLength |= currentByte;

      if (_topicLength <= MQTT_MAX_TOPIC_LENGTH) {
        _parsingInformation->topicBuffer.reserve(_topicLength + 1);
        if (_parsingInformation->topicBuffer.data() == nullptr) _ignore = true;
      } else {
        _ignore = true;
      }
      bytePosition = 0;

      if (_topicLength == 0) {
        state = _qos == MQTTQOS::QOS0 ? ParsingState::PROPERTIES_LENGTH : ParsingState::PACKET_IDENTIFIER_HIGH;
      } else {
        state = ParsingState::TOPIC_NAME;
      }
      break;
    case ParsingState::TOPIC_NAME:
      if (!_ignore) _parsingInformation->topicBuffer.push_back(currentByte);
      if (++bytePosition == _topicLength) {
        bytePosition = 0;
        _parsingInformation->topicBuffer.push_back('\0');
        state = _qos == MQTTQOS::QOS0 ? ParsingState::PROPERTIES_LENGTH : ParsingState::PACKET_IDENTIFIER_HIGH;
      }
      break;
    case ParsingState::PACKET_IDENTIFIER_HIGH:
      _packetId = currentByte << 8u;
      state = ParsingState::PACKET_IDENTIFIER_LOW;
      break;
    case ParsingState::PACKET_IDENTIFIER_LOW:
      _packetId |= currentByte;
      state = ParsingState::PROPERTIES_LENGTH;
      break;
    case ParsingState::PROPERTIES_LENGTH:
      propertiesLength |= (currentByte & 0x7Fu) << bytePosition * 7;
      if ((currentByte & 0x80u) != 0) {
        bytePosition++;
      } else {
        bytePosition = 0;
        if (propertiesLength == 0) {
          _preparePayloadHandling();
        } else {
          state = ParsingState::PROPERTIES;
          if (!_ignore) props.buffer.reserve(propertiesLength);
        }
      }
      break;
    case ParsingState::PROPERTIES:
      if (!_ignore && props.buffer.data() != nullptr) props.buffer.push_back(currentByte);
      if (++bytePosition == propertiesLength) {
        _preparePayloadHandling();
      }
      break;
    case ParsingState::PAYLOAD:
      size_t remainToRead = len - currentBytePosition;
      if (_payloadBytesRead + remainToRead > _payloadLength) remainToRead = _payloadLength - _payloadBytesRead;

      if (!_ignore) {
        _dataCallback(reinterpret_cast<char*>(_parsingInformation->topicBuffer.data()), data + currentBytePosition, _qos, _dup, _retain, props, remainToRead, _payloadBytesRead, _payloadLength, _packetId);
      }
      _payloadBytesRead += remainToRead;
      currentBytePosition += remainToRead;

      if (_payloadBytesRead == _payloadLength) {
        _parsingInformation->bufferState = BufferState::NONE;
        _parsingInformation->topicBuffer.clear();
        if (!_ignore) _completeCallback(_packetId, _qos, props);
      }
  }
}

void PublishPacket::_preparePayloadHandling() {
  uint32_t payloadLength = _parsingInformation->size -
                         (2 /*topic name length*/  + _topicLength +
                          (_qos == MQTTQOS::QOS0 ? 0 : 2 /*packet id*/) +
                          AsyncMqttClientInternals::Helpers::variableByteIntegerSize(propertiesLength) + propertiesLength);
  _payloadLength = payloadLength;

  if (payloadLength == 0) {
    _parsingInformation->bufferState = BufferState::NONE;
    if (!_ignore) {
      _dataCallback(reinterpret_cast<char*>(_parsingInformation->topicBuffer.data()), nullptr, _qos, _dup, _retain, props, 0, 0, 0, _packetId);
      _completeCallback(_packetId, _qos, props);
    }
  } else {
   state = ParsingState::PAYLOAD;
  }
}

