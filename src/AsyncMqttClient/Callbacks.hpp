#pragma once

#include <functional>

#include "Error.hpp"
#include "Properties.h"
#include "QOS.hpp"


namespace AsyncMqttClientInternals {
// user callbacks
typedef std::function<void(bool sessionPresent, ConnectReason reason, const Properties& properties)> OnConnectUserCallback;
typedef std::function<void()> OnDisconnectUserCallback;
typedef std::function<void(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties)> OnSubscribeUserCallback;
typedef std::function<void(uint16_t packetId)> OnUnsubscribeUserCallback;
typedef std::function<void(char* topic, uint8_t* payload, MQTTQOS qos, bool dup, bool retain, const Properties& properties, size_t len, size_t index, size_t total)> OnMessageUserCallback;
typedef std::function<void(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties)> OnPublishUserCallback;
typedef std::function<void(Error err)> OnErrorUserCallback;

// internal callbacks
typedef std::function<void(bool sessionPresent, ConnectReason reason, const Properties& properties)> OnConnAckInternalCallback;
typedef std::function<void()> OnPingRespInternalCallback;
typedef std::function<void(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties)> OnSubUnsubAckInternalCallback;
typedef std::function<void(char* topic, uint8_t* payload, MQTTQOS qos, bool dup, bool retain, const Properties& properties, size_t len, size_t index, size_t total, uint16_t packetId)> OnMessageInternalCallback;
typedef std::function<void(uint16_t packetId, MQTTQOS qos, const Properties& properties)> OnPublishInternalCallback;
typedef std::function<void(uint16_t packetId, AckReason reason, const Properties& props)> OnAckInternalCallback;
}  // namespace AsyncMqttClientInternals
