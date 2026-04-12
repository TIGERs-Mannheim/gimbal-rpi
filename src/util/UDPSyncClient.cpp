#include "UDPSyncClient.hpp"
#include "easylogging++.h"

using boost::asio::ip::udp;
namespace asio = ::boost::asio;

UDPSyncClient::UDPSyncClient(const std::string& ip, uint16_t port)
{
    pSocket_ = std::make_unique<socket_t>(ioContext_, udp::endpoint(udp::v4(), port));
    pSocket_->set_option(udp::socket::reuse_address(true));
    auto address = asio::ip::address::from_string(ip);
    pSocket_->set_option(asio::ip::multicast::join_group(address));

    pSocket_->async_receive_from(asio::buffer(rxBuffer_), rxEndpoint_, std::bind(&UDPSyncClient::onReceive, this, std::placeholders::_1, std::placeholders::_2));
}

UDPSyncClient::~UDPSyncClient()
{
    pSocket_->cancel();
    spinOnce();
}

void UDPSyncClient::spinOnce()
{
    while(ioContext_.poll())
    {
        // poll until no further handlers are processed
    }
}

bool UDPSyncClient::receive(NetMessage& msg)
{
    if(receivedMessages_.empty())
        return false;

    msg = receivedMessages_.front();
    receivedMessages_.pop_front();

    return true;
}

void UDPSyncClient::send(const NetMessage& msg)
{
    pSocket_->send_to(asio::buffer(msg.data), udp::endpoint(msg.address, msg.port));
}

void UDPSyncClient::onReceive(const boost::system::error_code& ec, std::size_t bytesRead)
{
    if(ec.failed())
    {
        if(ec == asio::error::operation_aborted)
        {
            // cancel() was called
        }
        else
        {
            LOG(ERROR) << "RX error: " << ec.message();
        }
    }
    else
    {
        NetMessage msg;
        msg.address = rxEndpoint_.address();
        msg.port = rxEndpoint_.port();
        msg.data.assign(rxBuffer_.begin(), rxBuffer_.begin()+bytesRead);

        receivedMessages_.push_back(std::move(msg));

        pSocket_->async_receive_from(asio::buffer(rxBuffer_), rxEndpoint_, std::bind(&UDPSyncClient::onReceive, this, std::placeholders::_1, std::placeholders::_2));
    }
}
