#pragma once

#include "Settings.hpp"
#include "GimbalController.hpp"
#include "TrackedFrameProvider.hpp"
#include "View.hpp"
#include "NetworkInterface.hpp"
#include "drv/Joystick.hpp"
#include <eigen3/Eigen/Geometry>

class TrackingCamera
{
public:
    enum Mode
    {
        MODE_OFF,
        MODE_MANUAL,
        MODE_LIVE,
    };

    TrackingCamera();
    ~TrackingCamera();

    int spinOnce();

private:
    void getPanTilt(const Eigen::Vector3f& field_p_ball, float& pan_deg, float& tilt_deg);
    void handleEvent(View::EventData& event);

    bool initSuccess_ = false;

    Settings settings_;
    NetworkInterface eth0_;
    SysFsEntry hostname_;
    std::unique_ptr<GimbalController> pGimbalController_;
    TrackedFrameProvider trackedFrameProvider_;
    Joystick joystick_;
    View view_;

    Mode mode_ = MODE_LIVE;
    bool quit_ = false;
};
