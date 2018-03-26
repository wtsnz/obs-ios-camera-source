#!/bin/sh
# export the tarball

# update the .spec file with the version number

# upload both files
echo "[general]" > ~/.oscrc
echo "apiurl = https://api.opensuse.org" >> ~/.oscrc
echo "[https://api.opensuse.org]" >> ~/.oscrc
echo "user = qmfrederik" >> ~/.oscrc
echo "pass = $GPG_PASSPHRASE" >> ~/.oscrc

mkdir osc
cd osc
osc checkout home:qmfrederik
cd home\:qmfrederik/libusbmuxd
osc rm *
cd ../../../
git archive --format tar.gz -o osc/home\:qmfrederik/libusbmuxd/libusbmuxd-${LIBUSBMUXD_VERSION_PREFIX}.$TRAVIS_BUILD_NUMBER.tar.gz --prefix libusbmuxd-${LIBUSBMUXD_VERSION_PREFIX}.$TRAVIS_BUILD_NUMBER/ HEAD
cd osc/home\:qmfrederik/libusbmuxd
cp ../../../libusbmuxd.spec .
sed -i "s/${LIBUSBMUXD_VERSION_PREFIX}.build/${LIBUSBMUXD_VERSION_PREFIX}.${TRAVIS_BUILD_NUMBER}/" libusbmuxd.spec
osc add *
osc commit -m "Update for Travis CI build $TRAVIS_BUILD_NUMBER"

