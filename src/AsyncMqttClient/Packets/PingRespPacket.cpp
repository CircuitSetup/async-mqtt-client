#include "PingRespPacket.hpp"

#include <utility>

using AsyncMqttClientInternals::PingRespPacket;

PingRespPacket::PingRespPacket(ParsingInformation* parsingInformation, OnPingRespInternalCallback callback)
: _parsingInformation(parsingInformation)
, _callback(std::move(callback)) {
}

PingRespPacket::~PingRespPacket() = default;

void PingRespPacket::parseData(uint8_t* data, size_t len, size_t* currentBytePosition) {
  (void)data;
  (void)len;
  (void)currentBytePosition;
}
