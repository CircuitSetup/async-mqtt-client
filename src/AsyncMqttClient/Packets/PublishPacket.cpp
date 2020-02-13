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
, propertiesLength(0)
, state(ParsingState::TOPIC_NAME_LENGTH) {
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
  uint8_t currentByte = data[currentBytePosition++];
  switch (state) {
    case ParsingState::TOPIC_NAME_LENGTH:
      if (!_parsingInformation->read(_topicLength, data, len, currentBytePosition))
        return;

      if (_topicLength <= MQTT_MAX_TOPIC_LENGTH) {
        _parsingInformation->topicBuffer.reserve(_topicLength + 1);
        if (_parsingInformation->topicBuffer.data() == nullptr) _ignore = true;
      } else {
        _ignore = true;
      }
      if (_topicLength == 0) {
        state = _qos == MQTTQOS::QOS0 ? ParsingState::PROPERTIES_LENGTH : ParsingState::PACKET_IDENTIFIER;
      } else {
        state = ParsingState::TOPIC_NAME;
      }
      break;
    case ParsingState::TOPIC_NAME:
      if (!_ignore) _parsingInformation->topicBuffer.push_back(currentByte);
      if (_parsingInformation->topicBuffer.size() == _topicLength) {
        _parsingInformation->topicBuffer.push_back('\0');
        state = _qos == MQTTQOS::QOS0 ? ParsingState::PROPERTIES_LENGTH : ParsingState::PACKET_IDENTIFIER;
      }
      break;
    case ParsingState::PACKET_IDENTIFIER:
      if (_parsingInformation->read(_packetId, data, len, currentBytePosition))
        state = ParsingState::PROPERTIES_LENGTH;
      break;
    case ParsingState::PROPERTIES_LENGTH:
      if (!_parsingInformation->readVbi(propertiesLength, data, len, currentBytePosition))
        return;

      if (propertiesLength == 0) {
        _preparePayloadHandling();
      } else {
        state = ParsingState::PROPERTIES;
        if (!_ignore) props.buffer.reserve(propertiesLength);
      }
      break;
    case ParsingState::PROPERTIES:
      if (!_ignore && props.buffer.data() != nullptr) props.buffer.push_back(currentByte);
      if (props.buffer.size() == propertiesLength) {
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

