#include "ConnAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::ConnAckPacket;

ConnAckPacket::ConnAckPacket(ParsingInformation* parsingInformation, OnConnAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _flags(0)
, _reason(AsyncMqttClientInternals::ConnectReason::SUCCESS)
, propertiesLength(0)
, state(ParsingState::FLAGS) {
}

ConnAckPacket::~ConnAckPacket() = default;

void ConnAckPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  switch (state) {
    case ParsingState::FLAGS:
      if (_parsingInformation->read(_flags, data, len, currentBytePosition))
        state = ParsingState::REASON;
      break;
    case ParsingState::REASON:
      if (_parsingInformation->read(*reinterpret_cast<uint8_t*>(&_reason), data, len, currentBytePosition))
        state = ParsingState::PROPERTIES_LENGTH;
      break;
    case ParsingState::PROPERTIES_LENGTH:
      if (_parsingInformation->readVbi(propertiesLength, data, len, currentBytePosition))
        state = ParsingState::PROPERTIES;
      break;
    case ParsingState::PROPERTIES:
      auto toCopy = std::min(len - currentBytePosition, propertiesLength - properties.size());
      properties.insert(properties.end(), data + currentBytePosition, data + currentBytePosition + toCopy);

      currentBytePosition += toCopy;

      if (propertiesLength == properties.size()) {
        _parsingInformation->bufferState = BufferState::NONE;
        Properties props(std::move(properties));
        _callback(_flags & 0x80, _reason, props);
      }
      break;
  }
}
