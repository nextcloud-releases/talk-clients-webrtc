# WebRTC client library

This repository contains the tooling to build the WebRTC libraries for the Nextcloud Talk Android and iOS apps.

Pick the latest stable branch from that table (click on the brunch number in the column `WebRTC`):
- https://chromiumdash.appspot.com/branches

In the following build instructions replace `$BRANCH` with the picked branch number.

Official WebRTC build guides:
- https://webrtc.googlesource.com/src/+/main/docs/native-code/android/README.md
- https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/README.md

## Build Android
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
cp -R {path-to-this-repo}/ios/files/. .
find {path-to-this-repo}/ios/patches/ -name "*.patch" -print0 | xargs -0 -n 1 patch -p1 -i
cd tools_webrtc/ios
python build_ios_libs.py
```

If build succeeds, you will find the **WebRTC.xcframework** in `src/out_ios_libs/WebRTC.xcframework`

`patch` is used instead of `git apply`, which rejects patches that only apply with fuzz on newer branches.

### Talk changes to WebRTC

- `ios/files`: files added to WebRTC, in the layout of `src`, e.g. the end-to-end call encryption.
- `ios/patches`: changes to existing WebRTC files, e.g. `talk-frame-crypto.patch` adds the files above to the build.

After changing them in a WebRTC checkout, copy them back and regenerate the patch from `src`:

```
cp -R modules/talk_frame_crypto {path-to-this-repo}/ios/files/modules/
cp sdk/objc/api/peerconnection/RTCTalkKeyRing.* {path-to-this-repo}/ios/files/sdk/objc/api/peerconnection/
cp sdk/objc/unittests/RTCTalkKeyRingTest.mm {path-to-this-repo}/ios/files/sdk/objc/unittests/
git diff -- modules/BUILD.gn sdk/BUILD.gn sdk/objc/DEPS > {path-to-this-repo}/ios/patches/talk-frame-crypto.patch
```

The files in `modules/talk_frame_crypto` implement the frame format that Nextcloud Talk uses for end-to-end encryption and are licensed under Apache-2.0.

### Running the Talk frame crypto tests

They run on the Mac, from `src`:

```
gn gen out/talk_tests --args='is_debug=false dcheck_always_on=true rtc_include_tests=true'
autoninja -C out/talk_tests modules/talk_frame_crypto:talk_frame_crypto_unittests
out/talk_tests/talk_frame_crypto_unittests
```

The Objective-C tests are part of `sdk_unittests` and run in the simulator:

```
gn gen out/sim --args='target_os="ios" target_environment="simulator" target_cpu="arm64" ios_enable_code_signing=false is_debug=true rtc_include_tests=true enable_run_ios_unittests_with_xctest=true'
autoninja -C out/sim sdk:sdk_unittests testing/iossim
$(find out/sim -name iossim -type f -perm -u+x | head -1) -d 'iPhone 17' -s 26.5 -t RTCTalkKeyRingTest out/sim/sdk_unittests.app "$(find out/sim/sdk_unittests.app -name '*.xctest' -maxdepth 2 | head -1)"
```

Newer Xcode versions may not be supported by the WebRTC branch, use the one from `.github/workflows/build_ios.yml`, e.g. with `export DEVELOPER_DIR=/Applications/Xcode_X.app/Contents/Developer` before `gn gen`.

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
