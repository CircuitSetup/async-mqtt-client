#include "AsyncMqttClient.hpp"
#include "AsyncMqttClient/Headers.hpp"
#include "AsyncMqttClient/Properties.h"
#include <ESP8266WiFiType.h>
#include <lwip/tcpbase.h>
#include <memory>

AsyncMqttClient::AsyncMqttClient()
: _connected(false)
, _lastClientActivity(0)
, _lastServerActivity(0)
, _lastPingRequestTime(0)
, _host(nullptr)
#if ASYNC_TCP_SSL_ENABLED
, _secure(false)
#endif
, _port(0)
, _keepAlive(15)
, _cleanSession(true)
, _clientId(nullptr)
, _username(nullptr)
, _password(nullptr)
, _willTopic(nullptr)
, _willPayload(nullptr)
, _willPayloadLength(0)
, _willQos(QOS0)
, _willRetain(false)
, _parsingInformation{}
, _currentParsedPacket(nullptr)
, _nextPacketId(1) {
  _client.onConnect([](void* obj, AsyncClient* c) { (static_cast<AsyncMqttClient*>(obj))->_onConnect(c); }, this);
  _client.onDisconnect([](void* obj, AsyncClient* c) { (static_cast<AsyncMqttClient*>(obj))->_onDisconnect(c); }, this);
  _client.onError([](void* obj, AsyncClient* c, err_t error) { (static_cast<AsyncMqttClient*>(obj))->_onError(c, error); }, this);
  _client.onTimeout([](void* obj, AsyncClient* c, uint32_t time) { (static_cast<AsyncMqttClient*>(obj))->_onTimeout(c, time); }, this);
  _client.onAck([](void* obj, AsyncClient* c, size_t len, uint32_t time) { (static_cast<AsyncMqttClient*>(obj))->_onAck(c, len, time); }, this);
  _client.onData([](void* obj, AsyncClient* c, void* data, size_t len) { (static_cast<AsyncMqttClient*>(obj))->_onData(c, static_cast<char*>(data), len); }, this);
  _client.onPoll([](void* obj, AsyncClient* c) { (static_cast<AsyncMqttClient*>(obj))->_onPoll(c); }, this);

#ifdef ESP32
  sprintf(_generatedClientId, "esp32-%06llx", ESP.getEfuseMac());
  _xSemaphore = xSemaphoreCreateMutex();
#elif defined(ESP8266)
  sprintf(_generatedClientId, "esp8266-%06x", ESP.getChipId());
#endif
  _clientId = _generatedClientId;
}

AsyncMqttClient::~AsyncMqttClient() {
#ifdef ESP32
  vSemaphoreDelete(_xSemaphore);
#endif
}

AsyncMqttClient& AsyncMqttClient::setKeepAlive(uint16_t keepAlive) {
  _keepAlive = keepAlive;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setClientId(const char* clientId) {
  _clientId = clientId;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setCleanSession(bool cleanSession) {
  _cleanSession = cleanSession;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setCredentials(const char* username, const char* password) {
  _username = username;
  _password = password;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setWill(const char* topic, MQTTQOS qos, bool retain, const char* payload, size_t length) {
  _willTopic = topic;
  _willQos = qos;
  _willRetain = retain;
  _willPayload = payload;
  _willPayloadLength = length;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setServer(const IPAddress& ip, uint16_t port) {
  _ip = ip;
  _host = nullptr;
  _port = port;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::setServer(const char* host, uint16_t port) {
  _host = host;
  _port = port;
  return *this;
}

#if ASYNC_TCP_SSL_ENABLED
AsyncMqttClient& AsyncMqttClient::setSecure(bool secure) {
  _secure = secure;
  return *this;
}

AsyncMqttClient& AsyncMqttClient::addServerFingerprint(const uint8_t* fingerprint) {
  std::array<uint8_t, SHA1_SIZE> newFingerprint;
  memcpy(newFingerprint.data(), fingerprint, SHA1_SIZE);
  _secureServerFingerprints.push_back(newFingerprint);
  return *this;
}
#endif

AsyncMqttClient& AsyncMqttClient::onConnect(const AsyncMqttClientInternals::OnConnectUserCallback& callback) {
  _onConnectUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onDisconnect(const AsyncMqttClientInternals::OnDisconnectUserCallback& callback) {
  _onDisconnectUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onSubscribe(const AsyncMqttClientInternals::OnSubscribeUserCallback& callback) {
  _onSubscribeUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onUnsubscribe(const AsyncMqttClientInternals::OnUnsubscribeUserCallback& callback) {
  _onUnsubscribeUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onMessage(const AsyncMqttClientInternals::OnMessageUserCallback& callback) {
  _onMessageUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onPublish(const AsyncMqttClientInternals::OnPublishUserCallback& callback) {
  _onPublishUserCallbacks.push_back(callback);
  return *this;
}

AsyncMqttClient& AsyncMqttClient::onError(const AsyncMqttClientInternals::OnErrorUserCallback& callback) {
  _onErrorUserCallbacks.push_back(callback);
  return *this;
}

void AsyncMqttClient::_clear() {
  _lastPingRequestTime = 0;
  _connected = false;
  _currentParsedPacket.reset();

  _pendingPubRels.clear();
  _pendingPubRels.shrink_to_fit();

  _toSendAcks.clear();
  _toSendAcks.shrink_to_fit();

  _nextPacketId = 1;
  _parsingInformation.bufferState = AsyncMqttClientInternals::BufferState::NONE;
  _parsingInformation.topicBuffer.clear();
  _parsingInformation.topicBuffer.shrink_to_fit();
}

/* TCP */
void AsyncMqttClient::_onConnect(AsyncClient* client) {
  (void)client;

#if ASYNC_TCP_SSL_ENABLED
  if (_secure && _secureServerFingerprints.size() > 0) {
    SSL* clientSsl = _client.getSSL();

    bool sslFoundFingerprint = false;
    for (std::array<uint8_t, SHA1_SIZE> fingerprint : _secureServerFingerprints) {
      if (ssl_match_fingerprint(clientSsl, fingerprint.data()) == SSL_OK) {
        sslFoundFingerprint = true;
        break;
      }
    }

    if (!sslFoundFingerprint) {
      _lastError = SERVER_FINGERPRINT_NOT_FOUND;
      _client.close(true);
      return;
    }
  }
#endif

  AsyncMqttClientInternals::ConnectHeader connectHeader{};
  connectHeader.flags.cleanStart = _cleanSession;
  connectHeader.flags.userNamePresent = _username != nullptr;
  connectHeader.flags.passwordPresent = _password != nullptr;
  connectHeader.flags.willPresent = _willTopic != nullptr;
  connectHeader.flags.willRetain = _willRetain;
  connectHeader.flags.willQOS = _willQos;
  connectHeader.flags.reserved = false;

  connectHeader.keepAliveSec = AsyncMqttClientInternals::Helpers::bigEndian(_keepAlive);

  //TODO: add properties
  uint8_t propertiesLength = 0;
  uint8_t properties[0];

  uint16_t clientIdLength = strlen(_clientId);
  uint8_t clientIdLengthBytes[2];
  clientIdLengthBytes[0] = clientIdLength >> 8u;
  clientIdLengthBytes[1] = clientIdLength & 0xFFu;

  // Optional fields
  uint8_t willPropertiesLength = 0;
  uint8_t willProperties[0];
  uint16_t willTopicLength = 0;
  uint8_t willTopicLengthBytes[2];
  uint16_t willPayloadLength = _willPayloadLength;
  uint8_t willPayloadLengthBytes[2];
  if (_willTopic != nullptr) {
    willTopicLength = strlen(_willTopic);
    willTopicLengthBytes[0] = willTopicLength >> 8u;
    willTopicLengthBytes[1] = willTopicLength & 0xFFu;

    if (_willPayload != nullptr && willPayloadLength == 0) willPayloadLength = strlen(_willPayload);

    willPayloadLengthBytes[0] = willPayloadLength >> 8u;
    willPayloadLengthBytes[1] = willPayloadLength & 0xFFu;
  }

  uint16_t usernameLength = 0;
  uint8_t usernameLengthBytes[2];
  if (_username != nullptr) {
    usernameLength = strlen(_username);
    usernameLengthBytes[0] = usernameLength >> 8u;
    usernameLengthBytes[1] = usernameLength & 0xFFu;
  }

  uint16_t passwordLength = 0;
  uint8_t passwordLengthBytes[2];
  if (_password != nullptr) {
    passwordLength = strlen(_password);
    passwordLengthBytes[0] = passwordLength >> 8u;
    passwordLengthBytes[1] = passwordLength & 0xFFu;
  }

  uint32_t remainingLength = sizeof(connectHeader) + AsyncMqttClientInternals::Helpers::variableByteIntegerSize(propertiesLength) + propertiesLength;
  remainingLength += 2 + clientIdLength; // client id
  if (_willTopic != nullptr) remainingLength += AsyncMqttClientInternals::Helpers::variableByteIntegerSize(willPropertiesLength) + willPropertiesLength + 2 + willTopicLength + 2 + willPayloadLength;
  if (_username != nullptr) remainingLength += 2 + usernameLength;
  if (_password != nullptr) remainingLength += 2 + passwordLength;

  AsyncMqttClientInternals::FixedHeader fixedHeader{AsyncMqttClientInternals::PacketType::CONNECT, AsyncMqttClientInternals::HeaderFlag.CONNECT_RESERVED, remainingLength};

  uint32_t neededSpace = fixedHeader.getHeaderSize();
  neededSpace += remainingLength;
  SEMAPHORE_TAKE();
  if (_client.space() < neededSpace) {
    for (const auto& callback : _onErrorUserCallbacks) callback(Error::CONNECT_NO_MEMORY);
    _client.close(true);
    SEMAPHORE_GIVE();
    return;
  }

  _client.add(fixedHeader.bytes, fixedHeader.getHeaderSize(), TCP_WRITE_FLAG_COPY);
  // Using a sendbuffer to fix bug setwill on SSL not working

  _client.add(reinterpret_cast<char*>(&connectHeader), sizeof(connectHeader), TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<char*>(&propertiesLength), sizeof(propertiesLength), TCP_WRITE_FLAG_COPY);
  if (propertiesLength != 0)
    _client.add(reinterpret_cast<char*>(&properties), sizeof(properties), TCP_WRITE_FLAG_COPY);

  _client.add(reinterpret_cast<const char*>(clientIdLengthBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(_clientId, clientIdLength, TCP_WRITE_FLAG_COPY);
  if (_willTopic != nullptr) {
    _client.add(reinterpret_cast<char*>(&willPropertiesLength), sizeof(willPropertiesLength), TCP_WRITE_FLAG_COPY);
    if (willPropertiesLength != 0)
      _client.add(reinterpret_cast<char*>(&willProperties), sizeof(willProperties), TCP_WRITE_FLAG_COPY);
    _client.add(reinterpret_cast<const char*>(willTopicLengthBytes), 2, TCP_WRITE_FLAG_COPY);
    _client.add(_willTopic, willTopicLength);

    _client.add(reinterpret_cast<const char*>(willPayloadLengthBytes), 2, TCP_WRITE_FLAG_COPY);
    if (_willPayload != nullptr) _client.add(_willPayload, willPayloadLength);
  }
  if (_username != nullptr) {
    _client.add(reinterpret_cast<const char*>(usernameLengthBytes), 2, TCP_WRITE_FLAG_COPY);
    _client.add(_username, usernameLength);
  }
  if (_password != nullptr) {
    _client.add(reinterpret_cast<const char*>(passwordLengthBytes), 2, TCP_WRITE_FLAG_COPY);
    _client.add(_password, passwordLength);
  }
  if (!_client.send()) {
    for (const auto& callback : _onErrorUserCallbacks) callback(Error::CONNECTION_FAILED);
  }
  _lastClientActivity = millis();
  SEMAPHORE_GIVE();
}

void AsyncMqttClient::_onDisconnect(AsyncClient* client) {
  (void)client;

  _clear();

  for (const auto& callback : _onDisconnectUserCallbacks) callback();
}

void AsyncMqttClient::_onError(AsyncClient* client, err_t error) {
  (void)client;
  Error err;
  switch (error) {
    case ERR_MEM:        err = Error::TCP_OUT_OF_MEMORY; break;
    case ERR_BUF:        err = Error::TCP_BUFFER_ERROR; break;
    case ERR_TIMEOUT:    err = Error::TCP_TIMEOUT; break;
    case ERR_RTE:        err = Error::TCP_ROUTING; break;
    case ERR_ABRT:       err = Error::CONNECTION_ABORTED; break;
    case ERR_RST:        err = Error::CONNECTION_RESET; break;
    case ERR_CLSD:       err = Error::CONNECTION_CLOSED; break;
    case ERR_IF:         err = Error::TCP_LOW_LEVEL; break;
    case -55:            err = Error::DNS_ERROR; break;

    case ERR_ISCONN:
    case ERR_USE:
    case ERR_ARG:
    case ERR_CONN:
    case ERR_WOULDBLOCK:
    case ERR_VAL:
    case ERR_INPROGRESS:
    case ERR_OK:
    default:             err = Error::TCP_UNKNOWN; break;
  }

  for (const auto& callback : _onErrorUserCallbacks) callback(err);
}

void AsyncMqttClient::_onTimeout(AsyncClient* client, uint32_t time) {
  (void)client;
  (void)time;
  // disconnection will be handled by ping/pong management
}

void AsyncMqttClient::_onAck(AsyncClient* client, size_t len, uint32_t time) {
  (void)client;
  (void)len;
  (void)time;
}

void AsyncMqttClient::_onData(AsyncClient* client, char* cdata, size_t len) {
  auto* data = reinterpret_cast<uint8_t*>(cdata);
  (void)client;
  size_t currentBytePosition = 0;
  uint8_t currentByte;
  do {
#ifdef MQTT_DEBUG
    Serial.print("loop begin state: ");
    Serial.print(static_cast<unsigned char>(_parsingInformation.bufferState));
    Serial.print(" packet: ");
    Serial.println(static_cast<unsigned char>(_parsingInformation.type));
#endif

    switch (_parsingInformation.bufferState) {
      case AsyncMqttClientInternals::BufferState::NONE:
        currentByte = data[currentBytePosition++];
        _parsingInformation.type = static_cast<AsyncMqttClientInternals::PacketType>(currentByte >> 4u);
        _parsingInformation.flags = currentByte & 0x7Fu;
        _parsingInformation.size = 0;
        _parsingInformation.bufferState = AsyncMqttClientInternals::BufferState::REMAINING_LENGTH;
        _lastServerActivity = millis();
        switch (_parsingInformation.type) {
          case AsyncMqttClientInternals::PacketType::CONNACK:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::ConnAckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onConnAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PINGRESP:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::PingRespPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onPingResp, this)));
            break;
          case AsyncMqttClientInternals::PacketType::SUBACK:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::SubUnsubAckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onSubAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::UNSUBACK:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::SubUnsubAckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onUnsubAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PUBLISH:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::PublishPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9, std::placeholders::_10), std::bind(&AsyncMqttClient::_onPublish, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PUBREL:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::AckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onPubRel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PUBACK:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::AckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onPubAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PUBREC:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::AckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onPubRec, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          case AsyncMqttClientInternals::PacketType::PUBCOMP:
            _currentParsedPacket.reset(new AsyncMqttClientInternals::AckPacket(&_parsingInformation, std::bind(&AsyncMqttClient::_onPubComp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
            break;
          default:
            break;
        }
        break;
      case AsyncMqttClientInternals::BufferState::REMAINING_LENGTH:
        if (_parsingInformation.readVbi(_parsingInformation.size, data, len, currentBytePosition)) {
          if (_parsingInformation.size > 0) {
            _parsingInformation.bufferState = AsyncMqttClientInternals::BufferState::DATA;
          } else {
            // PINGRESP is a special case where it has no variable header, so the packet ends right here
            _parsingInformation.bufferState = AsyncMqttClientInternals::BufferState::NONE;
            _onPingResp();
          }
        }
        break;
      case AsyncMqttClientInternals::BufferState::DATA:
        _currentParsedPacket->parseData(data, len, currentBytePosition);
        break;
      default:
        currentBytePosition = len;
    }
  } while (currentBytePosition != len);
}

void AsyncMqttClient::_onPoll(AsyncClient* client) {
  (void)client;
  if (!_connected) return;

  // if there is too much time the client has sent a ping request without a response, disconnect client to avoid half open connections
  if (_lastPingRequestTime != 0 && (millis() - _lastPingRequestTime) >= (_keepAlive * 1000 * 2)) {
#ifdef MQTT_DEBUG
    Serial.println("Timeout disconnect");
#endif
    disconnect();
    return;
  // send ping to ensure the server will receive at least one message inside keepalive window
  } else if (_lastPingRequestTime == 0 && (millis() - _lastClientActivity) >= (_keepAlive * static_cast<uint32_t >(1000 * 0.7))) {
#ifdef MQTT_DEBUG
    Serial.println("Send ca ping");
#endif
    _sendPing();

  // send ping to verify if the server is still there (ensure this is not a half connection)
  } else if (_connected && _lastPingRequestTime == 0 && (millis() - _lastServerActivity) >= (_keepAlive * static_cast<uint32_t >(1000 * 0.7))) {
#ifdef MQTT_DEBUG
    Serial.println("Send sa ping");
#endif
    _sendPing();
  }

  // handle to send ack packets
  _sendAcks();
}

/* MQTT */
void AsyncMqttClient::_onPingResp() {
  _currentParsedPacket.reset();
  _lastPingRequestTime = 0;
}

void AsyncMqttClient::_onConnAck(bool sessionPresent, AsyncMqttClientInternals::ConnectReason reason, const Properties& properties) {
  _currentParsedPacket.reset();

  _connected = true;

  for (const auto& callback : _onConnectUserCallbacks) callback(sessionPresent, reason, properties);
}

void AsyncMqttClient::_onSubAck(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties) {
  _currentParsedPacket.reset();

  for (const auto& callback : _onSubscribeUserCallbacks) callback(packetId, reason, properties);
}

void AsyncMqttClient::_onUnsubAck(uint16_t packetId, AsyncMqttClientInternals::SubAckReason reason, const Properties& properties) {
  _currentParsedPacket.reset();

  for (const auto& callback : _onUnsubscribeUserCallbacks) callback(packetId);
}

void AsyncMqttClient::_onMessage(char* topic, uint8_t* payload, MQTTQOS qos, bool dup, bool retain, const Properties& properties, size_t len, size_t index, size_t total, uint16_t packetId) {
  bool notifyPublish = true;

  if (qos == 2) {
    for (AsyncMqttClientInternals::PendingPubRel pendingPubRel : _pendingPubRels) {
      if (pendingPubRel.packetId == packetId) {
        notifyPublish = false;
        break;
      }
    }
  }

  if (notifyPublish) {
    for (const auto& callback : _onMessageUserCallbacks) callback(topic, payload, qos, dup, retain, properties, len, index, total);
  }
}

void AsyncMqttClient::_onPublish(uint16_t packetId, MQTTQOS qos, const Properties& properties) {
  _currentParsedPacket.reset();

  if (qos == QOS0) {
    return;
  }

  if (qos == QOS1) {
    _toSendAcks.emplace_back(AsyncMqttClientInternals::PacketType::PUBACK, AsyncMqttClientInternals::HeaderFlag.PUBACK_RESERVED, AsyncMqttClientInternals::Helpers::bigEndian(packetId));
  } else {
    _toSendAcks.emplace_back(AsyncMqttClientInternals::PacketType::PUBREC, AsyncMqttClientInternals::HeaderFlag.PUBREC_RESERVED, AsyncMqttClientInternals::Helpers::bigEndian(packetId));

    bool pubRelAwaiting = false;
    for (AsyncMqttClientInternals::PendingPubRel pendingPubRel : _pendingPubRels) {
      if (pendingPubRel.packetId == packetId) {
        pubRelAwaiting = true;
        break;
      }
    }

    if (!pubRelAwaiting) {
      AsyncMqttClientInternals::PendingPubRel pendingPubRel{.packetId = packetId};
      _pendingPubRels.push_back(pendingPubRel);
    }
  }

  _sendAcks();
}

void AsyncMqttClient::_onPubRel(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties) {
  _currentParsedPacket.reset();

  _toSendAcks.emplace_back(AsyncMqttClientInternals::PacketType::PUBCOMP, AsyncMqttClientInternals::HeaderFlag.PUBCOMP_RESERVED, AsyncMqttClientInternals::Helpers::bigEndian(packetId));

  for (size_t i = 0; i < _pendingPubRels.size(); i++) {
    if (_pendingPubRels[i].packetId == packetId) {
      _pendingPubRels.erase(_pendingPubRels.begin() + i);
      _pendingPubRels.shrink_to_fit();
    }
  }

  _sendAcks();
}

void AsyncMqttClient::_onPubAck(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties) {
  for (const auto& callback : _onPublishUserCallbacks) callback(packetId, reason, properties);
  _currentParsedPacket.reset();
}

void AsyncMqttClient::_onPubRec(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties) {
  _currentParsedPacket.reset();

  for (const auto& callback : _onPublishUserCallbacks) callback(packetId, reason, properties);

  if (reason == AsyncMqttClientInternals::AckReason::SUCCESS || reason == AsyncMqttClientInternals::AckReason::NO_MATCHING_SUBSCRIBERS) {
    _toSendAcks.emplace_back(AsyncMqttClientInternals::PacketType::PUBREL, AsyncMqttClientInternals::HeaderFlag.PUBREL_RESERVED, AsyncMqttClientInternals::Helpers::bigEndian(packetId));

    _sendAcks();
  }
}

void AsyncMqttClient::_onPubComp(uint16_t packetId, AsyncMqttClientInternals::AckReason reason, const Properties& properties) {
  _currentParsedPacket.reset();
}

bool AsyncMqttClient::_sendPing() {
  AsyncMqttClientInternals::FixedHeader header{AsyncMqttClientInternals::PacketType::PINGREQ, AsyncMqttClientInternals::HeaderFlag.PINGREQ_RESERVED, 0};

  SEMAPHORE_TAKE(false);
  if (_client.space() < 2) {
    SEMAPHORE_GIVE();
    return false;
  }

#ifdef MQTT_DEBUG
  Serial.println("send ping");
#endif

  _client.add(header.bytes, 2, TCP_WRITE_FLAG_COPY);
  _client.send();
  _lastPingRequestTime = _lastClientActivity = millis();

  SEMAPHORE_GIVE();
  return true;
}

void AsyncMqttClient::_sendAcks() {
  uint8_t neededAckSpace = sizeof(AsyncMqttClientInternals::AckRetPacket);

  SEMAPHORE_TAKE();

  while (_client.space() >= neededAckSpace && !_toSendAcks.empty()) {
    _client.add(reinterpret_cast<const char*>(&_toSendAcks.front()), neededAckSpace, TCP_WRITE_FLAG_COPY);
    _client.send();
    _toSendAcks.pop_front();
    _lastClientActivity = millis();
  }
  SEMAPHORE_GIVE();
}

bool AsyncMqttClient::_sendDisconnect() {
  if (!_connected) return true;

  const uint8_t neededSpace = 2;

  SEMAPHORE_TAKE(false);

  if (_client.space() < neededSpace) { SEMAPHORE_GIVE(); return false; }

  AsyncMqttClientInternals::FixedHeader fixedHeader{AsyncMqttClientInternals::PacketType::DISCONNECT, AsyncMqttClientInternals::HeaderFlag.DISCONNECT_RESERVED, 0};

  _client.add(fixedHeader.bytes, 2, TCP_WRITE_FLAG_COPY);
  _client.send();
  _client.close(true);

  SEMAPHORE_GIVE();
  return true;
}

uint16_t AsyncMqttClient::_getNextPacketId() {
  uint16_t nextPacketId = _nextPacketId;

  if (_nextPacketId == 65535) _nextPacketId = 0; // 0 is forbidden
  _nextPacketId++;

  return nextPacketId;
}

bool AsyncMqttClient::connected() const {
  return _connected;
}

Error AsyncMqttClient::connect() {
  if (_connected) return Error::ALREADY_CONNECTED;

  bool connectResult;
#if ASYNC_TCP_SSL_ENABLED
  if (_useIp) {
    connectResult = _client.connect(_ip, _port, _secure);
  } else {
    connectResult = _client.connect(_host, _port, _secure);
  }
#else
  if (_host) {
    connectResult = _client.connect(_host, _port);
  } else {
    connectResult = _client.connect(_ip, _port);
  }
#endif
  if (connectResult)
    return Error::OK;
  else
    return Error::CONNECTION_FAILED;
}

void AsyncMqttClient::disconnect(bool force) {
  if (!_connected) return;

  if (force) {
    _client.close(true);
  } else {
    _sendDisconnect();
  }
}

uint16_t AsyncMqttClient::subscribe(const char* topic, MQTTQOS qos, bool nonLocal, bool retainAsPublished, AsyncMqttClientInternals::RetainFlag retainFlag, const Properties* const properties) {
  if (!_connected) return 0;

  uint16_t topicLength = strlen(topic);

  uint32_t propertiesLength = properties == nullptr ? 0 : properties->buffer.size();
  std::array<uint8_t, 4> propertiesLengthBytes{};
  uint8_t propertiesLengthLength = AsyncMqttClientInternals::Helpers::encodeVariableByteInteger(propertiesLength, propertiesLengthBytes.data());

  uint32_t remainingLength = 2 + propertiesLengthLength + propertiesLength + 2 + topicLength + 1;

  AsyncMqttClientInternals::FixedHeader fixedHeader{AsyncMqttClientInternals::PacketType::SUBSCRIBE, AsyncMqttClientInternals::HeaderFlag.SUBSCRIBE_RESERVED, remainingLength};

  size_t neededSpace = fixedHeader.getHeaderSize() + remainingLength;

  SEMAPHORE_TAKE(0);
  if (_client.space() < neededSpace) { SEMAPHORE_GIVE(); return 0; }

  uint16_t packetId = _getNextPacketId();
  uint8_t packetIdBytes[2];
  packetIdBytes[0] = packetId >> 8u;
  packetIdBytes[1] = packetId & 0xFFu;

  uint8_t topicLengthBytes[2];
  topicLengthBytes[0] = topicLength >> 8u;
  topicLengthBytes[1] = topicLength & 0xFFu;

  AsyncMqttClientInternals::SubscriptionOption option {qos, nonLocal, retainAsPublished, retainFlag, 0};

  _client.add(fixedHeader.bytes, fixedHeader.getHeaderSize(), TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(packetIdBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(propertiesLengthBytes.data()), propertiesLengthLength, TCP_WRITE_FLAG_COPY);
  if (properties != nullptr && propertiesLength > 0)
    _client.add(reinterpret_cast<const char*>(properties->buffer.data()), propertiesLength, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(topicLengthBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(topic, topicLength, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(&option), 1, TCP_WRITE_FLAG_COPY);
  _client.send();
  _lastClientActivity = millis();

  SEMAPHORE_GIVE();
  return packetId;
}

uint16_t AsyncMqttClient::unsubscribe(const char* topic, const Properties* const properties) {
  if (!_connected) return 0;

  uint16_t topicLength = strlen(topic);

  uint32_t propertiesLength = properties == nullptr ? 0 : properties->buffer.size();
  std::array<uint8_t, 4> propertiesLengthBytes{};
  uint8_t propertiesLengthLength = AsyncMqttClientInternals::Helpers::encodeVariableByteInteger(propertiesLength, propertiesLengthBytes.data());

  size_t remainingLength = 2 + propertiesLengthLength + propertiesLength + 2 + topicLength;

  AsyncMqttClientInternals::FixedHeader fixedHeader{AsyncMqttClientInternals::PacketType::UNSUBSCRIBE, AsyncMqttClientInternals::HeaderFlag.UNSUBSCRIBE_RESERVED, remainingLength};
  size_t neededSpace = fixedHeader.getHeaderSize();
  neededSpace += remainingLength;

  SEMAPHORE_TAKE(0);
  if (_client.space() < neededSpace) { SEMAPHORE_GIVE(); return 0; }

  uint16_t packetId = _getNextPacketId();
  uint8_t packetIdBytes[2];
  packetIdBytes[0] = packetId >> 8u;
  packetIdBytes[1] = packetId & 0xFFu;

  uint8_t topicLengthBytes[2];
  topicLengthBytes[0] = topicLength >> 8u;
  topicLengthBytes[1] = topicLength & 0xFFu;

  _client.add(fixedHeader.bytes, fixedHeader.getHeaderSize(), TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(packetIdBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(propertiesLengthBytes.data()), propertiesLengthLength, TCP_WRITE_FLAG_COPY);
  if (properties != nullptr && propertiesLength > 0)
    _client.add(reinterpret_cast<const char*>(properties->buffer.data()), propertiesLength, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(topicLengthBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(topic, topicLength, TCP_WRITE_FLAG_COPY);
  _client.send();
  _lastClientActivity = millis();

  SEMAPHORE_GIVE();
  return packetId;
}

uint16_t AsyncMqttClient::publish(const char* topic, MQTTQOS qos, bool retain, const char* payload, size_t length, const Properties* const properties, bool dup, uint16_t message_id) {
  if (!_connected) return 0;

  uint16_t topicLength = topic != nullptr ? strlen(topic) : 0;
  uint8_t topicLengthBytes[2];
  topicLengthBytes[0] = topicLength >> 8u;
  topicLengthBytes[1] = topicLength & 0xFFu;

  uint32_t payloadLength = length;
  if (payload != nullptr && payloadLength == 0) payloadLength = strlen(payload);

  uint32_t propertiesLength = properties == nullptr ? 0 : properties->buffer.size();
  std::array<uint8_t, 4> propertiesLengthBytes{};
  uint8_t propertiesLengthLength = AsyncMqttClientInternals::Helpers::encodeVariableByteInteger(propertiesLength, propertiesLengthBytes.data());

  uint32_t remainingLength = 2 + topicLength + propertiesLengthLength + propertiesLength + payloadLength;
  if (qos != 0) remainingLength += 2;

  uint8_t flags = (dup ? AsyncMqttClientInternals::HeaderFlag.PUBLISH_DUP : 0u) | (retain ? AsyncMqttClientInternals::HeaderFlag.PUBLISH_RETAIN : 0u) | (qos == QOS0 ? AsyncMqttClientInternals::HeaderFlag.PUBLISH_QOS0 : 0u) | (qos == QOS1 ? AsyncMqttClientInternals::HeaderFlag.PUBLISH_QOS1 : 0u) | (qos == QOS2 ? AsyncMqttClientInternals::HeaderFlag.PUBLISH_QOS2 : 0u);

  AsyncMqttClientInternals::FixedHeader fixedHeader{AsyncMqttClientInternals::PacketType::PUBLISH, flags, remainingLength};

  size_t neededSpace = fixedHeader.getHeaderSize() + remainingLength;

  SEMAPHORE_TAKE(0);
  if (_client.space() < neededSpace) { SEMAPHORE_GIVE(); return 0; }

  uint16_t packetId = 0;
  uint8_t packetIdBytes[2];
  if (qos != 0) {
    if (dup && message_id > 0) {
      packetId = message_id;
    } else {
      packetId = _getNextPacketId();
    }

    packetIdBytes[0] = packetId >> 8u;
    packetIdBytes[1] = packetId & 0xFFu;
  }

  _client.add(fixedHeader.bytes, fixedHeader.getHeaderSize(), TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(topicLengthBytes), 2, TCP_WRITE_FLAG_COPY);
  if (topicLength > 0) _client.add(topic, topicLength, TCP_WRITE_FLAG_COPY);
  if (qos != 0) _client.add(reinterpret_cast<const char*>(packetIdBytes), 2, TCP_WRITE_FLAG_COPY);
  _client.add(reinterpret_cast<const char*>(propertiesLengthBytes.data()), propertiesLengthLength, TCP_WRITE_FLAG_COPY);
  if (propertiesLength > 0) _client.add(reinterpret_cast<const char*>(properties->buffer.data()), propertiesLength, TCP_WRITE_FLAG_COPY);
  if (payload != nullptr) _client.add(payload, payloadLength, TCP_WRITE_FLAG_COPY);
  _client.send();
  _lastClientActivity = millis();

  SEMAPHORE_GIVE();
  if (qos != 0) {
    return packetId;
  } else {
    return 1;
  }
}

const char* AsyncMqttClient::getClientId() {
  return _clientId;
}