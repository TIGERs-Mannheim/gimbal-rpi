#include "NetworkInterface.hpp"

#include <chrono>
#include <thread>
#include <cstring>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>

NetworkInterface::NetworkInterface(std::string name)
:name_(name),
 sysCarrier_("/sys/class/net/" + name + "/carrier"),
 sysSpeed_("/sys/class/net/" + name + "/speed")
{
}

bool NetworkInterface::isOnline()
{
    auto optIfr = request(SIOCGIFFLAGS);
    if(!optIfr)
        return false;

    return !!(optIfr.value().ifr_flags & IFF_UP);
}

std::string NetworkInterface::getIp4()
{
    auto optIfr = request(SIOCGIFADDR);
    if(!optIfr)
        return std::string();

    char* buf = inet_ntoa(((struct sockaddr_in *)&optIfr.value().ifr_addr)->sin_addr);

    return std::string(buf);
}

std::string NetworkInterface::getNetmask4()
{
    auto optIfr = request(SIOCGIFNETMASK);
    if(!optIfr)
        return std::string();

    char* buf = inet_ntoa(((struct sockaddr_in *)&optIfr.value().ifr_netmask)->sin_addr);

    return std::string(buf);
}

std::string NetworkInterface::getBroadcastIp4()
{
    auto optIfr = request(SIOCGIFBRDADDR);
    if(!optIfr)
        return std::string();

    char* buf = inet_ntoa(((struct sockaddr_in *)&optIfr.value().ifr_broadaddr)->sin_addr);

    return std::string(buf);
}

bool NetworkInterface::waitUntilOnline(uint32_t timeout_ms)
{
    auto tStart = std::chrono::steady_clock::now();

    while(!isOnline())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if(std::chrono::steady_clock::now() - tStart > std::chrono::milliseconds(timeout_ms))
        {
            return false;
        }
    }

    return true;
}

NetworkInterface::MacAddress NetworkInterface::getMacAddress() const
{
    MacAddress mac = {};

    auto optIfr = request(SIOCGIFHWADDR);
    if(!optIfr)
        return mac;

    memcpy(mac.data(), optIfr.value().ifr_hwaddr.sa_data, 6);

    return mac;
}

std::string NetworkInterface::formatMacAddress(const MacAddress& mac, MacAddressFormat format)
{
    char buf[32];

    switch(format)
    {
        case MAC_ADDRESS_FORMAT_HEX_WITH_COLONS:
            snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
        case MAC_ADDRESS_FORMAT_HEX_NO_COLONS:
            snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
    }

    return std::string(buf);
}

long NetworkInterface::getSpeed_Mbps() const
{
    return sysSpeed_.readInt32();
}

std::optional<ifreq> NetworkInterface::request(unsigned long opt) const
{
    ifreq ifr = {};
    strcpy(ifr.ifr_name, name_.c_str());

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sock < 0)
        return std::nullopt;

    int result = ioctl(sock, opt, &ifr);
    close(sock);

    if(result < 0)
        return std::nullopt;

    return ifr;
}
