#include "TrackingCamera.hpp"
#include "easylogging++.h"
#include "proto/messages_robocup_ssl_wrapper_tracked.pb.h"

TrackingCamera::TrackingCamera()
: settings_("/root/ssl_tracking_cam.json"),
  trackedFrameProvider_(settings_),
  presenter_(state_, settings_)
{
    SerialPortOptions options;
    options.baudRate = SerialPortOptions::RATE_115200;
    options.dataBits = SerialPortOptions::D8;
    options.parity = SerialPortOptions::NONE;
    options.stopBits = SerialPortOptions::S1;

    auto pSerialPort = SerialPortFactory::open(settings_.getSerial().port, options);
    if(!pSerialPort)
    {
        LOG(ERROR) << "Failed to open serial port: " << settings_.getSerial().port;
        return;
    }

    pMotionController_ = std::make_unique<MotionController>(pSerialPort);

    initSuccess_ = true;
}

TrackingCamera::~TrackingCamera()
{
}

int TrackingCamera::spinOnce()
{
    trackedFrameProvider_.spinOnce();
    pMotionController_->spinOnce();

    auto pTrackedFrame = trackedFrameProvider_.getLatestTrackedFrame();
    if(pTrackedFrame && pTrackedFrame->lastFrame.balls_size() > 0 && pMotionController_->isReady())
    {
        auto ball = pTrackedFrame->lastFrame.balls(0);
        float visibility = ball.has_visibility() ? ball.visibility() : 1.0f;
        // LOG(INFO) << "Ball at " << ball.pos().x() << " " << ball.pos().y() << " " << ball.pos().z() << ", " << visibility;

        state_.ball.pos_m[0] = ball.pos().x();
        state_.ball.pos_m[1] = ball.pos().y();
        state_.ball.pos_m[2] = ball.pos().z();
        state_.ball.visibility = visibility;

        if(visibility > 0.5f)
        {
            Eigen::Vector3f field_p_ball(ball.pos().x(), ball.pos().y(), ball.pos().z());

            float pan_deg;
            float tilt_deg;

            getPanTilt(field_p_ball, pan_deg, tilt_deg);

            const auto& limits = settings_.getLimits();
            pMotionController_->setVelMax(limits.velMax_degDs);
            pMotionController_->setAccMax(limits.accMax_degDs2);

            pMotionController_->setTargetPos(limitToRange(pan_deg, limits.pan_deg), limitToRange(tilt_deg, limits.tilt_deg));
        }
    }

    state_.camera.isReady = pMotionController_->isReady();
    state_.camera.isMoving = pMotionController_->isMoving();
    state_.camera.pan_deg = pMotionController_->getTargetPan();
    state_.camera.tilt_deg = pMotionController_->getTargetTilt();
    state_.camera.velMax_degDs = pMotionController_->getVelMax();
    state_.camera.accMax_degDs2 = pMotionController_->getAccMax();

    presenter_.spinOnce();

    return 0;
}

void TrackingCamera::getPanTilt(const Eigen::Vector3f& field_p_ball, float& pan_deg, float& tilt_deg)
{
    const auto& camPose = settings_.getCameraPose();

    Eigen::Translation3f field_p_base(camPose.positionInFieldFrame_m[0], camPose.positionInFieldFrame_m[1], camPose.positionInFieldFrame_m[2]);
    float field_rz_base = camPose.yawInFieldFrame_deg * M_PI / 180.0f;

    Eigen::Affine3f field_T_base = field_p_base * Eigen::AngleAxisf(field_rz_base, Eigen::Vector3f::UnitZ());
    auto base_T_field = field_T_base.inverse();

    auto base_p_ball = base_T_field * field_p_ball;

    // LOG(INFO) << "base_p_ball: " << base_p_ball.x() << ", " << base_p_ball.y() << ", " << base_p_ball.z();

    pan_deg = atan2f(base_p_ball.y(), base_p_ball.x()) * 180.0f / M_PI;
    tilt_deg = -atan2f(base_p_ball.z(), base_p_ball.x()) * 180.0f / M_PI;

    // LOG(INFO) << "pan: " << pan_deg << ", tilt: " << tilt_deg;
}

float TrackingCamera::limitToRange(float in, std::array<float, 2> limits)
{
    if(in < limits[0])
        in = limits[0];
    else if(in > limits[1])
        in = limits[1];

    return in;
}
