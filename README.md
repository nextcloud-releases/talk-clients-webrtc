# WebRTC client library

This repository contains the tooling to build the WebRTC libraries for the Nextcloud Talk Android and iOS apps.

See actual stable branches:
- https://chromiumdash.appspot.com/branches

Official WebRTC build guides:
- https://webrtc.googlesource.com/src/+/main/docs/native-code/android/index.md
- https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/index.md

## Build for Android 

To build WebRTC for Android follow those steps:

```
docker build -t webrtc .
mkdir webrtc
docker run -it --name webrtc-build -v "$(pwd)"/webrtc webrtc-build
# now in container
fetch --nohooks webrtc_android
gclient sync
cd src
# You'll ask for some interactions:
#  1. Skip snapcraft
#  2. Configure timezone
build/install-build-deps-android.sh
git checkout -b branch_$BRANCH branch-heads/$BRANCH
gclient sync -D
tools_webrtc/android/build_aar.py
```

## Build for iOS

Requirements:

- XCode version 13.0

To build WebRTC for iOS follow those steps:

```
mkdir webrtc_ios
cd webrtc_ios
fetch --nohooks webrtc_ios
gclient sync
cd src
git checkout -b branch_$BRANCH branch-heads/$BRANCH
gclient sync -D
cd tools_webrtc/ios
python build_ios_libs.py
```

If build succeeds, you will find the **WebRTC.xcframework** in `src/out_ios_libs/WebRTC.xcframework`

## Releases

- v96.4664.0 [809830f1b39f9d0933dd979c9e8f32a4a922b71c](https://chromium.googlesource.com/external/webrtc/+/809830f1b39f9d0933dd979c9e8f32a4a922b71c)
