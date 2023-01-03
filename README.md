# WebRTC client library

This repository contains the tooling to build the WebRTC libraries for the Nextcloud Talk Android and iOS apps.

Pick the latest stable branch from that table (click on the brunch number in the column `WebRTC`):
- https://chromiumdash.appspot.com/branches

In the following build instructions replace `$BRANCH` with the picked branch number.

Official WebRTC build guides:
- https://webrtc.googlesource.com/src/+/main/docs/native-code/android/index.md
- https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/index.md

## Build for Android 

Requirements:
- [Podman](https://docs.podman.io/en/latest/)

To build WebRTC for Android follow those steps:

```
./build.sh $BRANCH
```

First run needs around an hour. Manual interactions are needed during the run.

The created `aar` can be found in `result/android` afterwards.

## Build for iOS

Requirements:

- Xcode version 13.0+
- [Depot tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) (latest master)

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

### Building on M1 Macs

Check if the command `python` is available. Most likley you need to link it (adjust the Xcode path accordingly):

```
sudo ln -s /Applications/Xcode.app/Contents/Developer/usr/bin/python3 /usr/local/bin/python
```

### Building multiple architectures

By default the following architectures are build:
```
'device:arm64', 'simulator:arm64', 'simulator:x64'
```

It is also possible to build for catalyst, the build command would look like this:

```
python build_ios_libs.py --arch "device:arm64" "simulator:arm64" "simulator:x64" "catalyst:arm64" "catalyst:x64"
```
