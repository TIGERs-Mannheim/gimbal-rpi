#include "traj_2order.h"
#include <stdint.h>
#include <math.h>

#define FIND_XY_DISTRI_PRECISION 0.0000001

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    double xdd;
    double xd;
    double x;
} TrajSecOrder1DValue;

static void partValues(const TrajSecOrderPart* pParts, uint8_t numParts, double trajTime, TrajSecOrder1DValue* pVal)
{
    uint8_t i;
    double t;
    const TrajSecOrderPart* pPiece = 0;
    double tPieceStart = 0;

    for(i = 0; i < numParts; i++)
    {
        pPiece = &pParts[i];
        if(trajTime < pPiece->tEnd)
            break;
    }

    if(i == numParts)
    {
        t = pPiece->tEnd - pParts[numParts - 2].tEnd; // trajectory complete, use end time
        pVal->xdd = 0;
    }
    else
    {
        if(i > 0)
            tPieceStart = pParts[i - 1].tEnd;

        t = trajTime - tPieceStart;
        pVal->xdd = pPiece->xdd;
    }

    pVal->xd = pPiece->xd0 + pPiece->xdd * t;
    pVal->x = pPiece->x0 + pPiece->xd0 * t + 0.5f * pPiece->xdd * t * t;
}

static double velChangeToZero(double s0, double v0, double aMax)
{
    double a;

    if(0 >= v0)
        a = aMax;
    else
        a = -aMax;

    double t = -v0 / a;
    double s1 = s0 + 0.5f * v0 * t;

    return s1;
}

static double velTriToZero(double s0, double v0, double v1, double aMax)
{
    double a1;
    double a2;

    if(v1 >= v0)
    {
        a1 = aMax;
        a2 = -aMax;
    }
    else
    {
        a1 = -aMax;
        a2 = aMax;
    }

    double t1 = (v1 - v0) / a1;
    double s1 = s0 + 0.5f * (v0 + v1) * t1;

    double t2 = -v1 / a2;
    double s2 = s1 + 0.5f * v1 * t2;

    return s2;
}

static void calcTri(TrajSecOrder1D* pTraj,
                    double s0, double v0, double s2, double a, uint8_t isPos)
{
    double T2;
    double v1;
    double T1;
    double s1;

    if(isPos)
    {
        // + -
        double sq = (a * (s2 - s0) + 0.5f * v0 * v0) / (a * a);
        if(sq > 0.0f)
            T2 = sqrtf(sq);
        else
            T2 = 0;
        v1 = a * T2;
        T1 = (v1 - v0) / a;
        s1 = s0 + (v0 + v1) * 0.5f * T1;
    }
    else
    {
        // - +
        double sq = (a * (s0 - s2) + 0.5f * v0 * v0) / (a * a);
        if(sq > 0.0f)
            T2 = sqrtf(sq);
        else
            T2 = 0;
        v1 = -a * T2;
        T1 = (v1 - v0) / -a;
        s1 = s0 + (v0 + v1) * 0.5f * T1;
        a *= -1;
    }

    pTraj->parts[0].tEnd = T1;
    pTraj->parts[0].xdd = a;
    pTraj->parts[0].xd0 = v0;
    pTraj->parts[0].x0 = s0;

    pTraj->parts[1].tEnd = T1;
    pTraj->parts[1].xdd = a;
    pTraj->parts[1].xd0 = v1;
    pTraj->parts[1].x0 = s1;

    pTraj->parts[2].tEnd = T1 + T2;
    pTraj->parts[2].xdd = -a;
    pTraj->parts[2].xd0 = v1;
    pTraj->parts[2].x0 = s1;
}

static void calcTrapz(TrajSecOrder1D* pTraj,
                      double s0, double v0, double v1, double s3, double aMax)
{
    double a1;
    double a3;

    if(v0 > v1)
        a1 = -aMax;
    else
        a1 = aMax;

    if(v1 > 0)
        a3 = -aMax;
    else
        a3 = aMax;

    double T1 = (v1 - v0) / a1;
    double v2 = v1;
    double T3 = -v2 / a3;

    double s1 = s0 + 0.5 * (v0 + v1) * T1;
    double s2 = s3 - 0.5 * v2 * T3;
    double T2 = (s2 - s1) / v1;

    pTraj->parts[0].tEnd = T1;
    pTraj->parts[0].xdd = a1;
    pTraj->parts[0].xd0 = v0;
    pTraj->parts[0].x0 = s0;

    pTraj->parts[1].tEnd = T1 + T2;
    pTraj->parts[1].xdd = 0;
    pTraj->parts[1].xd0 = v1;
    pTraj->parts[1].x0 = s1;

    pTraj->parts[2].tEnd = T1 + T2 + T3;
    pTraj->parts[2].xdd = a3;
    pTraj->parts[2].xd0 = v2;
    pTraj->parts[2].x0 = s2;
}

void TrajSecOrder1DCreate(TrajSecOrder1D* pTraj, double x0, double xd0, double xTrg, double xdMax, double xddMax)
{
    double sAtZeroAcc = velChangeToZero(x0, xd0, xddMax);

    if(sAtZeroAcc <= xTrg)
    {
        double sEnd = velTriToZero(x0, xd0, xdMax, xddMax);

        if(sEnd >= xTrg)
        {
            // Triangular profile
            calcTri(pTraj, x0, xd0, xTrg, xddMax, 1);
        }
        else
        {
            // Trapezoidal profile
            calcTrapz(pTraj, x0, xd0, xdMax, xTrg, xddMax);
        }
    }
    else
    {
        // even with a full brake we miss xTrg
        double sEnd = velTriToZero(x0, xd0, -xdMax, xddMax);

        if(sEnd <= xTrg)
        {
            // Triangular profile
            calcTri(pTraj, x0, xd0, xTrg, xddMax, 0);
        }
        else
        {
            // Trapezoidal profile
            calcTrapz(pTraj, x0, xd0, -xdMax, xTrg, xddMax);
        }
    }
}

void TrajSecOrder1DValuesAtTime(const TrajSecOrder1D* pTraj, double t, double* pX, double* pXd, double* pXdd)
{
    TrajSecOrder1DValue val;

    partValues(pTraj->parts, TRAJ_2ORDER_PARTS, t, &val);

    if(pX)
        *pX = val.x;
    if(pXd)
        *pXd = val.xd;
    if(pXdd)
        *pXdd = val.xdd;
}

double TrajSecOrder1DGetTotalTime(const TrajSecOrder1D* pTraj)
{
    return pTraj->parts[TRAJ_2ORDER_PARTS - 1].tEnd;
}

double TrajSecOrder1DGetFinalX(const TrajSecOrder1D* pTraj)
{
    double t = pTraj->parts[2].tEnd - pTraj->parts[1].tEnd;

    return pTraj->parts[2].x0 + pTraj->parts[2].xd0 * t + 0.5 * pTraj->parts[2].xdd * t * t;
}

void TrajSecOrder2DCreate(TrajSecOrder2D* pTraj, const double* pX0, const double* pXd0, const double* pXTrg, double xdMax, double xddMax)
{
    TrajSecOrder1D* pPosX = (TrajSecOrder1D*)pTraj->x;
    TrajSecOrder1D* pPosY = (TrajSecOrder1D*)pTraj->y;

    double inc = M_PI / 8.0f;
    double alpha = M_PI / 4.0f;

    // binary search, some iterations (fixed)
    while(inc > FIND_XY_DISTRI_PRECISION)
    {
        double cA = cosf(alpha);
        double sA = sinf(alpha);

        TrajSecOrder1DCreate(pPosX, pX0[0], pXd0[0], pXTrg[0], xdMax * cA, xddMax * cA);
        TrajSecOrder1DCreate(pPosY, pX0[1], pXd0[1], pXTrg[1], xdMax * sA, xddMax * sA);

        if(pPosX->parts[TRAJ_2ORDER_PARTS - 1].tEnd > pPosY->parts[TRAJ_2ORDER_PARTS - 1].tEnd)
            alpha = alpha - inc;
        else
            alpha = alpha + inc;

        inc *= 0.5f;
    }
}

void TrajSecOrder2DValuesAtTime(const TrajSecOrder2D* pTraj, double t, double* pX, double* pXd, double* pXdd)
{
    TrajSecOrder1DValue x;
    TrajSecOrder1DValue y;

    partValues(pTraj->x, TRAJ_2ORDER_PARTS, t, &x);
    partValues(pTraj->y, TRAJ_2ORDER_PARTS, t, &y);

    if(pX)
    {
        pX[0] = x.x;
        pX[1] = y.x;
    }

    if(pXd)
    {
        pXd[0] = x.xd;
        pXd[1] = y.xd;
    }

    if(pXdd)
    {
        pXdd[0] = x.xdd;
        pXdd[1] = y.xdd;
    }
}

double TrajSecOrder2DGetTotalTime(const TrajSecOrder2D* pTraj)
{
    double xEnd = pTraj->x[TRAJ_2ORDER_PARTS - 1].tEnd;
    double yEnd = pTraj->y[TRAJ_2ORDER_PARTS - 1].tEnd;

    if(xEnd > yEnd)
        return xEnd;
    else
        return yEnd;
}

#ifdef __cplusplus
}
#endif
