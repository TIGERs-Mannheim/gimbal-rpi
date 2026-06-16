/**
Raspberry Pi Pinout:

Peripheral  │ RPi Pin  │ 40-pin Header │ RPi Pin │ Peripheral
────────────┼──────────┼───────────────┼─────────┼──────────────
DISP-3V3    │ 3V3      │    1     2    │ 5V      │ MCU Supply
-           │ GPIO 2   │    3     4    │ 5V      │ MCU Supply
-           │ GPIO 3   │    5     6    │ GND     │ GND
-           │ GPIO 4   │    7     8    │ GPIO 14 │ RPI-TX/MCU-RX
            │ GND      │    9    10    │ GPIO 15 │ RPI-RX/MCU-TX
-           │ GPIO 17  │   11    12    │ GPIO 18 │ -
DISP-RST    │ GPIO 27  │   13    14    │ GND     │
MCU-NRST    │ GPIO 22  │   15    16    │ GPIO 23 │ MCU-BOOT0
DISP-3V3    │ 3V3      │   17    18    │ GPIO 24 │ DISP-BL
DISP-MOSI   │ GPIO 10  │   19    20    │ GND     │
-           │ GPIO 9   │   21    22    │ GPIO 25 │ DISP-DC
DISP-SCLK   │ GPIO 11  │   23    24    │ GPIO 8  │ DISP-CSI
            │ GND      │   25    26    │ GPIO 7  │ -
-           │ GPIO 0   │   27    28    │ GPIO 1  │ -
DISP-JLEFT  │ GPIO 5   │   29    30    │ GND     │
DISP-JUP    │ GPIO 6   │   31    32    │ GPIO 12 │ -
DISP-JPRESS │ GPIO 13  │   33    34    │ GND     │
DISP-JDOWN  │ GPIO 19  │   35    36    │ GPIO 16 │ DISP-KEY3
DISP-JRIGHT │ GPIO 26  │   37    38    │ GPIO 20 │ DISP-KEY2
            │ GND      │   39    40    │ GPIO 21 │ DISP-KEY1

MCU is supplied via LDO on 5V supply. Display is supplied by 3.3V from Raspberry Pi.
*/

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
