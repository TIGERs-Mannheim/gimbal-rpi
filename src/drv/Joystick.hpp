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

private:
    RPiGPIO left_;
    RPiGPIO right_;
    RPiGPIO up_;
    RPiGPIO down_;
    RPiGPIO press_;

    float pan_deg_ = 0.0f;
    float tilt_deg_ = 0.0f;
    float speed_ = SPEED_SLOW;

    bool isPressed_ = false;
    std::chrono::high_resolution_clock::time_point tPressStart_;

    static constexpr float PAN_LIMIT = 100.0f;
    static constexpr float TILT_LIMIT = 45.0f;

    static constexpr float SPEED_SLOW = 0.01f;
    static constexpr float SPEED_FAST = 0.05f;
};
