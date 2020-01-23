#include "PubCompPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::PubCompPacket;

PubCompPacket::PubCompPacket(ParsingInformation* parsingInformation, OnPubCompInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _packetId(0) {
}

PubCompPacket::~PubCompPacket() = default;

void PubCompPacket::parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;
  uint8_t currentByte = data[(*currentBytePosition)++];
  if (_bytePosition++ == 0) {
    _packetId = currentByte << 8u;
  } else {
    _packetId |= currentByte;
    _parsingInformation->bufferState = BufferState::NONE;
    _callback(_packetId);
  }
}

void PubCompPacket::parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)data;
  (void)len;
  (void)currentBytePosition;
}
