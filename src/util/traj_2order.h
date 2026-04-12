#pragma once

#define TRAJ_2ORDER_PARTS 3

typedef struct
{
    float tEnd;

    // xdd(t) = xdd
    // xd(t) = xd0 + xdd*t
    // x(t) = x0 + xd0*t + 0.5*xdd*t^2
    float xdd;  // x dot dot, control signal
    float xd0; // x dot initial value
    float x0; // x initial value
} TrajSecOrderPart;

typedef struct
{
    TrajSecOrderPart parts[TRAJ_2ORDER_PARTS];
} TrajSecOrder1D;

typedef struct
{
    TrajSecOrderPart x[TRAJ_2ORDER_PARTS];
    TrajSecOrderPart y[TRAJ_2ORDER_PARTS];
} TrajSecOrder2D;

#ifdef __cplusplus
extern "C" {
#endif

void TrajSecOrder1DCreate(TrajSecOrder1D* pTraj, float x0, float xd0, float xTrg, float xdMax, float xddMax);
void TrajSecOrder1DValuesAtTime(const TrajSecOrder1D* pTraj, float t, float* pX, float* pXd, float* pXdd);
float TrajSecOrder1DGetTotalTime(const TrajSecOrder1D* pTraj);
float TrajSecOrder1DGetFinalX(const TrajSecOrder1D* pTraj);

void TrajSecOrder2DCreate(TrajSecOrder2D* pTraj, const float* pX0, const float* pXd0, const float* pXTrg, float xdMax, float xddMax);
void TrajSecOrder2DValuesAtTime(const TrajSecOrder2D* pTraj, float t, float* pX, float* pXd, float* pXdd);
float TrajSecOrder2DGetTotalTime(const TrajSecOrder2D* pTraj);

#ifdef __cplusplus
}
#endif
