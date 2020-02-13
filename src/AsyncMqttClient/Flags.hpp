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
enum class ConnectReason : uint8_t {
  SUCCESS = 0x00,
  UNSPECIFIED_ERROR = 0x80,
  MALFORMED_PACKET = 0x81,
  PROTOCOL_ERROR = 0x82,
  IMPLEMENTATION_SPECIFIC_ERROR = 0x83,
  UNSUPPORTED_PROTOCOL_VERSION = 0x84,
  CLIENT_IDENTIFIER_NOT_VALID = 0x85,
  BAD_USERNAME_OR_PASSWORD = 0x86,
  NOT_AUTHORIZED = 0x87,
  SERVER_UNAVAILIABLE = 0x88,
  SERVER_BUSY = 0x89,
  BANNED = 0x8A,
  BAD_AUTHENTICATION_METHOD = 0x8C,
  WILL_TOPIC_NAME_INVALID = 0x90,
  PACKET_TOO_LARGE = 0x95,
  QUOTA_EXCEEDED = 0x97,
  PAYLOAD_FORMAT_INVALID = 0x99,
  RETAIN_NOT_SUPPORTED = 0x9A,
  QOS_NOT_SUPPORTED = 0x9B,
  USE_ANOTHER_SERVER = 0x9C,
  SERVER_MOVED = 0x9D,
  CONNECTION_RATE_EXCEEDED = 0x9F,
};

enum class AckReason : uint8_t {
  SUCCESS = 0x00,
  NO_MATCHING_SUBSCRIBERS = 0x10,
  UNSPECIFIED_ERROR = 0x80,
  IMPLEMENTATION_SPECIFIC_ERROR = 0x83,
  NOT_AUTHORIZED = 0x87,
  TOPIC_NAME_INVALID = 0x90,
  PACKET_IDENTIFIER_IN_USE = 0x91,
  PACKET_IDENTIFIER_NOT_FOUND = 0x92,
  QUOTA_EXCEEDED = 0x97,
  PAYLOAD_FORMAT_INVALID = 0x99,
};

enum class RetainFlag : uint8_t {
  ALWAYS_SEND_RETAIN = 0x0,
  SEND_RETAIN_IF_SUBSCRIPTION_IS_NEW = 0x1,
  SEND_NO_RETAINED_MESSAGES = 0x2,
};

enum class SubAckReason : uint8_t {
  GRANTED_QOS0                            = 0x00,
  GRANTED_QOS1                            = 0x01,
  GRANTED_QOS2                            = 0x02,
  UNSPECIFIED_ERROR                       = 0x80,
  IMPLEMENTATION_SPECIFIC_ERROR           = 0x83,
  NOT_AUTHORIZED                          = 0x87,
  TOPIC_FILTER_INVALID                    = 0x8F,
  PACKET_IDENTIFIER_IN_USE                = 0x91,
  QUOTA_EXCEEDED                          = 0x97,
  SHARED_SUBSCRIPTIONS_NOT_SUPPORTED      = 0x9E,
  SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED  = 0xA1,
  WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED    = 0xA2,

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
