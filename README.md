# 2-Axis SSL Tracking Camera System
This application is designed to control a pan-tilt gimbal connected to a Raspberry Pi via TMC2209 stepper motor drivers.

## Hardware

This has been tested with the following hardware:
- Raspberry Pi Zero 2W
- [PoE Ethernet Hat for Raspberry Pi Zero](https://www.waveshare.com/poe-eth-usb-hub-hat.htm)
- [1.3" LCD Hat with buttons](https://www.waveshare.com/1.3inch-lcd-hat.htm)
- Custom board with two [TMC2209](https://www.analog.com/en/products/tmc2209.html) drivers powered with 5V
- [NEMA 17 1A Stepper Motor](https://www.omc-stepperonline.com/de/nema-17-bipolar-1-8deg-16ncm-22-6oz-in-1a-3-5v-42x42x20mm-4-draehte-17hs08-1004s) for the tilt axis
- [NEMA 17 1.2A Stepper Motor](https://www.omc-stepperonline.com/de/e-serie-nema-17-bipolar-26ncm-36-82oz-in-1-2a-42x42x30mm-4draehte-w-1m-kabel-verbinder-17he12-1204s) for the pan (yaw) axis

# Development

## How to build

Build is based on cmake. The applications are cross-compiled on a standard x64 computer.

You need:
- The sysroot of your target. Available as artifact of the tigers-buildroot project. It contains all the system libraries and files.
    - If you have a local tigers-buildroot setup the required subfolder is `tigers-buildroot/buildroot/output/host/arm-buildroot-linux-gnueabihf/sysroot`
    - Alternatively, you may download the sysroot from the tigers-buildroot CI [here](https://gitlab.tigers-mannheim.de/main/tigers-buildroot/-/pipelines). Download the pizero2w-tigers-sysroot artifact.
- A toolchain:
    - For Linux: [here](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz)
    - For Windows: [here](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-linux-gnueabihf.zip)
- cmake

:point_up: The required compiler is GCC 15.2 for 32-Bit ARM with hardware floating point support. This version is compatible with the version tigers-buildroot is using. If tigers-buildroot is updated, this one must be changed as well.

Then go into the repository root folder and run:

1. Linux build
   ```
   cmake -B build -DCMAKE_SYSROOT=/your/path/to/sysroot -DCMAKE_PREFIX_PATH=/your/path/to/toolchain .
   cmake --build build -j
   ```
1. Windows build (assuming MinGW environment)
   ```
   cmake -B build -G "MinGW Makefiles" -DCMAKE_SYSROOT=/your/path/to/sysroot -DCMAKE_PREFIX_PATH=/your/path/to/toolchain .
   cmake --build build -j
   ```

You may need to adjust your sysroot and toolchain path.

## CLion IDE Setup
1. Install CLion and clone the project
1. Go to Settings => Appearance & Behavior => Path Variables and add:
    1. Name: TIGERS_RPI_TOOLCHAIN, Value: root path of your cross-compiler toolchain
    1. Name: TIGERS_PIZERO2W_SYSROOT, Value: root path of your target's sysroot
1. Go to Settings => Tools => SSH Configurations and add a configuration:
    1. Host: tigerspizero2w.local, User: root, PW: root
1. You can now compile the application locally and execute it remotely
1. If the above does not work after setting the paths restart CLion and delete the build folder, then rebuild.
