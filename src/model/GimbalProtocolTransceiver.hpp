#pragma once

#include <memory>
#include <chrono>
#include <vector>
#include <queue>
#include <cstdint>

#include "IoPort.hpp"
#include "GimbalProtocolMessage.hpp"

namespace gimbal_protocol
{

/**
 * The gimbal protocol transceiver interfaces with a generic byte-oriented IoPort.
 *
 * It converts messages to the gimbal wire format and sends them out.
 * It reads and parses bytes in gimbal wire format and stores complete messages in a buffer.
 *
 * This implementation is thread-less and spinOnce() should be called regularly.
 */
class Transceiver : public ITransceiver
{
public:
    struct Stats
    {
        uint32_t bytesRead = 0;
        uint32_t msgsComplete = 0;
        uint32_t msgsCrcErr = 0;
        uint32_t bytesWritten = 0;
        uint32_t msgSent = 0;
        uint32_t msgSentTruncated = 0;

        Stats& operator-=(const Stats& rhs);
        friend Stats operator-(Stats lhs, const Stats& rhs);

        Stats& operator*=(float scalar);
        friend Stats operator*(Stats lhs, float scalar);
    };

    Transceiver(std::shared_ptr<IIoPort> pPort);

    void spinOnce();
    bool receive(Message& message) override;
    void send(const Message& message) override;
    void send(const std::vector<uint8_t>& data);

    const Stats& getStats() const { return total_; }

private:
    std::shared_ptr<IIoPort> pPort_;

    std::vector<uint8_t> rxBuf_;

    std::queue<Message> newRxMessage_;

    Stats total_;
};

}
