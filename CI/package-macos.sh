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
for lib in $(otool -L ./build/obs-ios-camera-source.so | awk '$1 ~ /^\// && $1 ~ /(libav.*)$/ { print $1 }'); do
	install_name_tool -change "$lib" "@rpath/$(basename "$lib")" ./build/obs-ios-camera-source.so
done

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
	UPLOAD_RESULT=$(xcrun altool \
		--notarize-app \
		--primary-bundle-id "io.loftlabs.obs-ios-camera-source.pkg" \
		--username "$AC_USERNAME" \
		--password "$AC_PASSWORD" \
		--asc-provider "$AC_PROVIDER_SHORTNAME" \
		--file "./release/$FILENAME.zip")
	rm ./release/$FILENAME.zip

	REQUEST_UUID=$(echo $UPLOAD_RESULT | awk -F ' = ' '/RequestUUID/ {print $2}')
	echo "Request UUID: $REQUEST_UUID"

	echo "[obs-ios-camera-source] Wait for notarization result"
	# Pieces of code borrowed from rednoah/notarized-app
	while sleep 30 && date; do
		CHECK_RESULT=$(xcrun altool \
			--notarization-info "$REQUEST_UUID" \
			--username "$AC_USERNAME" \
			--password "$AC_PASSWORD" \
			--asc-provider "$AC_PROVIDER_SHORTNAME")
		echo $CHECK_RESULT

		if ! grep -q "Status: in progress" <<< "$CHECK_RESULT"; then
			echo "[obs-ios-camera-source] Staple ticket to installer: $FILENAME"
			xcrun stapler staple ./release/$FILENAME
			break
		fi
	done
else
	echo "[obs-ios-camera-source] Skipped installer codesigning and notarization"
fi