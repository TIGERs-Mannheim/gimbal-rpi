#pragma once

#include "SerialPort.hpp"
#include "Settings.hpp"
#include "math_util.hpp"
#include "model/GimbalProtocolTransceiver.hpp"

class GimbalController
{
public:
    GimbalController(Settings& settings, std::shared_ptr<ISerialPort> pSerialPort);
    ~GimbalController();

    void spinOnce();

    void startCalibration(uint8_t axisId, float testVoltage_V = 2.5f);
    void setTargetPos(float pan_deg, float tilt_deg);
    void setTargetVel(float pan_degDs, float tilt_degDs);
    void disableMotors();

    bool isConnected() const;
    bool isReady() const;

    auto& getState() const { return state_; }

    float getCurrentPan_deg() const { return -RAD_TO_DEG(state_.motion.encoderPosition_rad[0]) - settings_.cameraPose.axisZeroOffsets_deg[0]; }
    float getCurrentTilt_deg() const { return RAD_TO_DEG(state_.motion.encoderPosition_rad[1]) - settings_.cameraPose.axisZeroOffsets_deg[1]; }

    float getCurrentPositionRaw_deg(uint8_t axisId) const;

    void setCalibration(uint8_t axisId, const GimbalServoCalibration& calib);
    void setConfiguration(uint8_t axisId, const GimbalServoParameters& params);

private:
    using clock_t = std::chrono::steady_clock;

    Settings& settings_;

    std::shared_ptr<ISerialPort> pSerialPort_;
    gimbal_protocol::Transceiver transceiver_;

    clock_t::time_point tLastMsgReceived_;

    GimbalMsgState state_ {};
};
