#pragma once

#include <vector>
#include <cstdint>
#include <chrono>

#include "gimbal_protocol.h"

namespace gimbal_protocol
{

/**
 * A gimbal protocol message consists of:
 * - A header with protocol version and message type fields.
 * - Payload data.
 *
 * Objects of this class have an automatic system timestamp which is set upon creation.
 *
 * Messages can be converted to the wire format (COBS header).
 *
 * Messages can be invalid if they don't contain a real message or parsing of a format failed.
 */
class Message
{
public:
    using clock_t = std::chrono::system_clock;

    Message() = default;
    Message(const Message&) = default;
    Message(Message&&) = default;
    Message(const GimbalMsgHeader& header, const std::vector<uint8_t>& payload);

    Message(uint16_t type, uint16_t version = 0);
    Message(uint16_t type, const std::vector<uint8_t>& payload, uint16_t version = 0);

    template <typename T>
    Message(uint16_t type, const T& payload, uint16_t version = 0);

    ~Message() = default;

    Message& operator=(const Message&) = default;
    Message& operator=(Message&&) = default;

    template <typename T>
    const T* as() const;

    bool isValid() const { return isValid_; }
    uint16_t getType() const { return header_.type; }
    uint16_t getVersion() const { return header_.version; }
    const std::vector<uint8_t>& getData() const { return data_; }
    const clock_t::time_point& getCreateTime() const { return createTime_; };

    static Message fromWireFormat(const std::vector<uint8_t>& wire);

    std::vector<uint8_t> toWireFormat() const;

private:
    bool isValid_ { false };
    GimbalMsgHeader header_;
    std::vector<uint8_t> data_;
    clock_t::time_point createTime_;
};

template <typename T>
Message::Message(uint16_t type, const T& payload, uint16_t version)
{
    createTime_ = clock_t::now();
    header_.version = version;
    header_.type = type;

    auto pPayload = reinterpret_cast<const uint8_t*>(&payload);
    data_.assign(pPayload, pPayload + sizeof(T));

    isValid_ = true;
}

template <typename T>
const T* Message::as() const
{
    if(data_.size() < sizeof(T))
        return 0;

    return reinterpret_cast<const T*>(data_.data());
}

/**
 * An interface for a transceiver capable of sending and receiving gimbal_protocol::Message
 */
class ITransceiver
{
public:
    virtual ~ITransceiver() {}

    virtual bool receive(Message& message) =0;
    virtual void send(const Message& message) =0;
};

}
