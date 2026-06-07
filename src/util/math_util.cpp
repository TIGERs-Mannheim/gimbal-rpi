#include "math_util.hpp"

float limitToRange(float in, std::array<float, 2> limits)
{
    if(in < limits[0])
        in = limits[0];
    else if(in > limits[1])
        in = limits[1];

    return in;
}