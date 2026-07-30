LA9310 iqplayer based on https://github.com/nxp-qoriq/la931x_iqplayer

# IQPlayer firmware for LimeSDR-Micro VSPA

Building from source:

```
git clone github.com/myriadrf/LimeSDR-Micro_VSPA
cd LimeSDR-Micro_VSPA
cmake -B build && cd build
make
```

Produced firmware images will be placed in LimeSDR-Micro_VSPA/build/Release directory.

If "VSPA_TOOL" environment variable is available, VSPA toolchain will be used from that location.
If "VSPA_TOOL" environment variable is not set, CMake will download VSPA toolchain and place it in project's artifacts directory.

# Requirements

VSPA toolchain compiler is 32-bit only, so on 64-bit Debian/Ubuntu host machines 32-bit runtime libraries need to be installed.
If `i386` is not already enabled in `dpkg --print-foreign-architectures`, add it first:
```
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6:i386 libstdc++6:i386
```
