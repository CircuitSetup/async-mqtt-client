#include "SubUnsubAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::SubUnsubAckPacket;

SubUnsubAckPacket::SubUnsubAckPacket(ParsingInformation* parsingInformation, OnSubUnsubAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _packetId(0)
, propertiesLength(0)
, propertyLengthRead(false) {
}

SubUnsubAckPacket::~SubUnsubAckPacket() = default;

void SubUnsubAckPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  (void)len;

  if (_bytePosition == POS_PACKET_ID_HIGH) {
    _packetId = data[currentBytePosition] << 8u;
    _bytePosition++;
    currentBytePosition++;
  } else if (_bytePosition == POS_PACKET_ID_LOW) {
    _packetId |= data[currentBytePosition];
    _bytePosition++;
    currentBytePosition++;
  } else if (_bytePosition >= POS_PROPERTIES && !(propertyLengthRead && propertiesLength == properties.size())) {
    if (propertyLengthRead) {
      auto toCopy = std::min(len - currentBytePosition, propertiesLength - properties.size());
      properties.insert(properties.end(), data + currentBytePosition, data + currentBytePosition + toCopy);

      _bytePosition += toCopy;
      currentBytePosition += toCopy;
    } else {
      auto byteNr = _bytePosition - POS_PROPERTIES;
      auto shift = 7 * byteNr;
      propertiesLength |= (data[currentBytePosition] & 0x7Fu) << shift;
      if ((data[currentBytePosition] & 0x80u) == 0) {
        propertyLengthRead = true;
      }

      _bytePosition++;
      currentBytePosition++;
    }
  } else {
    auto reason = static_cast<SubAckReason>(data[currentBytePosition++]);

    _parsingInformation->bufferState = BufferState::NONE;
    Properties props{std::move(properties)};
    _callback(_packetId, reason, props);
  }
}
