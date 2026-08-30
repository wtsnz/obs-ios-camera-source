<div align = "center">
<img src=".github/obs-logo.svg" width="128" height="128" />
</div>


obs-ios-camera-source
==============
Use your iPhone camera as a video source in OBS Studio and stream high quality video from your iPhone's camera over USB.

This plugin pairs with the [accompanying iOS app](https://obs.camera/) to begin streaming in OBS.

## Downloads

Binaries for Windows and Mac are available in the [Releases](https://github.com/wtsnz/obs-ios-camera-source/releases) section.

## Linux build

Linux builds use the system copies of OBS, FFmpeg, libimobiledevice, and
libusbmuxd. Install their development packages, along with CMake, a C++17
compiler, and pkg-config. Then run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

Install the plugin for the current user without root access:

```sh
cmake --build build --target install-user
```

Restart OBS, add an **iOS Camera** source, connect and unlock the iPhone, and
accept the trust prompt if one appears. The accompanying iOS app must be
running. Device discovery can be checked independently with `idevice_id -l`.

For distribution packaging or a system-wide install, use the standard CMake
install target instead. Its destinations respect `CMAKE_INSTALL_PREFIX` and
the GNU installation directory variables.


## Special thanks

- The entire [obs-websockets](https://github.com/Palakis/obs-websocket) project for providing a stella example of an obs plugin build pipeline!
