cmake_minimum_required(VERSION 3.26)

project(gimbal_rpi)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_FLAGS_DEBUG "-Og -g")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")

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

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/TrackingCamera.cpp
    src/Settings.cpp
    src/TrackedFrameProvider.cpp
    src/View.cpp
    src/GimbalController.cpp
    src/model/GimbalProtocolMessage.cpp
    src/model/GimbalProtocolTransceiver.cpp
    src/model/Joystick.cpp
    src/util/UDPSyncClient.cpp
    src/util/RPiGPIO.cpp
    src/util/easylogging++.cc
    src/util/SerialPort.cpp
    src/util/SerialPortPosix.cpp
    src/util/SysFsEntry.cpp
    src/util/NetworkInterface.cpp
    src/util/cobs.c
    src/util/crc.c
    src/util/math_util.cpp
    src/util/STBootloader.cpp

    src/protobuf-c/protobuf-c.c
    src/protobuf-c/messages_robocup_ssl_wrapper_tracked.pb-c.c
    src/protobuf-c/messages_robocup_ssl_detection_tracked.pb-c.c
)

target_link_libraries(${PROJECT_NAME} gpiod lvgl)

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin)
install(FILES systemd/gimbal-rpi.service DESTINATION /usr/lib/systemd/system)
