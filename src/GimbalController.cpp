#include "GimbalController.hpp"
#include "easylogging++.h"
#include "math_util.hpp"

GimbalController::GimbalController(Settings& settings, std::shared_ptr<ISerialPort> pSerialPort)
: settings_(settings),
  pSerialPort_(pSerialPort),
  transceiver_(pSerialPort_)
{
}

GimbalController::~GimbalController()
{
}

void GimbalController::spinOnce()
{
    transceiver_.spinOnce();

    gimbal_protocol::Message msg;

    while(transceiver_.receive(msg))
    {
        if(msg.getVersion() != 0)
        {
            LOG(WARNING) << "Unsupported message version received: " << msg.getVersion();
            continue;
        }

        tLastMsgReceived_ = std::chrono::steady_clock::now();

        switch(msg.getType())
        {
            case GIMBAL_MSG_TYPE_STATE:
            {
                auto pMsg = msg.as<GimbalMsgState>();
                if(pMsg)
                {
                    state_ = *pMsg;

                    for(size_t i = 0; i < MACHINE_NUM_AXES; i++)
                    {
                        if(!pMsg->isServoCalibrated[i] && settings_.servoCalibration[i].isCalibrated)
                        {
                            GimbalMsgServoCalibration calib;
                            calib.axisId = i;
                            calib.isCalibrated = 1;
                            calib.data = settings_.servoCalibration[i].data;

                            transceiver_.send(gimbal_protocol::Message(GIMBAL_MSG_TYPE_SERVO_CALIBRATION, calib));

                            LOG(INFO) << "Sent calibration for servo " << i;
                        }
                    }
                }
            }
            break;
            case GIMBAL_MSG_TYPE_SERVO_CALIBRATION:
            {
                auto pMsg = msg.as<GimbalMsgServoCalibration>();
                if(pMsg)
                {
                    LOG(INFO) << "Calibration received for axis " << (uint16_t)pMsg->axisId << ", calibrated: " << (uint16_t)pMsg->isCalibrated;

                    if(pMsg->axisId < settings_.servoCalibration.size() && pMsg->isCalibrated)
                    {
                        auto& calib = settings_.servoCalibration[pMsg->axisId];
                        calib.data = pMsg->data;
                        calib.isCalibrated = true;
                        settings_.save();
                    }
                }
            }
            break;
            default:
                break;
        }
    }
}

bool GimbalController::isConnected() const
{
    using namespace std::chrono_literals;

    return clock_t::now() - tLastMsgReceived_ < 1s;
}

bool GimbalController::isReady() const
{
    return isConnected() && state_.isServoCalibrated[0] && state_.isServoCalibrated[1];
}

float GimbalController::getCurrentPositionRaw_deg(uint8_t axisId) const
{
    if(axisId == 0)
        return -RAD_TO_DEG(state_.motion.encoderPosition_rad[0]);

    return RAD_TO_DEG(state_.motion.encoderPosition_rad[1]);
}

void GimbalController::startCalibration(uint8_t axisId, float testVoltage_V)
{
    GimbalMsgTaskCalibrate calib;
    calib.axisId = axisId;
    calib.testVoltage_mV = std::lround(testVoltage_V * 1000.0f);

    transceiver_.send(gimbal_protocol::Message(GIMBAL_MSG_TYPE_TASK_CALIBRATE, calib));
}

void GimbalController::setTargetPos(float pan_deg, float tilt_deg)
{
    GimbalMsgTaskMove move;
    move.isPosMode = 1;
    move.target[0] = -DEG_TO_RAD(pan_deg + settings_.cameraPose.axisZeroOffsets_deg[0]);
    move.target[1] = DEG_TO_RAD(tilt_deg + settings_.cameraPose.axisZeroOffsets_deg[1]);
    move.velMax_radDs = DEG_TO_RAD(settings_.limits.velMax_degDs);
    move.accMax_radDs2 = DEG_TO_RAD(settings_.limits.accMax_degDs2);
    move.jerkMax_radDs3 = DEG_TO_RAD(settings_.limits.jerkMax_degDs3);

    transceiver_.send(gimbal_protocol::Message(GIMBAL_MSG_TYPE_TASK_MOVE, move));
}

void GimbalController::setTargetVel(float pan_degDs, float tilt_degDs)
{
    GimbalMsgTaskMove move;
    move.isPosMode = 0;
    move.target[0] = -DEG_TO_RAD(pan_degDs);
    move.target[1] = DEG_TO_RAD(tilt_degDs);
    move.velMax_radDs = DEG_TO_RAD(settings_.limits.velMax_degDs);
    move.accMax_radDs2 = DEG_TO_RAD(settings_.limits.accMax_degDs2);
    move.jerkMax_radDs3 = DEG_TO_RAD(settings_.limits.jerkMax_degDs3);

    transceiver_.send(gimbal_protocol::Message(GIMBAL_MSG_TYPE_TASK_MOVE, move));
}

void GimbalController::disableMotors()
{
    transceiver_.send(gimbal_protocol::Message(GIMBAL_MSG_TYPE_TASK_RESET));
}
