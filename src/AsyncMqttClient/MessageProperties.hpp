#pragma once

#include "QOS.hpp"

struct AsyncMqttClientMessageProperties {
  MQTTQOS qos;
  bool dup;
  bool retain;
};
