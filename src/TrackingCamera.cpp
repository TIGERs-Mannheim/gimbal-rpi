#include "TrackingCamera.hpp"
#include "easylogging++.h"
#include "proto/messages_robocup_ssl_wrapper_tracked.pb.h"

TrackingCamera::TrackingCamera()
: settings_("/root/ssl_tracking_cam.json"),
  eth0_("eth0"),
  hostname_("/proc/sys/kernel/hostname"),
  trackedFrameProvider_(settings_),
  view_(std::bind(&TrackingCamera::viewEventCallback, this, std::placeholders::_1))
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
    View::State viewState;

    trackedFrameProvider_.spinOnce();
    pMotionController_->spinOnce();
    joystick_.spinOnce();

    auto pTrackedFrame = trackedFrameProvider_.getLatestTrackedFrame();
    if(pTrackedFrame && pTrackedFrame->lastFrame.balls_size() > 0 && pMotionController_->isReady())
    {
        auto ball = pTrackedFrame->lastFrame.balls(0);
        float visibility = ball.has_visibility() ? ball.visibility() : 1.0f;
        // LOG(INFO) << "Ball at " << ball.pos().x() << " " << ball.pos().y() << " " << ball.pos().z() << ", " << visibility;

        viewState.ballPos_m[0] = ball.pos().x();
        viewState.ballPos_m[1] = ball.pos().y();
        viewState.ballPos_m[2] = ball.pos().z();

        if(visibility > 0.5f)
        {
            Eigen::Vector3f field_p_ball(ball.pos().x(), ball.pos().y(), ball.pos().z());

            float pan_deg;
            float tilt_deg;

            getPanTilt(field_p_ball, pan_deg, tilt_deg);

            if(mode_ == MODE_LIVE)
            {
                const auto& limits = settings_.getLimits();
                pMotionController_->setVelMax(limits.velMax_degDs);
                pMotionController_->setAccMax(limits.accMax_degDs2);
                pMotionController_->setTargetPos(limitToRange(pan_deg, limits.pan_deg), limitToRange(tilt_deg, limits.tilt_deg));
            }
        }
    }

    if(mode_ == MODE_MANUAL && pMotionController_->isReady())
    {
        pMotionController_->setVelMax(200.0f);
        pMotionController_->setAccMax(200.0f);
        pMotionController_->setTargetPos(joystick_.getPan_deg(), joystick_.getTilt_deg());
    }

    viewState.hostname = hostname_.readString();
    viewState.pan_deg = pMotionController_->getCurrentPan();
    viewState.tilt_deg = pMotionController_->getCurrentTilt();
    viewState.isHot = pMotionController_->areDriversHot();

    if(pTrackedFrame)
    {
        viewState.tracker = pTrackedFrame->name;
        viewState.trackerIp = pTrackedFrame->ipAddress + ":" + std::to_string(pTrackedFrame->port);
    }
    else
    {
        viewState.tracker = "None";
        viewState.trackerIp = "-";
    }

    if(eth0_.isOnline() && !eth0_.getIp4().empty())
        viewState.ip = eth0_.getIp4();
    else
        viewState.ip = "-";

    if(!pMotionController_->areStepperDriversConnected())
    {
        viewState.mode = "HW FAIL";
    }
    else if(!pMotionController_->isReady())
    {
        viewState.mode = "HOMING";
    }
    else
    {
        switch(mode_)
        {
            case MODE_OFF:
                viewState.mode = "OFF";
                break;
            case MODE_MANUAL:
                viewState.mode = "MANUAL";
                break;
            case MODE_LIVE:
                viewState.mode = "LIVE";
                break;
        }
    }

    view_.setState(viewState);

    view_.spinOnce();

    if(quit_)
        return 1;

    return 0;
}

void TrackingCamera::viewEventCallback(View::Event event)
{
    switch(event)
    {
        case View::EVENT_HOME_CLICKED:
            pMotionController_->resetHome();
            break;
        case View::EVENT_OFF_CLICKED:
            mode_ = MODE_OFF;
            break;
        case View::EVENT_MANUAL_CLICKED:
            mode_ = MODE_MANUAL;
            break;
        case View::EVENT_LIVE_CLICKED:
            mode_ = MODE_LIVE;
            break;
        case View::EVENT_QUIT_CLICKED:
            quit_ = true;
            break;
    }
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
