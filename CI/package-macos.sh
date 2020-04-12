#!/bin/sh

set -e

OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
    echo "[obs-ios-camera-plugin - Error] macOS package script can be run on Darwin-type OS only."
    exit 1
fi

echo "[obs-ios-camera-plugin] Preparing package build"

GIT_HASH=$(git rev-parse --short HEAD)
GIT_BRANCH_OR_TAG=$(git name-rev --name-only HEAD | awk -F/ '{print $NF}')

VERSION="$GIT_HASH-$GIT_BRANCH_OR_TAG"
LATEST_VERSION="$GIT_BRANCH_OR_TAG"

FILENAME_UNSIGNED="obs-ios-camera-source-$VERSION-Unsigned.pkg"
FILENAME="obs-ios-camera-source-$VERSION.pkg"

echo "-- Modifying obs-ios-camera-source.so"
install_name_tool \
	-change /usr/local/opt/ffmpeg/lib/libavcodec.58.dylib @rpath/libavcodec.58.dylib \
	-change /usr/local/opt/ffmpeg/lib/libavutil.56.dylib @rpath/libavutil.56.dylib \
	-change /tmp/obsdeps/bin/libavcodec.58.dylib @rpath/libavcodec.58.dylib \
	-change /tmp/obsdeps/bin/libavutil.56.dylib @rpath/libavutil.56.dylib \
	./build/obs-ios-camera-source.so

echo "-- Dependencies for obs-ios-camera-source"
otool -L ./build/obs-ios-camera-source.so

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "-- Signing plugin binary: obs-ios-camera-source.so"
	codesign --sign "$CODE_SIGNING_IDENTITY" ./build/obs-ios-camera-source.so
else
	echo "-- Skipped plugin codesigning"
fi

echo "-- Actual package build"
packagesbuild ./CI/macos/obs-ios-camera-source.pkgproj

echo "-- Renaming obs-ios-camera-source.pkg to $FILENAME_UNSIGNED"
# mkdir release
mv ./release/obs-ios-camera-source.pkg ./release/$FILENAME_UNSIGNED

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "[obs-ios-camera-source] Signing installer: $FILENAME"
	productsign \
		--sign "$INSTALLER_SIGNING_IDENTITY" \
		./release/$FILENAME_UNSIGNED \
		./release/$FILENAME

	echo "[obs-ios-camera-source] Submitting installer $FILENAME for notarization"
	zip -r ./release/$FILENAME.zip ./release/$FILENAME
	xcrun altool \
		--notarize-app \
		--primary-bundle-id "io.loftlabs.obs-ios-camera-source.pkg"
		--username $AC_USERNAME
		--password $AC_PASSWORD
		--asc-provider $AC_PROVIDER_SHORTNAME
		--file ./release/$FILENAME.zip

	rm ./release/$FILENAME_UNSIGNED ./release/$FILENAME.zip
else
	echo "[obs-ios-camera-source] Skipped installer codesigning and notarization"
fi