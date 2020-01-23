#include <utility>

#include "../Config.hpp"

#include "PublishPacket.hpp"

using AsyncMqttClientInternals::PublishPacket;

PublishPacket::PublishPacket(ParsingInformation* parsingInformation, OnMessageInternalCallback dataCallback, OnPublishInternalCallback completeCallback) : _parsingInformation(parsingInformation), _dataCallback(std::move(dataCallback)), _completeCallback(std::move(completeCallback)), _dup(false), _qos(QOS0), _retain(false), _bytePosition(0), _topicLength(0), _ignore(false), _packetId(0), _payloadLength(0), _payloadBytesRead(0) {
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

void PublishPacket::parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;
  uint8_t currentByte = data[(*currentBytePosition)++];
  if (_bytePosition == 0) {
    _topicLength = currentByte << 8u;
  } else if (_bytePosition == 1) {
    _topicLength |= currentByte;
    if (_topicLength > MQTT_MAX_TOPIC_LENGTH) {
      _ignore = true;
    } else {
      _parsingInformation->topicBuffer.reserve(_topicLength + 1);
      if (_parsingInformation->topicBuffer.data() == nullptr) // malloc failed
        _ignore = true;
    }
  } else if (_bytePosition >= 2 && _bytePosition < 2 + _topicLength) {
    // Starting from here, _ignore might be true
    if (!_ignore) _parsingInformation->topicBuffer.push_back(currentByte);
    if (_bytePosition == 2 + _topicLength - 1) { // we seen the last topic byte
      _parsingInformation->topicBuffer.push_back(0);
      if (_qos == 0) {
        _preparePayloadHandling(_parsingInformation->size - (_bytePosition + 1));
        return;
      }
    }
  } else if (_bytePosition == 2 + _topicLength) {
    _packetId = currentByte << 8u;
  } else {
    _packetId |= currentByte;
    _preparePayloadHandling(_parsingInformation->size - (_bytePosition + 1));
  }
  _bytePosition++;
}

void PublishPacket::_preparePayloadHandling(uint32_t payloadLength) {
  _payloadLength = payloadLength;
  if (payloadLength == 0) {
    _parsingInformation->bufferState = BufferState::NONE;
    if (!_ignore) {
      _dataCallback(reinterpret_cast<char*>(_parsingInformation->topicBuffer.data()), nullptr, _qos, _dup, _retain, 0, 0, 0, _packetId);
      _completeCallback(_packetId, _qos);
    }
  } else {
    _parsingInformation->bufferState = BufferState::PAYLOAD;
  }
}

void PublishPacket::parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) {
  size_t remainToRead = len - (*currentBytePosition);
  if (_payloadBytesRead + remainToRead > _payloadLength) remainToRead = _payloadLength - _payloadBytesRead;

  if (!_ignore) _dataCallback(reinterpret_cast<char*>(_parsingInformation->topicBuffer.data()), reinterpret_cast<char*>(data + (*currentBytePosition)), _qos, _dup, _retain, remainToRead, _payloadBytesRead, _payloadLength, _packetId);
  _payloadBytesRead += remainToRead;
  (*currentBytePosition) += remainToRead;

  if (_payloadBytesRead == _payloadLength) {
    _parsingInformation->bufferState = BufferState::NONE;
    if (!_ignore) _completeCallback(_packetId, _qos);
  }
}
