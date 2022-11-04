#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

cd workdir || exit
fetch --nohooks webrtc_android
gclient sync
cd src || exit
git checkout -b "$WEBRTC_BRANCH" branch-heads/"$WEBRTC_BRANCH"
echo "You'll ask for some interactions:"
echo "  1. Skip snapcraft"
echo "  2. Configure timezone"
read -r -p "Press key to continue.. " -n1 -s
gclient sync -D
sudo build/install-build-deps-android.sh
tools_webrtc/android/build_aar.py
