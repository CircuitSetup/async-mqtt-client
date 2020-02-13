#pragma once

#include "Flags.hpp"
#include "Headers.hpp"

namespace AsyncMqttClientInternals {
struct PendingPubRel {
  uint16_t packetId;
};

struct AckRetPacket {
    AckRetPacket(PacketType type, uint8_t flags, uint16_t packetId) :
            flags(flags), type(type), remainingSize(2), packetId(packetId) {}

    uint8_t flags : 4;
    PacketType type : 4;
    uint8_t remainingSize; // normally 2
    uint16_t packetId;
//    AckReason reason;
//    uint8_t propertiesSize;
} __attribute__((packed));

}  // namespace AsyncMqttClientInternals
