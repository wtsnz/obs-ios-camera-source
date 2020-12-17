<div align = "center">
<img src=".github/obs-logo.svg" width="128" height="128" />
</div>


obs-ios-camera-source
==============
Use your iPhone camera as a video source in OBS Studio and stream high quality video from your iPhone's camera over USB.

[![Build Status](https://dev.azure.com/wtsnz/Camera%20for%20OBS%20Plugin/_apis/build/status/wtsnz.obs-ios-camera-source?branchName=refs%2Ftags%2Fv2.7.0)](https://dev.azure.com/wtsnz/Camera%20for%20OBS%20Plugin/_build/latest?definitionId=1&branchName=refs%2Ftags%2Fv2.7.0)

To use this you use the [accompanying iOS app](https://will.townsend.io/products/obs-iphone/) to begin streaming in OBS.


## Downloads

Binaries for Windows and Mac are available in the [Releases](https://github.com/wtsnz/obs-ios-camera-source/releases) section.

## Building

You can run the CI scripts to build it. They will clone and build OBS Studio prior to building this plugin.

    ./CI/install-dependencies-macos.sh
    ./CI/install-build-obs-macos.sh
    ./CI/build-macos.sh
    ./CI/package-macos.sh


## Special thanks
- The entire [obs-websockets](https://github.com/Palakis/obs-websocket) project for providing a stella example of an obs plugin build pipeline!
