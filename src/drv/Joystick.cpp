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

    if(left_.read())
        pan_deg_ -= speed_;

    if(right_.read())
        pan_deg_ += speed_;

    if(up_.read())
        tilt_deg_ -= speed_;

    if(down_.read())
        tilt_deg_ += speed_;

    if(pan_deg_ > PAN_LIMIT)
        pan_deg_ = PAN_LIMIT;
    else if(pan_deg_ < -PAN_LIMIT)
        pan_deg_ = -PAN_LIMIT;

    if(tilt_deg_ > TILT_LIMIT)
        tilt_deg_ = TILT_LIMIT;
    else if(tilt_deg_ < -TILT_LIMIT)
        tilt_deg_ = -TILT_LIMIT;

    const bool pressed = press_.read();
    const auto tNow = std::chrono::high_resolution_clock::now();

    if(isPressed_ && !pressed && tNow - tPressStart_ < 200ms)
    {
        if(speed_ == SPEED_SLOW)
            speed_ = SPEED_FAST;
        else
            speed_ = SPEED_SLOW;
    }

    if(!isPressed_ && pressed)
        tPressStart_ = tNow;

    isPressed_ = pressed;

    if(isPressed_ && tNow - tPressStart_ > 2s)
    {
        pan_deg_ = 0.0f;
        tilt_deg_ = 0.0f;
    }
}
