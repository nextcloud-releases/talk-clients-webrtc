# WebRTC client library

This repository contains the tooling to build the WebRTC libraries for the Nextcloud Talk Android and iOS apps.

Pick the latest stable branch from that table (click on the brunch number in the column `WebRTC`):
- https://chromiumdash.appspot.com/branches

In the following build instructions replace `$BRANCH` with the picked branch number.

Official WebRTC build guides:
- https://webrtc.googlesource.com/src/+/main/docs/native-code/android/README.md
- https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/README.md

### Build Android
For android it is possible to build on github CI (recommended) or locally.
Both of them create the `aar` file.
To use this inside the nextcloud android talk app, follow the steps from https://github.com/nextcloud-deps/android-talk-webrtc#packaging-instructions

### Build Android lib on github CI (recommended)

- In https://github.com/nextcloud-releases/talk-clients-webrtc go to Actions
- got to Build AndroidWebRTC
- click "Run workflow"
- enter branch number (e.g. "1234") & confirm with "Run workflow"
- When finished, the resulting zipped aar file should be attached to the build.


### Build Android lib locally (in case CI won't work)

To build WebRTC for Android follow those steps:

```
./build-android.sh $BRANCH
```

First run needs around an hour. Manual interactions are needed during the run. It may happen that the script gets stuck. If this happens without any error message, just start it again.

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
git apply {path-to-this-repo}/ios/patches/*.patch
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

### Creating a debug build

```
python build_ios_libs.py --build_config debug
```
