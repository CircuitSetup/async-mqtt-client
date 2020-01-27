#pragma once

#include <functional>

#include "Error.hpp"
#include "MessageProperties.hpp"
#include "QOS.hpp"


namespace AsyncMqttClientInternals {
// user callbacks
typedef std::function<void(bool sessionPresent)> OnConnectUserCallback;
typedef std::function<void()> OnDisconnectUserCallback;
typedef std::function<void(uint16_t packetId, char status)> OnSubscribeUserCallback;
typedef std::function<void(uint16_t packetId)> OnUnsubscribeUserCallback;
typedef std::function<void(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)> OnMessageUserCallback;
typedef std::function<void(uint16_t packetId)> OnPublishUserCallback;
typedef std::function<void(Error err)> OnErrorUserCallback;

// internal callbacks
typedef std::function<void(bool sessionPresent, uint8_t connectReturnCode)> OnConnAckInternalCallback;
typedef std::function<void()> OnPingRespInternalCallback;
typedef std::function<void(uint16_t packetId, char status)> OnSubAckInternalCallback;
typedef std::function<void(uint16_t packetId)> OnUnsubAckInternalCallback;
typedef std::function<void(char* topic, char* payload, MQTTQOS qos, bool dup, bool retain, size_t len, size_t index, size_t total, uint16_t packetId)> OnMessageInternalCallback;
typedef std::function<void(uint16_t packetId, MQTTQOS qos)> OnPublishInternalCallback;
typedef std::function<void(uint16_t packetId)> OnPubRelInternalCallback;
typedef std::function<void(uint16_t packetId)> OnPubAckInternalCallback;
typedef std::function<void(uint16_t packetId)> OnPubRecInternalCallback;
typedef std::function<void(uint16_t packetId)> OnPubCompInternalCallback;
}  // namespace AsyncMqttClientInternals
