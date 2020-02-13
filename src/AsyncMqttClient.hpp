#pragma once

#include <functional>
#include <deque>
#include <vector>

#include "Arduino.h"

#ifdef ESP32
#include <AsyncTCP.h>
#include <freertos/semphr.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>

#else
#error Platform not supported
#endif

#if ASYNC_TCP_SSL_ENABLED
#include <tcp_axtls.h>
#define SHA1_SIZE 20
#endif

#include "AsyncMqttClient/Flags.hpp"
#include "AsyncMqttClient/ParsingInformation.hpp"
#include "AsyncMqttClient/Helpers.hpp"
#include "AsyncMqttClient/Callbacks.hpp"
#include "AsyncMqttClient/Error.hpp"
#include "AsyncMqttClient/Storage.hpp"
#include "AsyncMqttClient/QOS.hpp"

#include "AsyncMqttClient/Packets/AckPacket.hpp"
#include "AsyncMqttClient/Packets/ConnAckPacket.hpp"
#include "AsyncMqttClient/Packets/Packet.hpp"
#include "AsyncMqttClient/Packets/PingRespPacket.hpp"
#include "AsyncMqttClient/Packets/PublishPacket.hpp"
#include "AsyncMqttClient/Packets/SubUnsubAckPacket.hpp"

#if ESP32
#define SEMAPHORE_TAKE(X) if (xSemaphoreTake(_xSemaphore, 1000 / portTICK_PERIOD_MS) != pdTRUE) { return X; }  // Waits max 1000ms
#define SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore);
#elif defined(ESP8266)
#define SEMAPHORE_TAKE(X) void()
#define SEMAPHORE_GIVE() void()
#endif

class AsyncMqttClient {
 public:
  AsyncMqttClient();
  ~AsyncMqttClient();

  AsyncMqttClient& setKeepAlive(uint16_t keepAlive);
  AsyncMqttClient& setClientId(const char* clientId);
  AsyncMqttClient& setCleanSession(bool cleanSession);
  AsyncMqttClient& setCredentials(const char* username, const char* password = nullptr);
  AsyncMqttClient& setWill(const char* topic, MQTTQOS qos, bool retain, const char* payload = nullptr, size_t length = 0);
  AsyncMqttClient& setServer(const IPAddress& ip, uint16_t port);
  AsyncMqttClient& setServer(const char* host, uint16_t port);
#if ASYNC_TCP_SSL_ENABLED
  AsyncMqttClient& setSecure(bool secure);
  AsyncMqttClient& addServerFingerprint(const uint8_t* fingerprint);
#endif

  AsyncMqttClient& onConnect(const AsyncMqttClientInternals::OnConnectUserCallback& callback);
  AsyncMqttClient& onDisconnect(const AsyncMqttClientInternals::OnDisconnectUserCallback& callback);
  AsyncMqttClient& onSubscribe(const AsyncMqttClientInternals::OnSubscribeUserCallback& callback);
  AsyncMqttClient& onUnsubscribe(const AsyncMqttClientInternals::OnUnsubscribeUserCallback& callback);
  AsyncMqttClient& onMessage(const AsyncMqttClientInternals::OnMessageUserCallback& callback);
  AsyncMqttClient& onPublish(const AsyncMqttClientInternals::OnPublishUserCallback& callback);
  AsyncMqttClient& onError(const AsyncMqttClientInternals::OnErrorUserCallback& callback);

  bool connected() const;
  Error connect();
  void disconnect(bool force = false);
  uint16_t subscribe(const char* topic, MQTTQOS qos = MQTTQOS::QOS0, bool nonLocal = false, bool retainAsPublished = false, AsyncMqttClientInternals::RetainFlag retainFlag = AsyncMqttClientInternals::RetainFlag::ALWAYS_SEND_RETAIN, const Properties* const properties = nullptr);
  uint16_t unsubscribe(const char* topic, const Properties* properties = nullptr);
  uint16_t publish(const char* topic, MQTTQOS qos, bool retain, const char* payload = nullptr, size_t length = 0, const Properties* properties = nullptr, bool dup = false, uint16_t message_id = 0);

  const char* getClientId();

 private:
  AsyncClient _client;

  bool _connected;
  uint32_t _lastClientActivity;
  uint32_t _lastServerActivity;
  uint32_t _lastPingRequestTime;

  char _generatedClientId[18 + 1];  // esp8266-abc123 and esp32-abcdef123456 
  IPAddress _ip;
  const char* _host;
#if ASYNC_TCP_SSL_ENABLED
  bool _secure;
#endif
  uint16_t _port;
  uint16_t _keepAlive;
  bool _cleanSession;
  const char* _clientId;
  const char* _username;
  const char* _password;
  const char* _willTopic;
  const char* _willPayload;
  uint16_t _willPayloadLength;
  MQTTQOS _willQos;
  bool _willRetain;

#if ASYNC_TCP_SSL_ENABLED
  std::vector<std::array<uint8_t, SHA1_SIZE>> _secureServerFingerprints;
#endif

  std::vector<AsyncMqttClientInternals::OnConnectUserCallback> _onConnectUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnDisconnectUserCallback> _onDisconnectUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnSubscribeUserCallback> _onSubscribeUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnUnsubscribeUserCallback> _onUnsubscribeUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnMessageUserCallback> _onMessageUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnPublishUserCallback> _onPublishUserCallbacks;
  std::vector<AsyncMqttClientInternals::OnErrorUserCallback> _onErrorUserCallbacks;

  AsyncMqttClientInternals::ParsingInformation _parsingInformation;
  std::unique_ptr<AsyncMqttClientInternals::Packet> _currentParsedPacket;
  uint8_t _remainingLengthBufferPosition;
  uint8_t _remainingLengthBuffer[4];

  uint16_t _nextPacketId;

  std::vector<AsyncMqttClientInternals::PendingPubRel> _pendingPubRels;

  std::deque<AsyncMqttClientInternals::AckRetPacket> _toSendAcks;

#ifdef ESP32
  SemaphoreHandle_t _xSemaphore = nullptr;
#endif

  void _clear();

  // TCP
  void _onConnect(AsyncClient* client);
  void _onDisconnect(AsyncClient* client);
  void _onError(AsyncClient* client, err_t error);
  void _onTimeout(AsyncClient* client, uint32_t time);
  void _onAck(AsyncClient* client, size_t len, uint32_t time);
  void _onData(AsyncClient* client, char* data, size_t len);
  void _onPoll(AsyncClient* client);

  // MQTT
  void _onPingResp();
  void _onConnAck(bool sessionPresent, AsyncMqttClientInternals::ConnectReason reason, const Properties& properties);
  void _onSubAck(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties);
  void _onUnsubAck(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties);
  void _onMessage(char* topic, uint8_t* payload, MQTTQOS qos, bool dup, bool retain, const Properties& properties, size_t len, size_t index, size_t total, uint16_t packetId);
  void _onPublish(uint16_t packetId, MQTTQOS qos, const Properties& properties);
  void _onPubRel(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties);
  void _onPubAck(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties);
  void _onPubRec(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties);
  void _onPubComp(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties);

  bool _sendPing();
  void _sendAcks();
  bool _sendDisconnect();

  uint16_t _getNextPacketId();
};
