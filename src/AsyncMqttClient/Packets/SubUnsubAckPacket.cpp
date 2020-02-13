#include "SubUnsubAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::SubUnsubAckPacket;

SubUnsubAckPacket::SubUnsubAckPacket(ParsingInformation* parsingInformation, OnSubUnsubAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _packetId(0)
, propertiesLength(0)
, state(ParsingState::PACKET_IDENTIFIER) {
}

SubUnsubAckPacket::~SubUnsubAckPacket() = default;

void SubUnsubAckPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  switch (state) {
    case ParsingState::PACKET_IDENTIFIER:
      if (_parsingInformation->read(_packetId, data, len, currentBytePosition))
        state = ParsingState::PROPERTIES_LENGTH;
      break;
    case ParsingState::PROPERTIES_LENGTH:
      if (_parsingInformation->readVbi(propertiesLength, data, len, currentBytePosition))
        state = ParsingState::PROPERTIES;
      break;
    case ParsingState::PROPERTIES: {
      auto toCopy = std::min(len - currentBytePosition, propertiesLength - properties.size());
      properties.insert(properties.end(), data + currentBytePosition, data + currentBytePosition + toCopy);

      currentBytePosition += toCopy;

      if (propertiesLength == properties.size()) {
        state = ParsingState::PAYLOAD;
      }
      break;
    }
    case ParsingState::PAYLOAD:
      auto reason = static_cast<SubAckReason>(data[currentBytePosition++]);

      _parsingInformation->bufferState = BufferState::NONE;
      Properties props{std::move(properties)};
      _callback(_packetId, reason, props);
      break;
  }
}
