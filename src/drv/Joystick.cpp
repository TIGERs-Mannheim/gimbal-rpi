#include "Joystick.hpp"

Joystick::Joystick()
: left_("GPIO26", RPiGPIO::INPUT, RPiGPIO::PULL_UP, true),
  right_("GPIO5", RPiGPIO::INPUT, RPiGPIO::PULL_UP, true),
  up_("GPIO19", RPiGPIO::INPUT, RPiGPIO::PULL_UP, true),
  down_("GPIO6", RPiGPIO::INPUT, RPiGPIO::PULL_UP, true),
  press_("GPIO13", RPiGPIO::INPUT, RPiGPIO::PULL_UP, true)
{
}

void Joystick::spinOnce()
{
    using namespace std::chrono_literals;

    const auto tNow = std::chrono::high_resolution_clock::now();
    float dt_s = ((tNow - tLastSpin_) / 1us) * 1e-6f;
    if(dt_s > 1.0f)
        dt_s = 1.0f;

    if(left_.read())
        pan_deg_ += speed_ * dt_s;

    if(right_.read())
        pan_deg_ -= speed_ * dt_s;

    if(up_.read())
        tilt_deg_ -= speed_ * dt_s;

    if(down_.read())
        tilt_deg_ += speed_ * dt_s;

    const bool pressed = press_.read();

    if(isPressed_ && !pressed && tNow - tPressStart_ < 200ms)
    {
        if(speed_ == SPEED_SLOW_DEG_D_S)
            speed_ = SPEED_FAST_DEG_D_S;
        else
            speed_ = SPEED_SLOW_DEG_D_S;
    }

    if(!isPressed_ && pressed)
        tPressStart_ = tNow;

    isPressed_ = pressed;

    if(isPressed_ && tNow - tPressStart_ > 2s)
    {
        pan_deg_ = 0.0f;
        tilt_deg_ = 0.0f;
    }

    tLastSpin_ = tNow;
}

void Joystick::setPosition(uint8_t axisId, float pos_deg)
{
    if(axisId == 0)
        pan_deg_ = pos_deg;
    else
        tilt_deg_ = pos_deg;
}
