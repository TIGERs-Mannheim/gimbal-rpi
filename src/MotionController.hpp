#pragma once

#include "StepperAxis.hpp"

#include <thread>
#include <atomic>
#include <mutex>

class MotionController
{
public:
    MotionController(std::shared_ptr<ISerialPort> pSerialPort);
    ~MotionController();

    void spinOnce();
    void setTargetPos(float pan_deg, float tilt_deg);
    void setVelMax(float velMax_degDs) { velMax_degDs_ = velMax_degDs; }
    void setAccMax(float accMax_degDs2) { accMax_degDs2_ = accMax_degDs2; }
    void resetHome() { state_ = STOP_MOVEMENT; }

    float getVelMax() const { return velMax_degDs_; }
    float getAccMax() const { return accMax_degDs2_; }
    float getTargetPan() const { return targetPos_deg_[0]; }
    float getTargetTilt() const { return targetPos_deg_[1]; }
    float getCurrentPan() const { return currentPos_deg_[0]; }
    float getCurrentTilt() const { return currentPos_deg_[1]; }

    bool isReady() const { return state_ == READY; }
    bool areStepperDriversConnected() const { return panAxis_.isStepperDriverConnected() && tiltAxis_.isStepperDriverConnected(); }
    bool isMoving() const { return isMoving_; }
    bool areDriversHot() const { return panAxis_.isStepperDriverHot() || tiltAxis_.isStepperDriverHot(); }

    static constexpr float MAX_VEL = 800.0f;
    static constexpr float MAX_ACC = 4000.0f;

private:
    enum State
    {
        DISCONNECTED,
        STOP_MOVEMENT,
        HOMING_GO_TO_LIMIT,
        HOMING_GO_TO_CENTER,
        READY,
    };

    void outputControl();

    bool initSuccess_ = false;
    State state_ = DISCONNECTED;

    std::shared_ptr<ISerialPort> pSerialPort_;

    StepperAxis panAxis_;
    StepperAxis tiltAxis_;
    RPiGPIO enableNotPin_;

    std::thread controlThread_;
    std::atomic<bool> runControlThread_ = true;

    double currentPos_deg_[2] = { 0.0, 0.0 };
    double currentVel_degDs_[2] = { 0.0, 0.0 };
    double currentAcc_degDs2_[2] = { 0.0, 0.0 };

    float velMax_degDs_ = 800.0f;
    float accMax_degDs2_ = 4000.0f;

    float targetPos_deg_[2] = { 0.0f, 0.0f };
    std::mutex targetPosMutex_;

    std::atomic<bool> isMoving_ = false;
    std::atomic<bool> zeroPosition_ = false;

    int32_t currentStepPos_[2] = { 0, 0 };

    static constexpr float POS_SPAN[2] = { 109.0f*2.0f, 49.0f*2.0f };
    static constexpr float STEPS_PER_REVOLUTION = 16.0f * 200.0f;
    static constexpr float DEG_TO_STEPS = STEPS_PER_REVOLUTION / 360.0f;
    static constexpr float MAX_STEP_FREQUENCY = 10e3f;
    static constexpr float MAX_VEL_DEG_S = MAX_STEP_FREQUENCY / STEPS_PER_REVOLUTION * 360.0f;
};
