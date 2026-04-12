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
#set(CMAKE_FIND_USE_CMAKE_PATH OFF)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)

#set(Protobuf_USE_STATIC_LIBS ON CACHE STRING "" FORCE)

#find_package(Protobuf REQUIRED)

#message("protoc: ${Protobuf_PROTOC_EXECUTABLE}")

add_definitions(
    -DELPP_STL_LOGGING
    -DELPP_THREAD_SAFE
)

add_compile_options(-Wno-psabi)

include_directories(
    src
    src/util

#    ${Protobuf_INCLUDE_DIRS}
)

add_executable(ssl_tracking_cam
    src/main.cpp
    src/TrackingCamera.cpp
    src/Settings.cpp
    src/StepperAxis.cpp
    src/MotionController.cpp
    src/TrackedFrameProvider.cpp
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

#set_source_files_properties(
#    src/proto/messages_robocup_ssl_wrapper_tracked.pb.cc
#    src/proto/messages_robocup_ssl_detection_tracked.pb.cc
#    PROPERTIES
#    COMPILE_OPTIONS "-O3;-DNDEBUG;-g0"
#)

target_link_libraries(ssl_tracking_cam gpiod protobuf absl_log_internal_check_op absl_log_internal_message absl_log_internal_nullguard)
