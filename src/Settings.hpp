#pragma once

#include "easylogging++.h"
#include "nlohmann/json.hpp"
#include "model/gimbal_protocol.h"

#include <string>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GimbalServoCalibration,
                                                motorInvertDirection, motorPhaseCurrentOffsetU_V, motorPhaseCurrentOffsetV_V, motorPhaseCurrentOffsetW_V,
                                                encoderPosElectricalZero, phaseResistance_R, phaseInductance_H);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GimbalServoParameters, current.bandwidth_Hz, current.outputRamp_VDs,
                                            velocity.lowPassBandwidth_Hz, velocity.Kp, velocity.Ki, velocity.Kd, velocity.outputRamp_ADs, velocity.outputLimit_A,
                                            position.Kp, position.Ki, position.Kd, position.outputRamp_radDs2, position.outputLimit_radDs);

struct Settings
{
    struct Serial
    {
        std::string port = "/dev/ttyAMA0";

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Serial, port);
    };

    struct Network
    {
        std::string trackerIp = "224.5.23.2";
        uint16_t trackerPort = 10010;
        std::string preferedTrackerSource = "TIGERs";

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Network, trackerIp, trackerPort, preferedTrackerSource);
    };

    struct CameraPose
    {
        std::array<float, 3> positionInFieldFrame_m = { 0, 0, 1.7f };
        float yawInFieldFrame_deg = 0.0f;
        std::array<float, 2> axisZeroOffsets_deg = { 0.0f, 0.0f };

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CameraPose, positionInFieldFrame_m, yawInFieldFrame_deg, axisZeroOffsets_deg);
    };

    struct Limits
    {
        std::array<float, 2> pan_deg = { -90.0f, 90.0f };
        std::array<float, 2> tilt_deg = { -40.0f, 40.0f };
        float velMax_degDs = 200.0f;
        float accMax_degDs2 = 400.0f;
        float jerkMax_degDs3 = 2000.0f;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Limits, pan_deg, tilt_deg, velMax_degDs, accMax_degDs2, jerkMax_degDs3);
    };

    struct StoredGimbalServoCalibration
    {
        GimbalServoCalibration data;
        bool isCalibrated = false;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(StoredGimbalServoCalibration, isCalibrated, data);
    };


    Settings();
    Settings(const std::string& filename);

    bool load();
    void save();

    static GimbalServoParameters getDefaultServoParameters();

    const std::string filename;

    Serial serial;
    Network network;
    CameraPose cameraPose;
    Limits limits;
    std::array<StoredGimbalServoCalibration, 2> servoCalibration {};
    std::array<GimbalServoParameters, 2> servoParameters {};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Settings, serial, network, cameraPose, limits, servoCalibration, servoParameters);
};
