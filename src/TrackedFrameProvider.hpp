#pragma once

#include "util/UDPSyncClient.hpp"
#include "Settings.hpp"
#include "proto/messages_robocup_ssl_wrapper_tracked.pb.h"

class TrackedFrameProvider
{
public:
    struct Source
    {
        std::string ipAddress;
        uint16_t port;
        std::string uuid;
        std::string name;
        std::chrono::steady_clock::time_point tLastReception;

        TrackedFrame lastFrame;

        bool operator==(const Source& rhs) const { return ipAddress == rhs.ipAddress && port == rhs.port && uuid == rhs.uuid && name == rhs.name; }
    };

    TrackedFrameProvider(Settings& settings);

    void spinOnce();
    const Source* getLatestTrackedFrame() const;

    const std::vector<Source>& getSources() const { return sources_; }

private:
    void handleMessage(const NetMessage& msg);

    Settings& settings_;
    UDPSyncClient trackerClient_;
    std::vector<Source> sources_;
};
