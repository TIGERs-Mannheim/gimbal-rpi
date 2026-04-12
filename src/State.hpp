#pragma once

struct State
{
    struct
    {
        float pos_m[3] = {};
        float visibility = 0.0f;
    } ball;

    struct
    {
        bool isReady = false;
        bool isMoving = false;
        float pan_deg = 0.0f;
        float tilt_deg = 0.0f;
        float velMax_degDs = 0.0f;
        float accMax_degDs2 = 0.0f;
    } camera;
};
