#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

cd workdir || exit
if [[ ! -d "src" ]]; then
  fetch --nohooks webrtc_android
fi
gclient sync
cd src || exit
if [[ -z $(git branch --list "$WEBRTC_BRANCH") ]]; then
  git checkout -b "$WEBRTC_BRANCH" branch-heads/"$WEBRTC_BRANCH"
else
  git pull
fi
echo "***************************************************************"
echo "During the build, you'll be asked for some interactions (Configure timezone, ...)"
echo "***************************************************************"

gclient sync -D
sudo build/install-build-deps.sh

echo "Copying files..."
cp -R /webrtc/repo/files/. .
cp -R /webrtc/repo/android/files/. .

echo "Applying patches..."
find /webrtc/repo/patches/ /webrtc/repo/android/patches/ -name "*.patch" -print0 | xargs -0 -n 1 patch -p1 -i

tools_webrtc/android/build_aar.py
