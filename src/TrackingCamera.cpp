#include "TrackingCamera.hpp"
#include "easylogging++.h"
#include "math_util.hpp"
#include "proto/messages_robocup_ssl_wrapper_tracked.pb.h"

TrackingCamera::TrackingCamera()
: settings_("/root/ssl_tracking_cam.json"),
  eth0_("eth0"),
  hostname_("/proc/sys/kernel/hostname"),
  trackedFrameProvider_(settings_)
{
    SerialPortOptions options;
    options.baudRate = SerialPortOptions::RATE_921600;
    options.dataBits = SerialPortOptions::D8;
    options.parity = SerialPortOptions::EVEN;
    options.stopBits = SerialPortOptions::S1;

    auto pSerialPort = SerialPortFactory::open(settings_.serial.port, options);
    if(!pSerialPort)
    {
        LOG(ERROR) << "Failed to open serial port: " << settings_.serial.port;
        return;
    }

    pGimbalController_ = std::make_unique<GimbalController>(settings_, pSerialPort);
    pGimbalController_->disableMotors();

    view_.setPose(settings_.cameraPose.positionInFieldFrame_m, settings_.cameraPose.yawInFieldFrame_deg);

    initSuccess_ = true;
}

TrackingCamera::~TrackingCamera()
{
}

int TrackingCamera::spinOnce()
{
    View::State viewState;

    trackedFrameProvider_.spinOnce();
    pGimbalController_->spinOnce();
    joystick_.spinOnce();

    auto pTrackedFrame = trackedFrameProvider_.getLatestTrackedFrame();
    if(pTrackedFrame && pTrackedFrame->lastFrame.balls_size() > 0 && pGimbalController_->isReady())
    {
        auto ball = pTrackedFrame->lastFrame.balls(0);
        float visibility = ball.has_visibility() ? ball.visibility() : 1.0f;

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
                const auto& limits = settings_.limits;
                pGimbalController_->setTargetPos(limitToRange(pan_deg, limits.pan_deg), limitToRange(tilt_deg, limits.tilt_deg));
            }
        }
    }

    if(mode_ == MODE_MANUAL && pGimbalController_->isReady())
    {
        pGimbalController_->setTargetPos(joystick_.getPan_deg(), joystick_.getTilt_deg());
    }

    viewState.hostname = hostname_.readString();
    viewState.pan_deg = pGimbalController_->getCurrentPan_deg();
    viewState.tilt_deg = pGimbalController_->getCurrentTilt_deg();
    viewState.limitPan_deg = settings_.limits.pan_deg;
    viewState.limitTilt_deg = settings_.limits.tilt_deg;
    viewState.gimbalSupply_V = pGimbalController_->getState().power.supplyVcc_mV * 0.001f;
    viewState.gimbalCpuLoad = pGimbalController_->getState().cpuLoad * 0.01f;

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

    if(!pGimbalController_->isConnected())
    {
        viewState.mode = "HW FAIL";
    }
    else if(!pGimbalController_->isReady())
    {
        if(pGimbalController_->getState().isServoCalibrated[0])
            viewState.mode = "NOCAL T";
        else if(pGimbalController_->getState().isServoCalibrated[1])
            viewState.mode = "NOCAL P";
        else
            viewState.mode = "NOCAL";
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

    View::EventData event;
    while(view_.getNextEvent(event))
        handleEvent(event);

    if(quit_)
        return 1;

    return 0;
}

void TrackingCamera::handleEvent(View::EventData& event)
{
    switch(event.event)
    {
        case View::EVENT_OFF_CLICKED:
            mode_ = MODE_OFF;
            pGimbalController_->disableMotors();
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
        case View::EVENT_POSE_X_CHANGED:
            settings_.cameraPose.positionInFieldFrame_m[0] = event.intParam*0.001f;
            settings_.save();
            break;
        case View::EVENT_POSE_Y_CHANGED:
            settings_.cameraPose.positionInFieldFrame_m[1] = event.intParam*0.001f;
            settings_.save();
            break;
        case View::EVENT_POSE_Z_CHANGED:
            settings_.cameraPose.positionInFieldFrame_m[2] = event.intParam*0.001f;
            settings_.save();
            break;
        case View::EVENT_POSE_ORIENTATION_CHANGED:
            settings_.cameraPose.yawInFieldFrame_deg = event.intParam;
            settings_.save();
            break;
        case View::EVENT_CALIB_CLICKED:
            mode_ = MODE_OFF;
            pGimbalController_->startCalibration(event.intParam);
            break;
        case View::EVENT_LIMIT_PAN_CLICKED:
            settings_.limits.pan_deg[event.intParam] = pGimbalController_->getCurrentPan_deg();
            settings_.save();
            break;
        case View::EVENT_LIMIT_TILT_CLICKED:
            settings_.limits.tilt_deg[event.intParam] = pGimbalController_->getCurrentTilt_deg();
            settings_.save();
            break;
        case View::EVENT_ZERO_CLICKED:
            settings_.cameraPose.axisZeroOffsets_deg[event.intParam] = pGimbalController_->getCurrentPositionRaw_deg(event.intParam);
            joystick_.setPosition(event.intParam, 0);
            settings_.save();
            break;
        default:
            break;
    }
}

void TrackingCamera::getPanTilt(const Eigen::Vector3f& field_p_ball, float& pan_deg, float& tilt_deg)
{
    const auto& camPose = settings_.cameraPose;

    Eigen::Translation3f field_p_base(camPose.positionInFieldFrame_m[0], camPose.positionInFieldFrame_m[1], camPose.positionInFieldFrame_m[2]);
    float field_rz_base = camPose.yawInFieldFrame_deg * M_PI / 180.0f;

    Eigen::Affine3f field_T_base = field_p_base * Eigen::AngleAxisf(field_rz_base, Eigen::Vector3f::UnitZ());
    auto base_T_field = field_T_base.inverse();

    auto base_p_ball = base_T_field * field_p_ball;

    // LOG(INFO) << "base_p_ball: " << base_p_ball.x() << ", " << base_p_ball.y() << ", " << base_p_ball.z();

    pan_deg = atan2f(base_p_ball.y(), base_p_ball.x()) * 180.0f / M_PI;
    tilt_deg = atan2f(base_p_ball.z(), base_p_ball.head(2).norm()) * 180.0f / M_PI;

    // LOG(INFO) << "pan: " << pan_deg << ", tilt: " << tilt_deg;
}
