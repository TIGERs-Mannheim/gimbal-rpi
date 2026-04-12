cmake_minimum_required(VERSION 3.26)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-linux-gnueabihf.toolchain.cmake)

project(ssl_tracking_cam)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)

add_definitions(
    -DELPP_STL_LOGGING
    -DELPP_THREAD_SAFE
)

add_compile_options(-Wno-psabi)

include(FetchContent)

set(LV_BUILD_CONF_PATH "src/lv_conf.h" CACHE PATH "Custom lvgl config" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG 85aa60d18b3d5e5588d7b247abf90198f07c8a63 # Release 9.5.0
)

FetchContent_MakeAvailable(lvgl)

include_directories(
    src
    src/util
)

add_executable(ssl_tracking_cam
    src/main.cpp
    src/TrackingCamera.cpp
    src/Settings.cpp
    src/StepperAxis.cpp
    src/MotionController.cpp
    src/TrackedFrameProvider.cpp
    src/Presenter.cpp
    src/drv/TMC2209.cpp
    src/util/UDPSyncClient.cpp
    src/util/RPiGPIO.cpp
    src/util/easylogging++.cc
    src/util/SerialPort.cpp
    src/util/SerialPortPosix.cpp
    src/util/traj_2order.c

    src/proto/messages_robocup_ssl_wrapper_tracked.pb.cc
    src/proto/messages_robocup_ssl_detection_tracked.pb.cc
)

target_link_libraries(ssl_tracking_cam gpiod protobuf absl_log_internal_check_op absl_log_internal_message absl_log_internal_nullguard lvgl)

install(TARGETS ssl_tracking_cam DESTINATION /usr/local/bin)
install(FILES systemd/ssl-tracking-cam.service DESTINATION /usr/lib/systemd/system)
