#include "ConnAckPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::ConnAckPacket;

ConnAckPacket::ConnAckPacket(ParsingInformation* parsingInformation, OnConnAckInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback))
, _bytePosition(0)
, _sessionPresent(false)
, _connectReturnCode(0) {
}

ConnAckPacket::~ConnAckPacket() = default;

void ConnAckPacket::parseVariableHeader(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)len;
  uint8_t currentByte = data[(*currentBytePosition)++];
  if (_bytePosition++ == 0) {
    _sessionPresent = currentByte & 0x80u;
  } else {
    _connectReturnCode = currentByte;
    _parsingInformation->bufferState = BufferState::NONE;
    _callback(_sessionPresent, _connectReturnCode);
  }
}

void ConnAckPacket::parsePayload(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)data;
  (void)len;
  (void)currentBytePosition;
}
