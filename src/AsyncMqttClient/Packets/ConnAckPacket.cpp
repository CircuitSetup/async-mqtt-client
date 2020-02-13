#include "ConnAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::ConnAckPacket;

ConnAckPacket::ConnAckPacket(ParsingInformation* parsingInformation, OnConnAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _sessionPresent(false)
, _reason(AsyncMqttClientInternals::ConnectReason::SUCCESS)
, propertiesLength(0)
, propertyLengthRead(false) {
}

ConnAckPacket::~ConnAckPacket() = default;

void ConnAckPacket::parseData(uint8_t* data, size_t len, size_t& currentBytePosition) {
  (void)len;
  if (_bytePosition == POS_FLAGS) {
    _sessionPresent = data[currentBytePosition] & 0x80u;
    _bytePosition++;
    currentBytePosition++;
  } else if (_bytePosition == POS_REASON) {
    _reason = AsyncMqttClientInternals::ConnectReason(data[currentBytePosition]);
    _bytePosition++;
  bool propertyLengthRead;
    currentBytePosition++;
  } else if (_bytePosition >= POS_PROPERTIES) {
    if (propertyLengthRead) {
      auto toCopy = std::min(len - currentBytePosition, propertiesLength - properties.size());
      properties.insert(properties.end(), data + currentBytePosition, data + currentBytePosition + toCopy);

      _bytePosition += toCopy;
      currentBytePosition += toCopy;

      if (propertiesLength == properties.size()) {
        _parsingInformation->bufferState = BufferState::NONE;
        Properties props(std::move(properties));
        _callback(_sessionPresent, _reason, props);
      }
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
  }
}
