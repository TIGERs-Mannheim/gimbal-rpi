#include "GimbalProtocolTransceiver.hpp"

namespace gimbal_protocol
{

Transceiver::Transceiver(std::shared_ptr<IIoPort> pPort)
:pPort_(pPort)
{
}

void Transceiver::spinOnce()
{
    char portReadBuf[1024];
    int bytesRead;

    while((bytesRead = pPort_->read(portReadBuf, sizeof(portReadBuf))) > 0)
    {
        for(int i = 0; i < bytesRead; i++)
        {
            char c = portReadBuf[i];

            if(c == 0)
            {
                auto msg = Message::fromWireFormat(rxBuf_);

                if(msg.isValid())
                {
                    newRxMessage_.push(std::move(msg));
                    total_.msgsComplete++;
                }
                else
                {
                    total_.msgsCrcErr++;
                }

                rxBuf_.clear();
            }
            else
            {
                rxBuf_.push_back(c);
            }
        }

        total_.bytesRead += bytesRead;
    }
}

bool Transceiver::receive(Message& message)
{
    if(newRxMessage_.empty())
        return false;

    message = std::move(newRxMessage_.front());
    newRxMessage_.pop();

    return true;
}

void Transceiver::send(const Message& message)
{
    auto data = message.toWireFormat();
    send(data);
}

void Transceiver::send(const std::vector<uint8_t>& data)
{
    int bytesWritten = pPort_->write(data.data(), data.size());

    if(bytesWritten == data.size())
    {
        total_.msgSent++;
    }
    else
    {
        total_.msgSentTruncated++;
    }

    total_.bytesWritten += bytesWritten;
}

Transceiver::Stats& Transceiver::Stats::operator-=(const Transceiver::Stats& rhs)
{
    bytesRead -= rhs.bytesRead;
    msgsComplete -= rhs.msgsComplete;
    msgsCrcErr -= rhs.msgsCrcErr;
    bytesWritten -= rhs.bytesWritten;
    msgSent -= rhs.msgSent;
    msgSentTruncated -= rhs.msgSentTruncated;

    return *this;
}

Transceiver::Stats operator-(Transceiver::Stats lhs, const Transceiver::Stats& rhs)
{
    lhs -= rhs;
    return lhs;
}

Transceiver::Stats& Transceiver::Stats::operator*=(float scalar)
{
    bytesRead *= scalar;
    msgsComplete *= scalar;
    msgsCrcErr *= scalar;
    bytesWritten *= scalar;
    msgSent *= scalar;
    msgSentTruncated *= scalar;

    return *this;
}

Transceiver::Stats operator*(Transceiver::Stats lhs, float scalar)
{
    lhs *= scalar;
    return lhs;
}

}
