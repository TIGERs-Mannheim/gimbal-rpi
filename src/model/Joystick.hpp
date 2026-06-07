#pragma once

#include "RPiGPIO.hpp"
#include <chrono>

class Joystick
{
public:
    Joystick();

    void spinOnce();

    float getPan_deg() const { return pan_deg_; }
    float getTilt_deg() const { return tilt_deg_; }

    void setPosition(uint8_t axisId, float pos_deg);

private:
    RPiGPIO left_;
    RPiGPIO right_;
    RPiGPIO up_;
    RPiGPIO down_;
    RPiGPIO press_;

    float pan_deg_ = 0.0f;
    float tilt_deg_ = 0.0f;
    float speed_ = SPEED_SLOW_DEG_D_S;

    bool isPressed_ = false;
    std::chrono::high_resolution_clock::time_point tPressStart_;

    std::chrono::high_resolution_clock::time_point tLastSpin_;

    static constexpr float SPEED_SLOW_DEG_D_S = 10.0f;
    static constexpr float SPEED_FAST_DEG_D_S = 100.0f;
};
