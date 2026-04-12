#include "TrackedFrameProvider.hpp"
#include "easylogging++.h"

TrackedFrameProvider::TrackedFrameProvider(Settings& settings)
: settings_(settings),
  trackerClient_(settings.getNetwork().trackerIp, settings.getNetwork().trackerPort)
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
    {
        return tNow - s.tLastReception > std::chrono::seconds(2);
    });
}

const TrackedFrameProvider::Source* TrackedFrameProvider::getLatestTrackedFrame() const
{
    if(sources_.empty())
        return nullptr;

    auto preferredSrc = std::ranges::find_if(sources_, [&](const auto& s){ return s.name == settings_.getNetwork().prefferedTrackerSource; });
    if(preferredSrc == sources_.end())
    {
        return &sources_[0];
    }

    return &(*preferredSrc);
}

void TrackedFrameProvider::handleMessage(const NetMessage& msg)
{
    TrackerWrapperPacket trackerPacket;

    // Parse packet and validate that source name and tracked frame are set
    if(!trackerPacket.ParseFromArray(msg.data.data(), msg.data.size()))
    {
        LOG(ERROR) << "Failed to parse tracker packet: " << trackerPacket.DebugString();
        return;
    }

    if(!trackerPacket.has_source_name() || !trackerPacket.has_tracked_frame())
        return;

    // Check if this source already exists. If not, create it.
    auto srcIter = std::ranges::find_if(sources_, [&](const auto& s)
                                        { return s.ipAddress == msg.address.to_string() && s.port == msg.port && s.uuid == trackerPacket.uuid() && s.name == trackerPacket.source_name(); });

    if(srcIter == sources_.end())
    {
        LOG(INFO) << "New source: " << trackerPacket.source_name();

        Source src;
        src.ipAddress = msg.address.to_string();
        src.port = msg.port;
        src.uuid = trackerPacket.uuid();
        src.name = trackerPacket.source_name();

        sources_.push_back(src);
        srcIter = --sources_.end();
    }

    // Update stored source data
    auto& src = *srcIter;
    src.tLastReception = std::chrono::steady_clock::now();

    src.lastFrame = trackerPacket.tracked_frame();

    // LOG(INFO) << "Tracket packet: " << trackerPacket.uuid() << ", src: " << trackerPacket.source_name();
    //
    // if(trackerPacket.has_tracked_frame())
    // {
    //     const auto& frame = trackerPacket.tracked_frame();
    //     LOG(INFO) << frame.frame_number() << ", " << frame.timestamp() << ", " << frame.balls_size();
    // }
}
