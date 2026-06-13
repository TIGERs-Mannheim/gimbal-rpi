#include "TrackedFrameProvider.hpp"
#include "easylogging++.h"

TrackedFrameProvider::TrackedFrameProvider(Settings& settings)
: settings_(settings),
  trackerClient_(settings.network.trackerIp, settings.network.trackerPort)
{
}

void TrackedFrameProvider::spinOnce()
{
    trackerClient_.spinOnce();

    // check received messages
    NetMessage netMsg;
    while(trackerClient_.receive(netMsg))
    {
        handleMessage(netMsg);
    }

    // Prune sources on timeout
    auto tNow = std::chrono::steady_clock::now();
    std::erase_if(sources_, [&](const auto& s)
                  { return tNow - s.tLastReception > std::chrono::seconds(2); });
}

const TrackedFrameProvider::Source* TrackedFrameProvider::getLatestTrackedFrame() const
{
    if(sources_.empty())
        return nullptr;

    auto preferredSrc = std::ranges::find_if(sources_, [&](const auto& s)
                                             { return s.name == settings_.network.preferedTrackerSource; });
    if(preferredSrc == sources_.end())
    {
        return &sources_[0];
    }

    return &(*preferredSrc);
}

void TrackedFrameProvider::handleMessage(const NetMessage& msg)
{
    // Parse packet and validate that source name and tracked frame are set
    auto pUnpacked = tracker_wrapper_packet__unpack(nullptr, msg.data.size(), msg.data.data());
    if(!pUnpacked)
    {
        LOG(ERROR) << "Failed to parse tracker packet with " << msg.data.size() << "B from " << msg.address.to_string() << ":" << msg.port;
        return;
    }

    // Make unique_ptr with custom deallocator for proper cleanup
    TrackerWrapperPacketUniquePtr pTrackerPacket(pUnpacked, [](TrackerWrapperPacket* p){tracker_wrapper_packet__free_unpacked(p, nullptr); });

    if(!pTrackerPacket->source_name || !pTrackerPacket->tracked_frame)
        return;

    // Check if this source already exists. If not, create it.
    auto srcIter = std::ranges::find_if(sources_, [&](const auto& s)
                                        { return s.ipAddress == msg.address.to_string() && s.port == msg.port && s.uuid == std::string(pTrackerPacket->uuid) && s.name == std::string(pTrackerPacket->source_name); });

    if(srcIter == sources_.end())
    {
        LOG(INFO) << "New source: " << pTrackerPacket->source_name;

        Source src;
        src.ipAddress = msg.address.to_string();
        src.port = msg.port;
        src.uuid = pTrackerPacket->uuid;
        src.name = pTrackerPacket->source_name;

        sources_.push_back(std::move(src));
        srcIter = --sources_.end();
    }

    // Update stored source data
    auto& src = *srcIter;
    src.tLastReception = std::chrono::steady_clock::now();

    // LOG(INFO) << "Tracket packet: " << pTrackerPacket->uuid << ", src: " << pTrackerPacket->source_name;
    //
    // if(pTrackerPacket->tracked_frame)
    // {
    //     const auto& frame = *pTrackerPacket->tracked_frame;
    //     LOG(INFO) << frame.frame_number << ", " << frame.timestamp << ", " << frame.n_balls;
    // }

    src.pLastPacket = std::move(pTrackerPacket);
}
