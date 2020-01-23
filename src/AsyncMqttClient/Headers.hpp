#pragma once

#include "Helpers.hpp"

namespace AsyncMqttClientInternals {
struct ConnectHeader {
  uint16_t protocolNameLength = Helpers::bigEndian(4);
  uint8_t protocolName[4] = {'M', 'Q', 'T', 'T'};
  uint8_t protocolVersion = 4;
  struct Flags {
    bool reserved : 1;
    bool cleanStart : 1;
    bool willPresent : 1;
    uint8_t willQOS : 2;
    bool willRetain : 1;
    bool passwordPresent : 1;
    bool userNamePresent : 1;
  } flags = {false, false, false, 0, false, false, false};
  uint16_t keepAliveSec = 0;

} __attribute__((packed));

struct FixedHeader {
  FixedHeader(PacketType type, uint8_t flags, uint32_t size) : flags(flags), type(type) { Helpers::encodeRemainingLength(size, packetSizeVbi.data()); }

  uint32_t getSize() const { return Helpers::decodeRemainingLength(packetSizeVbi.data()); }

  uint8_t getHeaderSize() const {
    uint8_t vbiSize = 1;
    for (const auto& byte : packetSizeVbi) {
      if (!(byte & 0x80u)) break;
      vbiSize++;
    }

    return 1 + vbiSize;
  }

  union {
    struct {
      uint8_t flags : 4;
      // bug fixed in https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61414
      PacketType type : 4;
      std::array<uint8_t, 4> packetSizeVbi;
    } __attribute__((packed));
    char bytes[5];
  };
};

} // namespace AsyncMqttClientInternals