# TIGERs Mannheim 2-Axis Gimbal System

This application is designed to control a pan-tilt gimbal connected to a Raspberry Pi via a custom driver board.
For details on the hardware please consult the gimbal-hardware repository.

# Development

The project is based on CMake and hence should be able to build natively on a Raspberry Pi with:
   ```
   cmake -B build .
   cmake --build build -j
   ```

Note that this has not been tested. Only the cross-compilation approach below was tested.

## How to cross-compile

You need:
- cmake
- The sysroot of your target. Available as artifact of the tigers-buildroot project. It contains all the system libraries and files.
    - If you have a local tigers-buildroot setup the required subfolder is `tigers-buildroot/tigers/output/<boardname>/host/arm-buildroot-linux-gnueabihf/sysroot`
    - Alternatively, you may download the sysroot from the tigers-buildroot CI [here](https://gitlab.tigers-mannheim.de/main/tigers-buildroot/-/pipelines). Download the gimbal_pizero-sysroot artifact.
- For a Raspberry Pi Zero 2W one of the following toolchains (ARMv7 compatible):
    - For Linux: [here](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz)
    - For Windows: [here](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-linux-gnueabihf.zip)
- For a Raspberry Pi Zero (incl. W) one of the following toolchains (ARMv6 compatible):
    - For Linux: [armv6-eabihf bleeding-edge](https://toolchains.bootlin.com/downloads/releases/toolchains/armv6-eabihf/tarballs/armv6-eabihf--glibc--bleeding-edge-2024.05-1.tar.xz)
    - For Windows: [raspberry-gcc14.2.0](https://sysprogs.com/getfile/2543/raspberry-gcc14.2.0.exe)
    - If you get compilation errors around `__GTHREAD_COND_INIT`:
        - Go to your compiler extraction folder and remove or rename `lib\gcc\arm-linux-gnueabihf\14\include-fixed`
        - The error occurs due to an incorrect "fixup" of the `pthread.h` file.

:point_up: The mentioned compilers are compatible with the one used by tigers-buildroot. If tigers-buildroot is updated, different ones may need to be selected.

A toolchain file needs to be selected based on the target platform and build operating system.

| Target                | Host OS       | Toolchain File                                  |
|-----------------------|---------------|-------------------------------------------------|
| Raspberry Pi Zero (W) | Linux         | `arm-buildroot-linux-gnueabihf.toolchain.cmake` |
| Raspberry Pi Zero (W) | Windows       | `arm-linux-gnueabihf.toolchain.cmake`           |
| Raspberry Pi Zero 2W  | Linux/Windows | `arm-none-linux-gnueabihf.toolchain.cmake`      |

Then go into the repository root folder and run:

1. Linux build
   ```
   cmake -B build -DCMAKE_SYSROOT=/your/path/to/sysroot -DCMAKE_PREFIX_PATH=/your/path/to/toolchain -DCMAKE_TOOLCHAIN_FILE=cmake/<toolchain_file> .
   cmake --build build -j
   ```
1. Windows build (assuming MinGW environment)
   ```
   cmake -B build -G "MinGW Makefiles" -DCMAKE_SYSROOT=/your/path/to/sysroot -DCMAKE_PREFIX_PATH=/your/path/to/toolchain -DCMAKE_TOOLCHAIN_FILE=cmake/<toolchain_file> .
   cmake --build build -j
   ```

You need to adjust your sysroot and toolchain path and select a proper toolchain file from the list above.

## Buildroot integration

The repository contains `Config.in` and `gimbal-rpi.mk` for easy buildroot integration.

If you have the tigers-buildroot repository just clone `gimbal-rpi` into `tigers-buildroot/tigers/package`.
Go into the `tigers-buildroot` folder and make the new package know to buildroot: `cat tigers/package/gimbal-rpi/Config.in >> tigers/Config.in`.
Then add it to a default board configuration: `echo BR2_PACKAGE_GIMBAL_RPI=y >> tigers/configs/gimbal_pizero_defconfig`.
Afterward, reload your board configuration: `cd tigers && make gimbal_pizero_defconfig` 
and make a clean build `cd ../output/gimbal_pizero && make clean && make -j8`.
The `gimbal-rpi.mk` is automatically picked up by buildroot.

See the tigers-buildroot repository for more details on how to use it.
The `.gitlab-ci.yml` contains an example of how to integrate the whole process in a CI/CD.

## CLion IDE Setup
1. Install CLion and clone the project
1. Go to Settings => Appearance & Behavior => Path Variables and add:
    1. Name: TIGERS_RPI_TOOLCHAIN, Value: root path of your cross-compiler toolchain
    1. Name: TIGERS_GIMBAL_PIZERO_SYSROOT, Value: root path of your target's sysroot
1. The default cmake profiles use `arm-linux-gnueabihf.toolchain.cmake`. You can change them at Settings => Build, Execution, Deployment => CMake => CMake Options
1. Go to Settings => Tools => SSH Configurations and add a configuration:
    1. Host: tigersgimbal.local, User: root, PW: root
1. You can now compile the application locally and execute it remotely
1. If the above does not work after setting the paths restart CLion and delete the build folder, then rebuild.
