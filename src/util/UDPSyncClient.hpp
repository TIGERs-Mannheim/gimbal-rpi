#pragma once

#include <boost/asio.hpp>
#include <deque>

struct NetMessage
{
    boost::asio::ip::address address;
    boost::asio::ip::port_type port;
    std::vector<uint8_t> data;
};

class UDPSyncClient
{
public:
    UDPSyncClient(const std::string& ip, uint16_t port);
    ~UDPSyncClient();

    void spinOnce();

    bool receive(NetMessage& msg);
    void send(const NetMessage& msg);

private:
    using socket_t = boost::asio::ip::udp::socket;
    using endpoint_t = boost::asio::ip::udp::endpoint;

    void onReceive(const boost::system::error_code& ec, std::size_t bytesRead);

    boost::asio::io_context ioContext_;

    std::unique_ptr<socket_t> pSocket_;

    std::array<uint8_t, 2048> rxBuffer_;
    endpoint_t rxEndpoint_;

    std::deque<NetMessage> receivedMessages_;
};
