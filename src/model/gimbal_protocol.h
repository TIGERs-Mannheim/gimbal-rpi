#pragma once

#include <stdint.h>

enum State
{
    GIMBAL_STATE_IDLE,
    GIMBAL_STATE_MOVING,
    GIMBAL_STATE_CALIBRATING,
};

// Machine parameters
#define GIMBAL_NUM_AXES 2

#define GIMBAL_AXIS_PAN  0
#define GIMBAL_AXIS_TILT 1

#define GIMBAL_MAX_MSG_SIZE 256

// List of messages
#define GIMBAL_MSG_TYPE_STATE             0
#define GIMBAL_MSG_TYPE_SERVO_CALIBRATION 1
#define GIMBAL_MSG_TYPE_SERVO_PARAMETERS  2

#define GIMBAL_MSG_TYPE_TASK_CALIBRATE 0x0100
#define GIMBAL_MSG_TYPE_TASK_MOVE      0x0101
#define GIMBAL_MSG_TYPE_TASK_RESET     0x0102

#pragma pack(push, 1)

// Important: structs below must be manually padded so that floats are at a 4 byte boundary to avoid unaligned access!

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
    uint8_t state;

    struct
    {
        uint16_t supplyVcc_mV;
    } power;

    uint8_t isServoCalibrated[GIMBAL_NUM_AXES];

    struct
    {
        float encoderPosition_rad[GIMBAL_NUM_AXES];
        float encoderVelocityLP_rad[GIMBAL_NUM_AXES];
        float currentQLP_A[GIMBAL_NUM_AXES];
        uint16_t encoderErrorFlags[GIMBAL_NUM_AXES];
        uint16_t motorErrorFlags[GIMBAL_NUM_AXES];
    } motion;

    struct
    {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
        uint8_t dirty;
        uint32_t sha1;
    } version;
} GimbalMsgState;

typedef struct
{
    uint8_t axisId;
    uint16_t testVoltage_mV;
} GimbalMsgTaskCalibrate;

typedef struct
{
    float target[GIMBAL_NUM_AXES];
    float velMax_radDs;
    float accMax_radDs2;
    float jerkMax_radDs3;
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
        float outputBandwidth_Hz;
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
