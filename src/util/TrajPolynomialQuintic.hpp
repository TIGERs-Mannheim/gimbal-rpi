#pragma once

class TrajPolynomialQuintic
{
public:
    TrajPolynomialQuintic(double x0, double xd0, double xdd0, double xTrg, double xdTrg, double xddTrg, double tEnd_);

    void getValuesAtTime(double t, double* pX, double* pXd, double* pXdd);

private:
    double a[6];
    double tEnd;
};
