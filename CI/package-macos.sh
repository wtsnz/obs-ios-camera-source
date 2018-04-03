#!/bin/sh

set -e

echo "-- Preparing package build"
export QT_CELLAR_PREFIX="$(find /usr/local/Cellar/qt -d 1 | tail -n 1)"

export GIT_HASH=$(git rev-parse --short HEAD)

export VERSION="$GIT_HASH-$TRAVIS_BRANCH"
export LATEST_VERSION="$TRAVIS_BRANCH"
if [ -n "${TRAVIS_TAG}" ]; then
	export VERSION="$TRAVIS_TAG"
	export LATEST_VERSION="$TRAVIS_TAG"
fi

export FILENAME="obs-ios-camera-source-$VERSION.pkg"
export LATEST_FILENAME="obs-ios-camera-source-latest-$LATEST_VERSION.pkg"

echo "-- Modifying obs-ios-camera-source.so"
install_name_tool \
	-change /usr/local/opt/ffmpeg/lib/libavcodec.57.dylib @rpath/libavcodec.57.dylib \
	-change /usr/local/opt/ffmpeg/lib/libavutil.55.dylib @rpath/libavutil.55.dylib \
	./build/obs-ios-camera-source.so

echo "-- Dependencies for obs-ios-camera-source"
otool -L ./build/obs-ios-camera-source.so

echo "-- Actual package build"
packagesbuild ./CI/macos/obs-ios-camera-source.pkgproj

echo "-- Renaming obs-ios-camera-source.pkg to $FILENAME"
mv ./release/obs-ios-camera-source.pkg ./release/$FILENAME
cp ./release/$FILENAME ./release/$LATEST_FILENAME
