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
