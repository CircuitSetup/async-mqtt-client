#include "SubAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::SubAckPacket;

SubAckPacket::SubAckPacket(ParsingInformation* parsingInformation, OnSubAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _packetId(0) {
}

SubAckPacket::~SubAckPacket() = default;

void SubAckPacket::parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;
  uint8_t currentByte = data[(*currentBytePosition)++];
  if (_bytePosition++ == 0) {
    _packetId = currentByte << 8u;
  } else {
    _packetId |= currentByte;
    _parsingInformation->bufferState = BufferState::PAYLOAD;
  }
}

void SubAckPacket::parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;
  char status = data[(*currentBytePosition)++];

  /* switch (status) {
    case 0:
      Serial.println("Success QoS 0");
      break;
    case 1:
      Serial.println("Success QoS 1");
      break;
    case 2:
      Serial.println("Success QoS 2");
      break;
    case 0x80:
      Serial.println("Failure");
      break;
  } */

  _parsingInformation->bufferState = BufferState::NONE;
  _callback(_packetId, status);
}
