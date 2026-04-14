#pragma once

#include "nlohmann/json.hpp"

#include <string>

class Settings
{
public:
    struct Serial
    {
        std::string port = "/dev/ttyAMA0";

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Serial, port);
    };

    struct Network
    {
        std::string trackerIp = "224.5.23.2";
        uint16_t trackerPort = 10010;
        std::string prefferedTrackerSource = "TIGERs";

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Network, trackerIp, trackerPort, prefferedTrackerSource);
    };

    struct CameraPose
    {
        std::array<float, 3> positionInFieldFrame_m = { 0, 0, 0 };
        float yawInFieldFrame_deg = 0.0f;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CameraPose, positionInFieldFrame_m, yawInFieldFrame_deg);
    };

    struct Limits
    {
        std::array<float, 2> pan_deg = { -90.0f, 90.0f };
        std::array<float, 2> tilt_deg = { -40.0f, 40.0f };
        float velMax_degDs = 200.0f;
        float accMax_degDs2 = 400.0f;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Limits, pan_deg, tilt_deg, velMax_degDs, accMax_degDs2);
    };

    Settings(const std::string& filename);
    ~Settings();

    Serial& getSerial() { return serial_; }
    Network& getNetwork() { return network_; }
    CameraPose& getCameraPose() { return cameraPose_; }
    Limits& getLimits() { return limits_; }

private:
    void load();
    void save();

    std::string filename_;

    Serial serial_;
    Network network_;
    CameraPose cameraPose_;
    Limits limits_;
};
