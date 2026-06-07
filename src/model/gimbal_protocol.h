#pragma once

#include <stdint.h>

enum AxisId
{
    AXIS_ID_PAN,
    AXIS_ID_TILT,
    AXIS_ID_SIZE,
};

// Machine parameters
#define MACHINE_NUM_AXES AXIS_ID_SIZE

#define MACHINE_AXIS_PAN  AXIS_ID_PAN
#define MACHINE_AXIS_TILT AXIS_ID_TILT

// List of messages
#define GIMBAL_MSG_TYPE_STATE             0
#define GIMBAL_MSG_TYPE_SERVO_CALIBRATION 1
#define GIMBAL_MSG_TYPE_SERVO_PARAMETERS  2

#define GIMBAL_MSG_TYPE_TASK_CALIBRATE 0x0100
#define GIMBAL_MSG_TYPE_TASK_MOVE      0x0101
#define GIMBAL_MSG_TYPE_TASK_RESET     0x0102

#pragma pack(push, 1)

typedef struct
{
    uint16_t version;
    uint16_t type;
} GimbalMsgHeader;

typedef struct
{
    uint32_t checksum;
} GimbalMsgTrailer;

typedef struct
{
    uint8_t cpuLoad; // 0 - 100

    struct
    {
        uint8_t isActive;
        uint16_t id;
        int16_t state;
    } task;

    struct
    {
        uint16_t supplyVcc_mV;
    } power;

    uint8_t isServoCalibrated[MACHINE_NUM_AXES];

    struct
    {
        float encoderPosition_rad[MACHINE_NUM_AXES];
    } motion;
} GimbalMsgState;

typedef struct
{
    uint8_t axisId;
    uint16_t testVoltage_mV;
} GimbalMsgTaskCalibrate;

typedef struct
{
    float target[MACHINE_NUM_AXES];
    float velMax_radDs;
    float accMax_radDs2;
    float jerkMax_radDs3;
    uint8_t isPosMode;
} GimbalMsgTaskMove;

typedef struct
{
    float motorPhaseCurrentOffsetU_V;
    float motorPhaseCurrentOffsetV_V;
    float motorPhaseCurrentOffsetW_V;

    float phaseResistance_R;
    float phaseInductance_H;

    int16_t encoderPosElectricalZero;
    uint8_t motorInvertDirection;
} GimbalServoCalibration;

typedef struct
{
    GimbalServoCalibration data;

    uint8_t axisId;
    uint8_t isCalibrated;
} GimbalMsgServoCalibration;

typedef struct
{
    struct
    {
        float bandwidth_Hz;
        float outputRamp_VDs;
    } current;

    struct
    {
        float lowPassBandwidth_Hz;

        float Kp;
        float Ki;
        float Kd;
        float outputRamp_ADs;
        float outputLimit_A;
    } velocity;

    struct
    {
        float Kp;
        float Ki;
        float Kd;
        float outputRamp_radDs2;
        float outputLimit_radDs;
    } position;
} GimbalServoParameters;

typedef struct
{
    GimbalServoParameters data;

    uint8_t axisId;
} GimbalMsgServoParameters;

#pragma pack(pop)
