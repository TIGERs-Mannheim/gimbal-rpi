#pragma once

#include "util/UDPSyncClient.hpp"
#include "Settings.hpp"
#include "protobuf-c/messages_robocup_ssl_wrapper_tracked.pb-c.h"

class TrackedFrameProvider
{
public:
    using TrackerWrapperPacketUniquePtr = std::unique_ptr<TrackerWrapperPacket, void(*)(TrackerWrapperPacket*)>;

    struct Source
    {
        std::string ipAddress;
        uint16_t port;
        std::string uuid;
        std::string name;
        std::chrono::steady_clock::time_point tLastReception;
        TrackerWrapperPacketUniquePtr pLastPacket { nullptr, [](TrackerWrapperPacket* p) {} };

        bool operator==(const Source& rhs) const { return ipAddress == rhs.ipAddress && port == rhs.port && uuid == rhs.uuid && name == rhs.name; }

        auto getTrackedFrame() const { return pLastPacket ? *pLastPacket->tracked_frame : (TrackedFrame)TRACKED_FRAME__INIT; }
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
