#include "TrajPolynomialQuintic.hpp"

TrajPolynomialQuintic::TrajPolynomialQuintic(double x0, double xd0, double xdd0, double xTrg, double xdTrg, double xddTrg, double tEnd_)
{
    tEnd = tEnd_;

    a[0] = x0;
    a[1] = xd0;
    a[2] = xdd0/2.0;
    a[3] = (20*xTrg - 20*x0 - (8*xdTrg + 12*xd0)*tEnd - (3*xdd0 - xddTrg)*tEnd*tEnd) / (2*tEnd*tEnd*tEnd);
    a[4] = (30*x0 - 30*xTrg + (14*xdTrg + 16*xd0)*tEnd + (3*xdd0 - 2*xddTrg)*tEnd*tEnd) / (2*tEnd*tEnd*tEnd*tEnd);
    a[5] = (12*xTrg - 12*x0 - (6*xdTrg + 6*xd0)*tEnd - (xdd0 - xddTrg)*tEnd*tEnd) / (2*tEnd*tEnd*tEnd*tEnd*tEnd);
}

void TrajPolynomialQuintic::getValuesAtTime(double t, double* pX, double* pXd, double* pXdd)
{
    if(t > tEnd)
        t = tEnd;

    if(pX)
        *pX = a[0] + a[1]*t + a[2]*t*t + a[3]*t*t*t + a[4]*t*t*t*t + a[5]*t*t*t*t*t;

    if(pXd)
        *pXd = a[1] + 2*a[2]*t + 3*a[3]*t*t + 4*a[4]*t*t*t + 5*a[5]*t*t*t*t;

    if(pXdd)
        *pXdd = 2*a[2] + 6*a[3]*t + 12*a[4]*t*t + 20*a[5]*t*t*t;
}
