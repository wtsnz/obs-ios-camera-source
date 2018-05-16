#!/bin/sh
set -ex

# OBS Studio deps
brew update
brew install ffmpeg
brew install libav

# We need to make sure that the version of FFMpeg is the same as
# what was used to build OBS Studio. Right now it was 3.4.2, so I created
# a homebrew tap that points to that version.
#brew install ffmpeg # installs version 4.
brew tap wtsnz/brew-ffmpeg-tap https://github.com/wtsnz/brew-ffmpeg-tap.git
brew install wtsnz/brew-ffmpeg-tap/ffmpeg

# qtwebsockets deps
# qt latest
#brew install qt5

# qt 5.9.2
brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/2b121c9a96e58a5da14228630cb71d5bead7137e/Formula/qt.rb

#echo "Qt path: $(find /usr/local/Cellar/qt5 -d 1 | tail -n 1)"

# Build obs-studio
cd ..
git clone --recursive https://github.com/jp9000/obs-studio
cd obs-studio
git checkout 21.0.0
mkdir build && cd build
cmake .. \
  -DCMAKE_PREFIX_PATH=/usr/local/opt/qt/lib/cmake \
&& make -j4

# Packages app
cd ..
curl -L -O  http://s.sudre.free.fr/Software/files/Packages.dmg -f --retry 5 -C -
hdiutil attach ./Packages.dmg
sudo installer -pkg /Volumes/Packages\ 1.2.3/packages/Packages.pkg -target /
