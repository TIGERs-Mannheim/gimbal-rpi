#pragma once

#include "Settings.hpp"
#include "MotionController.hpp"
#include "TrackedFrameProvider.hpp"
#include <eigen3/Eigen/Geometry>

class TrackingCamera
{
public:
    TrackingCamera();
    ~TrackingCamera();

    int spinOnce();

private:
    void getPanTilt(const Eigen::Vector3f& field_p_ball, float& pan_deg, float& tilt_deg);
    float limitToRange(float in, std::array<float, 2> limits);

    bool initSuccess_ = false;

    Settings settings_;
    std::unique_ptr<MotionController> pMotionController_;
    TrackedFrameProvider trackedFrameProvider_;
};
