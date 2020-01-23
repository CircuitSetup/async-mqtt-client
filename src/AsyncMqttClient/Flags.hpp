#pragma once

namespace AsyncMqttClientInternals {
enum class PacketType : uint8_t {
  RESERVED    = 0,
  CONNECT     = 1,
  CONNACK     = 2,
  PUBLISH     = 3,
  PUBACK      = 4,
  PUBREC      = 5,
  PUBREL      = 6,
  PUBCOMP     = 7,
  SUBSCRIBE   = 8,
  SUBACK      = 9,
  UNSUBSCRIBE = 10,
  UNSUBACK    = 11,
  PINGREQ     = 12,
  PINGRESP    = 13,
  DISCONNECT  = 14,
};

constexpr struct {
  const uint8_t CONNECT_RESERVED     = 0x00;
  const uint8_t CONNACK_RESERVED     = 0x00;
  const uint8_t PUBLISH_DUP          = 0x08;
  const uint8_t PUBLISH_QOS0         = 0x00;
  const uint8_t PUBLISH_QOS1         = 0x02;
  const uint8_t PUBLISH_QOS2         = 0x04;
  const uint8_t PUBLISH_QOSRESERVED  = 0x06;
  const uint8_t PUBLISH_RETAIN       = 0x01;
  const uint8_t PUBACK_RESERVED      = 0x00;
  const uint8_t PUBREC_RESERVED      = 0x00;
  const uint8_t PUBREL_RESERVED      = 0x02;
  const uint8_t PUBCOMP_RESERVED     = 0x00;
  const uint8_t SUBSCRIBE_RESERVED   = 0x02;
  const uint8_t SUBACK_RESERVED      = 0x00;
  const uint8_t UNSUBSCRIBE_RESERVED = 0x02;
  const uint8_t UNSUBACK_RESERVED    = 0x00;
  const uint8_t PINGREQ_RESERVED     = 0x00;
  const uint8_t PINGRESP_RESERVED    = 0x00;
  const uint8_t DISCONNECT_RESERVED  = 0x00;
  const uint8_t RESERVED2_RESERVED   = 0x00;
} HeaderFlag;

}  // namespace AsyncMqttClientInternals
