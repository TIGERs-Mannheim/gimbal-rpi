#pragma once

#define TRAJ_2ORDER_PARTS 3

typedef struct
{
    double tEnd;

    // xdd(t) = xdd
    // xd(t) = xd0 + xdd*t
    // x(t) = x0 + xd0*t + 0.5*xdd*t^2
    double xdd;  // x dot dot, control signal
    double xd0; // x dot initial value
    double x0; // x initial value
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

void TrajSecOrder1DCreate(TrajSecOrder1D* pTraj, double x0, double xd0, double xTrg, double xdMax, double xddMax);
void TrajSecOrder1DValuesAtTime(const TrajSecOrder1D* pTraj, double t, double* pX, double* pXd, double* pXdd);
double TrajSecOrder1DGetTotalTime(const TrajSecOrder1D* pTraj);
double TrajSecOrder1DGetFinalX(const TrajSecOrder1D* pTraj);

void TrajSecOrder2DCreate(TrajSecOrder2D* pTraj, const double* pX0, const double* pXd0, const double* pXTrg, double xdMax, double xddMax);
void TrajSecOrder2DValuesAtTime(const TrajSecOrder2D* pTraj, double t, double* pX, double* pXd, double* pXdd);
double TrajSecOrder2DGetTotalTime(const TrajSecOrder2D* pTraj);

#ifdef __cplusplus
}
#endif
