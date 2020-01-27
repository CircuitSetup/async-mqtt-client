#pragma once

#include "Flags.hpp"
#include "Headers.hpp"

namespace AsyncMqttClientInternals {
struct PendingPubRel {
  uint16_t packetId;
};

struct AckPacket {
    AckPacket(PacketType type, uint8_t flags, uint16_t packetId) :
            flags(flags), type(type), payloadSize(2), packetId(packetId) {}

    uint8_t flags : 4;
    PacketType type : 4;
    uint8_t payloadSize; // normally 2
    uint16_t packetId;
} __attribute__((packed));

}  // namespace AsyncMqttClientInternals
