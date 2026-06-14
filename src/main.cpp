#include "easylogging++.h"
#include <thread>
#include "TrackingCamera.hpp"
#include <lvgl.h>
#include <chrono>

using namespace std::chrono_literals;

INITIALIZE_EASYLOGGINGPP

bool runFlag = true;

void signalHandler(int)
{
    runFlag = false;
}

std::chrono::steady_clock::time_point startTime;

uint32_t getMillisecondsSinceStart()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
}

int main(int, char**)
{
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime{%y-%M-%d %H:%m:%s.%g} [%levshort] %msg");
    el::Loggers::setDefaultConfigurations(defaultConf, true);

    LOG(INFO) << "Starting TIGERs Tracking Camera";

    lv_init();
    lv_tick_set_cb(&getMillisecondsSinceStart);

    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    pthread_sigmask (SIG_UNBLOCK, &signal_mask, nullptr);

    signal(SIGINT, &signalHandler);

    TrackingCamera cam;

    std::chrono::steady_clock::time_point tLastReport = std::chrono::steady_clock::now();
    float maxRuntime_us = 0.0f;
    float runtimeSum_us = 0.0f;
    float runtimeSamples = 0;

    auto tLastCall = std::chrono::steady_clock::now();

    try
    {
        while(runFlag)
        {
            auto tStart = std::chrono::high_resolution_clock::now();

            int result = cam.spinOnce();
            if(result)
                return result;

            auto tEnd = std::chrono::high_resolution_clock::now();

            float runtime_us = (tEnd - tStart) / std::chrono::microseconds(1);
            maxRuntime_us = std::max(maxRuntime_us, runtime_us);
            runtimeSum_us += runtime_us;
            runtimeSamples++;

            auto tNow = std::chrono::steady_clock::now();
            if(tNow - tLastReport > std::chrono::seconds(1))
            {
                tLastReport = tNow;

                // LOG(INFO) << "Avg: " << runtimeSum_us / runtimeSamples << "us, max: " << maxRuntime_us << "us, samples: " << runtimeSamples;
                maxRuntime_us = 0.0f;
                runtimeSum_us = 0.0f;
                runtimeSamples = 0;
            }

            std::this_thread::sleep_until(tLastCall+10ms);
            tLastCall = std::chrono::steady_clock::now();
        }
    }
    catch(std::exception& ex)
    {
        LOG(ERROR) << "Caught exception: " << ex.what();
        return 1;
    }
    catch(...)
    {
        return 2;
    }

    return 0;
}
