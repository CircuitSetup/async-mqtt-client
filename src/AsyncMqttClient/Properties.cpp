//
// Created by mknjc on 08.01.20.
//

#include "Properties.h"
#include "Helpers.hpp"

std::pair<Properties::const_itr_t, Properties::const_itr_t> Properties::getProperty(Properties::Type t, uint32_t offset) const {
    auto startItr = buffer.cbegin();

    uint32_t foundCount = 0;

    while(startItr != buffer.cend()) {
        auto currentType = static_cast<Type>(*startItr++);
        auto propPair = parseProperty(currentType, startItr);

        if (propPair.first == propPair.second)
            return std::make_pair(startItr, startItr);

        if (currentType == t) {
            if (foundCount < offset) {
                foundCount++;
            } else {
                return propPair;
            }
        }
        startItr = propPair.second;
    }

    return std::make_pair(startItr, startItr);
}

template <typename Itr>
Itr safeNext(Itr i, Itr end, uint32_t diff = 1) {
    if (std::distance(i, end) < diff) {
        return i;
    } else {
        return std::next(i, diff);
    }
}

std::pair<Properties::const_itr_t, Properties::const_itr_t> Properties::parseProperty(Type t, Properties::const_itr_t start) const {
    switch (getDatatype(t)) {
        case Datatype::BYTE:
            return std::make_pair(start, safeNext(start, buffer.cend()));
        case Datatype::TWO_BYTE_INTEGER:
            return std::make_pair(start, safeNext(start, buffer.cend(), 2));
        case Datatype::FOUR_BYTE_INTEGER:
            return std::make_pair(start, safeNext(start, buffer.cend(), 4));
        case Datatype::VARIABLE_BYTE_INTEGER: {
            auto end = start;

            do {
                if (buffer.cend() == end)
                    return std::make_pair(start, start);
                std::advance(end, 1);
            } while (*end & 0x80);
            return std::make_pair(start, std::next(end));
        }
        case Datatype::UTF8_STRING:
        case Datatype::DATA: {
            auto maxRemaining = std::distance(start, buffer.cend());
            if (maxRemaining < 2) {
                return std::make_pair(start, start);
            }
            uint16_t dataSize = *(start++) << 8u | *(start++);

            if (dataSize + 2 < maxRemaining) {
                return std::make_pair(start, start);
            }
            return std::make_pair(start, std::next(start, dataSize));
        }
        case Datatype::UTF8_STRING_PAIR: {
            auto maxRemaining = std::distance(start, buffer.cend());
            if (maxRemaining < 4) {
                return std::make_pair(start, start);
            }
            auto itr = start;
            uint16_t str1Size = *(itr++) << 8u | *(itr++);
            if (maxRemaining < 4 + str1Size) {
                return std::make_pair(start, start);
            }
            std::advance(itr, str1Size);
            uint16_t str2Size = *(itr++) << 8u | *(itr++);
            if (maxRemaining < 4 + str1Size + str2Size) {
                return std::make_pair(start, start);
            }
            return std::make_pair(start, std::next(itr, str2Size));
        }
      case Datatype::UNKNOWN:
      default:
        return std::make_pair(start, start);
    }

}
tl::optional<uint8_t> Properties::getByteProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::BYTE)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  return *property.first;
}
tl::optional<uint16_t> Properties::getTwoByteProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::TWO_BYTE_INTEGER)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  auto itr = property.first;
  return *(itr++) << 8u | *(itr++);
}
tl::optional<uint32_t> Properties::getFourByteProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::FOUR_BYTE_INTEGER)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  auto itr = property.first;
  return *(itr++) << 24u | *(itr++) << 16u | *(itr++) << 8u | *(itr++);
}
tl::optional<uint32_t> Properties::getVbiProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::VARIABLE_BYTE_INTEGER)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  return AsyncMqttClientInternals::Helpers::decodeVariableByteInteger(&*property.first);
}
tl::optional<String> Properties::getStringProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::UTF8_STRING)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  auto itr = property.first;
  uint16_t size = *(itr++) << 8u | *(itr++);
  String ret;
  if (ret.concat(reinterpret_cast<const char*>(&*itr), size) == 0) {
    return {};
  }

  return std::move(ret);
}
tl::optional<std::pair<Properties::const_itr_t, Properties::const_itr_t>> Properties::getDataProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::DATA)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  return std::make_pair(std::next(property.first, 2), property.second);
}
tl::optional<std::pair<String, String>> Properties::getStringPairProperty(Properties::Type t, uint32_t offset) const {
  if (getDatatype(t) != Datatype::UTF8_STRING_PAIR)
    return {};
  auto property = getProperty(t, offset);
  if (property.first == property.second)
    return {};

  auto itr = property.first;
  uint16_t size = *(itr++) << 8u | *(itr++);
  String key;
  if (key.concat(reinterpret_cast<const char*>(&*itr), size) == 0) {
    return {};
  }
  std::advance(itr, size);

  size = *(itr++) << 8u | *(itr++);
  String val;
  if (val.concat(reinterpret_cast<const char*>(&*itr), size) == 0) {
    return {};
  }

  return std::make_pair(std::move(key), std::move(val));
}
bool Properties::addByteProperty(Properties::Type t, uint8_t b) {
  if (getDatatype(t) != Datatype::BYTE)
    return false;
  buffer.reserve(buffer.size() + 1 + 1);
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));
  buffer.push_back(b);
  return true;
}
bool Properties::addTwoByteProperty(Properties::Type t, uint16_t s) {
  if (getDatatype(t) != Datatype::TWO_BYTE_INTEGER)
    return false;
  buffer.reserve(buffer.size() + 1 + 2);
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));
  buffer.push_back(s >> 8u);
  buffer.push_back(s & 0xFFu);
  return true;
}
bool Properties::addFourByteProperty(Properties::Type t, uint32_t i) {
  if (getDatatype(t) != Datatype::FOUR_BYTE_INTEGER)
    return false;
  buffer.reserve(buffer.size() + 1 + 4);
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));
  buffer.push_back((i >> 24u) & 0xFFu);
  buffer.push_back((i >> 16u) & 0xFFu);
  buffer.push_back((i >> 8u) & 0xFFu);
  buffer.push_back(i & 0xFFu);
  return true;
}
bool Properties::addVbiProperty(Properties::Type t, uint32_t vbi) {
  if (getDatatype(t) != Datatype::VARIABLE_BYTE_INTEGER)
    return false;

  auto neededBytes = AsyncMqttClientInternals::Helpers::variableByteIntegerSize(vbi);

  buffer.reserve(buffer.size() + 1 + neededBytes);
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));

  do {
    uint8_t encodedByte = vbi % 128;
    vbi /= 128;
    if (vbi > 0) {
      encodedByte = encodedByte | 128u;
    }

    buffer.push_back(encodedByte);
  } while (vbi > 0);
  return true;
}
bool Properties::addStringProperty(Properties::Type t, const String& s) {
  if (getDatatype(t) != Datatype::UTF8_STRING)
    return false;

  buffer.reserve(buffer.size() + 1 + 2 + s.length());
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));

  buffer.push_back(s.length() >> 8u);
  buffer.push_back(s.length() & 0xFFu);

  buffer.insert(buffer.end(), s.begin(), s.end());

  return true;
}
bool Properties::addDataProperty(Properties::Type t, uint8_t* data, uint16_t length) {
  if (getDatatype(t) != Datatype::DATA)
    return false;

  buffer.reserve(buffer.size() + 1 + 2 + length);
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));

  buffer.push_back(length >> 8u);
  buffer.push_back(length & 0xFFu);

  buffer.insert(buffer.end(), data, data + length);

  return true;
}
bool Properties::addStringPairProperty(Properties::Type t, const String& key, const String& val) {
  if (getDatatype(t) != Datatype::DATA)
    return false;

  buffer.reserve(buffer.size() + 1 + 2 + key.length() + 2 + val.length());
  if (buffer.data() == nullptr)
    return false;

  buffer.push_back(static_cast<uint8_t>(t));

  buffer.push_back(key.length() >> 8u);
  buffer.push_back(key.length() & 0xFFu);

  buffer.insert(buffer.end(), key.begin(), key.end());

  buffer.push_back(val.length() >> 8u);
  buffer.push_back(val.length() & 0xFFu);

  buffer.insert(buffer.end(), val.begin(), val.end());
}
