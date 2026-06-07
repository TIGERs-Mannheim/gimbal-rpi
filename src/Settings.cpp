#include "Settings.hpp"
#include "easylogging++.h"

#include <fstream>

Settings::Settings()
{
    for(auto& calib : servoParameters)
        calib = getDefaultServoParameters();
}

Settings::Settings(const std::string& filename)
:filename(filename)
{
    for(auto& calib : servoParameters)
        calib = getDefaultServoParameters();

    load();
}

bool Settings::load()
{
    try
    {
        std::ifstream input(filename);
        if(!input.good())
            return false;

        nlohmann::from_json(nlohmann::json::parse(input), *this);
    }
    catch(nlohmann::json::exception& ex)
    {
        LOG(ERROR) << "Failed to load: " << filename << ", error: " << ex.what();
    }

    return true;
}

void Settings::save()
{
    std::ofstream output(filename);
    if(!output.good())
        return;

    nlohmann::json json;
    nlohmann::to_json(json, *this);
    output << json.dump(4);
}

GimbalServoParameters Settings::getDefaultServoParameters()
{
    GimbalServoParameters params;

    params.current.bandwidth_Hz = 2000.0f;
    params.current.outputRamp_VDs = 0.0f;
    params.velocity.lowPassBandwidth_Hz = 10.0f;
    params.velocity.Kp = 0.2f;
    params.velocity.Ki = 0.2f;
    params.velocity.Kd = 0.0f;
    params.velocity.outputLimit_A = 0.8f;
    params.velocity.outputRamp_ADs = 0.0f;
    params.position.Kp = 20.0f;
    params.position.Ki = 0.0f;
    params.position.Kd = 0.0f;
    params.position.outputLimit_radDs = 2.0f * M_PIf * 2.0f;
    params.position.outputRamp_radDs2 = 0.0f;

    return params;
}
