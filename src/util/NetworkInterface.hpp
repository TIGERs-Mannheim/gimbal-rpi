#pragma once

#include "SysFsEntry.hpp"

#include <string>
#include <array>
#include <cstdint>
#include <optional>
#include <net/if.h>

class NetworkInterface
{
public:
    typedef std::array<uint8_t, 6> MacAddress;

    NetworkInterface(std::string name);

    bool isOnline();
    std::string getIp4();
    std::string getNetmask4();
    std::string getBroadcastIp4();
    std::string getName() const { return name_; }
    MacAddress getMacAddress() const;
    long getSpeed_Mbps() const;
    bool hasCarrier() const { return sysCarrier_.readInt32(); }

    bool waitUntilOnline(uint32_t timeout_ms);

    enum MacAddressFormat
    {
        MAC_ADDRESS_FORMAT_HEX_WITH_COLONS,
        MAC_ADDRESS_FORMAT_HEX_NO_COLONS,
    };

    static std::string formatMacAddress(const MacAddress& mac, MacAddressFormat format = MAC_ADDRESS_FORMAT_HEX_WITH_COLONS);

private:
    std::optional<ifreq> request(unsigned long opt) const;

    std::string name_;

    SysFsEntry sysCarrier_;
    SysFsEntry sysSpeed_;
};
