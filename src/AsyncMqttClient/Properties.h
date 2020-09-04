#pragma once

#include <vector>
#include <cstdint>
#include "WString.h"
#include "../../tl/optional.hpp"

namespace AsyncMqttClientInternals {
class PublishPacket;
}
class Properties {
    using buffer_t = std::vector<uint8_t>;

    buffer_t buffer;

    friend class AsyncMqttClient;
    friend class AsyncMqttClientInternals::PublishPacket;

public:
    using const_itr_t = buffer_t::const_iterator;

    enum class Datatype : uint8_t {
        UNKNOWN,
        BYTE,
        TWO_BYTE_INTEGER,
        FOUR_BYTE_INTEGER,
        VARIABLE_BYTE_INTEGER,
        UTF8_STRING,
        DATA,
        UTF8_STRING_PAIR
    };
    enum class Type : uint8_t {
        PAYLOAD_FORMAT_IDENTIFIER   = 0x01,
        MESSAGE_EXPIRY_INTERVAL     = 0x02,
        CONTENT_TYPE                = 0x03,
        RESPONSE_TOPIC              = 0x08,
        CORRELATION_DATA            = 0x09,
        SUBSCRIPTION_IDENTIFIER     = 0x0B,

        SESSION_EXPIRY_INTERVAL     = 0x11,
        ASSIGNED_CLIENT_IDENTIFIER  = 0x12,
        SERVER_KEEP_ALIVE           = 0x13,
        AUTHENTICATION_METHOD       = 0x15,
        AUTHENTICATION_DATA         = 0x16,
        REQUEST_PROBLEM_INFORMATION = 0x17,
        WILL_DELAY_INTERVAL         = 0x18,
        REQUEST_RESPONSE_INFORMATION= 0x19,
        RESPONSE_INFORMATION        = 0x1A,
        SERVER_REFERENCE            = 0x1C,
        REASON_STRING               = 0x1F,
        RECEIVE_MAXIMUM             = 0x21,
        TOPIC_ALIAS_MAXIMUM         = 0x22,
        TOPIC_ALIAS                 = 0x23,
        MAXIMUM_QOS                 = 0x24,
        RETAIN_AVAILIBLE            = 0x25,
        USER_PROPERTY               = 0x26,
        MAXIMUM_PACKET_SIZE         = 0x27,
        WILDCARD_SUBSCRIPTION_AVAILIBLE = 0x28,
        SUBSCRIPTION_IDENTIFIER_AVAILIBLE = 0x29,
        SHARED_SUBSCRIPTION_AVAILIBLE = 0x2A,
    };

    explicit Properties() : buffer() {};
    explicit Properties(std::vector<uint8_t>&& buffer) : buffer(std::move(buffer)) {};

    /**
     * getDatatype returns the datatype which the standard declares for the property t
     * @param t the property type which is used
     * @return the datatype associated for the property type t
     */
    static Datatype getDatatype(Type t) {
        switch (t) {
            case Type::PAYLOAD_FORMAT_IDENTIFIER:
            case Type::REQUEST_PROBLEM_INFORMATION:
            case Type::REQUEST_RESPONSE_INFORMATION:
            case Type::MAXIMUM_QOS:
            case Type::RETAIN_AVAILIBLE:
            case Type::WILDCARD_SUBSCRIPTION_AVAILIBLE:
            case Type::SUBSCRIPTION_IDENTIFIER_AVAILIBLE:
            case Type::SHARED_SUBSCRIPTION_AVAILIBLE:
                return Datatype::BYTE;
            case Type::SERVER_KEEP_ALIVE:
            case Type::RECEIVE_MAXIMUM:
            case Type::TOPIC_ALIAS_MAXIMUM:
            case Type::TOPIC_ALIAS:
                return Datatype::TWO_BYTE_INTEGER;
            case Type::MESSAGE_EXPIRY_INTERVAL:
            case Type::SESSION_EXPIRY_INTERVAL:
            case Type::WILL_DELAY_INTERVAL:
            case Type::MAXIMUM_PACKET_SIZE:
                return Datatype::FOUR_BYTE_INTEGER;
            case Type::SUBSCRIPTION_IDENTIFIER:
                return Datatype::VARIABLE_BYTE_INTEGER;
            case Type::CONTENT_TYPE:
            case Type::RESPONSE_TOPIC:
            case Type::ASSIGNED_CLIENT_IDENTIFIER:
            case Type::AUTHENTICATION_METHOD:
            case Type::RESPONSE_INFORMATION:
            case Type::SERVER_REFERENCE:
            case Type::REASON_STRING:
                return Datatype::UTF8_STRING;
            case Type::USER_PROPERTY:
                return Datatype::UTF8_STRING_PAIR;
            case Type::CORRELATION_DATA:
            case Type::AUTHENTICATION_DATA:
                return Datatype::DATA;
            default:
                return Datatype::UNKNOWN;
        }
    }
  private:
    /**
     *
     * @param t
     * @param start
     * @return
     */
    std::pair<const_itr_t, const_itr_t> parseProperty(Type t, const_itr_t start) const;
  public:

    /**
     * getDataProperty reads the properties until the property of type t is found offset times and returns the position
     * @param t the type of property
     * @param offset the count how often a property with type t is ignored
     * @return a pair of iterators enclosing the property (first points to the first byte in the property, after the type. Second points after the end of the property)
     */
    std::pair<const_itr_t, const_itr_t> getProperty(Type t, uint32_t offset) const;

    tl::optional<uint8_t> getByteProperty(Type t, uint32_t offset) const;
    tl::optional<uint16_t> getTwoByteProperty(Type t, uint32_t offset) const;
    tl::optional<uint32_t> getFourByteProperty(Type t, uint32_t offset) const;
    tl::optional<uint32_t> getVbiProperty(Type t, uint32_t offset) const;
    tl::optional<String> getStringProperty(Type t, uint32_t offset) const;
    tl::optional<std::pair<const_itr_t, const_itr_t>> getDataProperty(Type t, uint32_t offset) const;
    tl::optional<std::pair<String, String>> getStringPairProperty(Type t, uint32_t offset) const;

    bool addByteProperty(Type t, uint8_t b);
  bool addTwoByteProperty(Type t, uint16_t s);
  bool addFourByteProperty(Type t, uint32_t i);
  bool addVbiProperty(Type t, uint32_t vbi);
  bool addStringProperty(Type t, const String& s);
  bool addDataProperty(Type t, uint8_t* data, uint16_t length);
  bool addStringPairProperty(Type t, const String& key, const String& val);
};

