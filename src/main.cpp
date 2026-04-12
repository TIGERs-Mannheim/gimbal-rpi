#include "easylogging++.h"
// #include "util/RPiGPIO.hpp"
#include <thread>
// #include "SerialPort.hpp"
// #include "drv/TMC2209.hpp"
// #include "MotionController.hpp"
#include "TrackingCamera.hpp"
#include <eigen3/Eigen/Geometry>

INITIALIZE_EASYLOGGINGPP

bool runFlag = true;

void signalHandler(int)
{
    runFlag = false;
}

int main(int, char**)
{
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime{%y-%M-%d %H:%m:%s.%g} [%levshort] %msg");
    el::Loggers::setDefaultConfigurations(defaultConf, true);

    LOG(INFO) << "Starting TIGERs Tracking Camera";

    // Testing Eigen
    // Eigen::Translation3f field_p_base(-1.0f, 5.0f, 1.6f);
    // float field_rz_base = -M_PI/2.0f;
    // // float field_rz_base = -2.0f;
    //
    // Eigen::Affine3f field_T_base = field_p_base * Eigen::AngleAxisf(field_rz_base, Eigen::Vector3f::UnitZ());
    // auto base_T_field = field_T_base.inverse();
    //
    // Eigen::Vector3f field_p_ball(0.0f, 0.0f, 0.0f);
    //
    // auto base_p_ball = base_T_field * field_p_ball;
    //
    // LOG(INFO) << "base_p_ball: " << base_p_ball.x() << ", " << base_p_ball.y() << ", " << base_p_ball.z();
    //
    // float pan_deg = atan2f(base_p_ball.y(), base_p_ball.x()) * 180.0f / M_PI;
    // float tilt_deg = atan2f(base_p_ball.z(), base_p_ball.x()) * 180.0f / M_PI;
    //
    // LOG(INFO) << "pan: " << pan_deg << ", tilt: " << tilt_deg;
    //
    //
    //
    // float base_pan_cam = -M_PI/4.0f;
    // float base_tilt_cam = M_PI/4.0f;
    //
    // Eigen::Affine3f base_R_cam(Eigen::AngleAxisf(base_pan_cam, Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(base_tilt_cam, Eigen::Vector3f::UnitY()));
    //
    // Eigen::Vector3f cam_p(1.0f, 0.0f, 0.0f);
    //
    // auto base_p = base_R_cam * cam_p;
    //
    // LOG(INFO) << "base_p: " << base_p.x() << ", " << base_p.y() << ", " << base_p.z();
    //
    // // Eigen::Affine3f base_T_field = Eigen::AngleAxisf(-field_rz_base, Eigen::Vector3f::UnitZ()) * field_p_base.inverse();
    //
    //
    // return 0;

    ////////////////////////

    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    pthread_sigmask (SIG_UNBLOCK, &signal_mask, nullptr);

    signal(SIGINT, &signalHandler);

    TrackingCamera cam;

    try
    {
        while(runFlag)
        {
            int result = cam.spinOnce();
            if(result)
                return result;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

// int main()
// {
//     el::Configurations defaultConf;
//     defaultConf.setToDefault();
//     defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime{%y-%M-%d %H:%m:%s.%g} [%levshort] %msg");
//     el::Loggers::setDefaultConfigurations(defaultConf, true);
//
//     LOG(INFO) << "Starting TIGERs Tracking Camera";
//
//     SerialPortOptions options;
//     options.baudRate = SerialPortOptions::RATE_115200;
//     options.dataBits = SerialPortOptions::D8;
//     options.parity = SerialPortOptions::NONE;
//     options.stopBits = SerialPortOptions::S1;
//
//     auto pSerialPort = SerialPortFactory::open("/dev/ttyAMA0", options);
//     if(!pSerialPort)
//     {
//         LOG(ERROR) << "Failed to open serial port.";
//         return -1;
//     }
//
//     struct Point
//     {
//         float x;
//         float y;
//     };
//
//     std::vector<Point> path = {
//         { -60, -40 },
//         { 60, 40 },
//         { 60, -40 },
//         { -60, 40 },
//         { 60, 40 },
//         { 60, -40},
//         { -100, 0 },
//         { 100, 0 },
//         { 0, 0}
//     };
//
//     MotionController motionController(pSerialPort);
//
//     size_t nextTargetIndex = 0;
//
//     while(true)
//     {
//         motionController.spinOnce();
//
//         if(motionController.isReady() && !motionController.isMoving())
//         {
//             // motionController.setVelMax(100.0f);
//             // motionController.setAccMax(500.0f);
//             motionController.setTargetPos(path[nextTargetIndex].x, path[nextTargetIndex].y);
//             nextTargetIndex = (nextTargetIndex + 1) % path.size();
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
//
// /*
//     pthread_t self = pthread_self();
//     cpu_set_t cpuset;
//     CPU_ZERO(&cpuset);
//     CPU_SET(3, &cpuset);
//
//     int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
//     if(rc != 0)
//     {
//         LOG(ERROR) << "Failed to pin core: %s" << strerror(errno);
//         return -1;
//     }
//
//     const std::string desiredLine = "GPIO26";
//
//     RPiGPIO pin("GPIO26", RPiGPIO::OUTPUT);
//
//     bool value = true;
//     auto interval = std::chrono::microseconds(100);
//     auto nextWakeTime = std::chrono::high_resolution_clock::now() + interval;
//
//     // for (size_t i = 0; i < 100; i++)
//     while(true)
//     {
//         value = !value;
//         pin.write(value);
//
//         std::this_thread::sleep_until(nextWakeTime);
//         nextWakeTime += interval;
//     }
// */
//     return 0;
// }
