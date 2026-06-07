#pragma once

#include <cmath>
#include <array>

#ifndef M_PIf
#define M_PIf ((float)M_PI)
#endif

#define RAD_TO_DEG(rad) ((rad)*180.0f/M_PIf)
#define DEG_TO_RAD(deg) ((deg)*M_PIf/180.0f)

float limitToRange(float in, std::array<float, 2> limits);
