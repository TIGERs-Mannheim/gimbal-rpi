#include "MotionController.hpp"
#include "easylogging++.h"
#include <cmath>

MotionController::MotionController(std::shared_ptr<ISerialPort> pSerialPort)
: panAxis_(pSerialPort, 1, "GPIO3", "GPIO2"),
  tiltAxis_(pSerialPort, 0, "GPIO7", "GPIO9"),
  enableNotPin_("GPIO12", RPiGPIO::OUTPUT)
{
    enableNotPin_.write(true);
    controlThread_ = std::thread(&MotionController::outputControl, this);

    initSuccess_ = true;
}

MotionController::~MotionController()
{
    if(controlThread_.joinable())
    {
        runControlThread_ = false;
        controlThread_.join();
    }

    enableNotPin_.write(true);
}

void MotionController::spinOnce()
{
    panAxis_.spinOnce();
    tiltAxis_.spinOnce();

    if(areStepperDriversConnected())
    {
        enableNotPin_.write(false);
    }
    else
    {
        enableNotPin_.write(true);
        state_ = DISCONNECTED;
    }

    switch(state_)
    {
        case DISCONNECTED:
        {
            if(areStepperDriversConnected())
            {
                state_ = STOP_MOVEMENT;
            }
        }
        break;
        case STOP_MOVEMENT:
        {
            if(!isMoving())
            {
                setVelMax(100);
                setAccMax(1000);
                setTargetPos(-POS_SPAN[0], -POS_SPAN[1]);
                state_ = HOMING_GO_TO_LIMIT;
            }
        }
        break;
        case HOMING_GO_TO_LIMIT:
        {
            if(!isMoving())
            {
                setTargetPos(POS_SPAN[0]*-0.5f, POS_SPAN[1]*-0.5f);
                state_ = HOMING_GO_TO_CENTER;
            }
        }
        break;
        case HOMING_GO_TO_CENTER:
        {
            if(!isMoving())
            {
                zeroPosition_ = true;
                accMax_degDs2_ = MAX_ACC;
                velMax_degDs_ = MAX_VEL;
                state_ = READY;
            }
        }
        break;
        case READY:
            break;
    }
}

void MotionController::setTargetPos(float pan_deg, float tilt_deg)
{
    auto lock = std::unique_lock<std::mutex>(targetPosMutex_);
    targetPos_deg_[0] = pan_deg;
    targetPos_deg_[1] = tilt_deg;
}

void MotionController::outputControl()
{
    // Pin this thread to core 3, which should be isolated by cmdline.txt, to reduce jitter
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);

    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if(rc != 0)
    {
        LOG(ERROR) << "Failed to pin core: %s" << strerror(errno);
        return;
    }

    // Precise loop for step pulse generation
    auto interval = std::chrono::microseconds(std::lround(1e6f / MAX_STEP_FREQUENCY));
    auto nextWakeTime = std::chrono::high_resolution_clock::now() + interval;

    while(runControlThread_)
    {
        float targetPos[2];

        {
            auto lock = std::unique_lock<std::mutex>(targetPosMutex_);

            if(zeroPosition_)
            {
                targetPos_deg_[0] = 0.0f;
                targetPos_deg_[1] = 0.0f;
                currentStepPos_[0] = 0.0f;
                currentStepPos_[1] = 0.0f;
                currentPos_deg_[0] = 0.0f;
                currentPos_deg_[1] = 0.0f;
                zeroPosition_ = false;
            }

            targetPos[0] = targetPos_deg_[0];
            targetPos[1] = targetPos_deg_[1];
        }

        float currentAcc_degDs2[2];

        TrajSecOrder2DCreate(&trajectory_, currentPos_deg_, currentVel_degDs_, targetPos, velMax_degDs_, accMax_degDs2_);
        TrajSecOrder2DValuesAtTime(&trajectory_, 0.1e-3f, currentPos_deg_, currentVel_degDs_, currentAcc_degDs2);

        isMoving_ = fabsf(currentPos_deg_[0] - targetPos[0]) > 1.0f/DEG_TO_STEPS || fabsf(currentPos_deg_[1] - targetPos[1]) > 1.0f/DEG_TO_STEPS;

        int32_t stepTarget[2];
        stepTarget[0] = std::round(currentPos_deg_[0] * DEG_TO_STEPS);
        stepTarget[1] = std::round(currentPos_deg_[1] * DEG_TO_STEPS);

        int32_t diff[2];
        diff[0] = std::abs(currentStepPos_[0] - stepTarget[0]);
        diff[1] = std::abs(currentStepPos_[1] - stepTarget[1]);

        if(diff[0] > 1 || diff[1] > 1)
        {
            LOG(WARNING) << "Over stepped! Pan: " << diff[0] << ", tilt: " << diff[1];
        }

        if(currentStepPos_[0] < stepTarget[0])
        {
            panAxis_.step(true);
            currentStepPos_[0]++;
        }
        else if(currentStepPos_[0] > stepTarget[0])
        {
            panAxis_.step(false);
            currentStepPos_[0]--;
        }

        if(currentStepPos_[1] < stepTarget[1])
        {
            tiltAxis_.step(true);
            currentStepPos_[1]++;
        }
        else if(currentStepPos_[1] > stepTarget[1])
        {
            tiltAxis_.step(false);
            currentStepPos_[1]--;
        }

        std::this_thread::sleep_until(nextWakeTime);
        nextWakeTime += interval;
    }
}
