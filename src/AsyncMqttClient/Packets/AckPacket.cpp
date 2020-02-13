#include "AckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::AckPacket;

AckPacket::AckPacket(ParsingInformation* parsingInformation, OnAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _packetId(0)
, _reason(AckReason::SUCCESS)
, propertiesLength(0)
, propertyLengthRead(false) {
}

AckPacket::~AckPacket() = default;

void AckPacket::parseData(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;

  if (_bytePosition == POS_PACKET_ID_HIGH) {
    _packetId = data[*currentBytePosition] << 8u;
    _bytePosition++;
    (*currentBytePosition)++;
  } else if (_bytePosition == POS_PACKET_ID_LOW) {
    _packetId |= data[*currentBytePosition];
    _bytePosition++;
    (*currentBytePosition)++;
    if (_parsingInformation->size == 2) {
      _parsingInformation->bufferState = BufferState::NONE;
      Properties props{{}};
      _callback(_packetId, AckReason::SUCCESS, props);
    }
  } else if (_bytePosition == POS_REASON) {
    _reason = AsyncMqttClientInternals::AckReason(data[*currentBytePosition]);
    _bytePosition++;
    (*currentBytePosition)++;
  } else if (_bytePosition >= POS_PROPERTIES) {
    if (propertyLengthRead) {
      auto toCopy = std::min(len - *currentBytePosition, propertiesLength - properties.size());
      properties.insert(properties.end(), data + *currentBytePosition, data + *currentBytePosition + toCopy);

      _bytePosition += toCopy;
      (*currentBytePosition) += toCopy;

      if (propertiesLength == properties.size()) {
        _parsingInformation->bufferState = BufferState::NONE;
        Properties props(std::move(properties));
        _callback(_packetId, _reason, props);
      }
    } else {
      auto byteNr = _bytePosition - POS_PROPERTIES;
      auto shift = 7 * byteNr;
      propertiesLength |= (data[*currentBytePosition] & 0x7Fu) << shift;
      if ((data[*currentBytePosition] & 0x80u) == 0) {
        propertyLengthRead = true;
      }

      _bytePosition++;
      (*currentBytePosition)++;
    }
  }
}
