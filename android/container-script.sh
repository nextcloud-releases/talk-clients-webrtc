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
echo "You'll be asked for some interactions:"
echo "  1. Skip snapcraft"
echo "  2. Configure timezone"
read -r -p "Press key to continue.. " -n1 -s
gclient sync -D
sudo build/install-build-deps.sh
tools_webrtc/android/build_aar.py
