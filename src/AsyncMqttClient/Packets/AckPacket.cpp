#include "AckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::AckPacket;

AckPacket::AckPacket(ParsingInformation* parsingInformation, OnAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, state(ParsingState::PACKET_IDENTIFIER)
, _packetId(0)
, _reason(AckReason::SUCCESS)
, propertiesLength(0) {
}

AckPacket::~AckPacket() = default;

void AckPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  switch (state) {
    case ParsingState::PACKET_IDENTIFIER:
      if (_parsingInformation->read(_packetId, data, len, currentBytePosition))
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
        _callback(_packetId, _reason, props);
      }
      break;
  }
}
