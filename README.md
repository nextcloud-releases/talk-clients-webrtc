# Fork of WebRTC client library

This is a fork of https://webrtc.googlesource.com/src to host our binaries for the iOS and Android clients.

## Update todo

* Checkout the `main` branch
* Pull from https://webrtc.googlesource.com/src `main`
* Push updated main to nextcloud-releases
* Create a tag: `git tag -s -a 'YYYY-MM-DD'
* Push the tag and make the tag a release
* Build the library for:
    * iOS: https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/index.md
    * Android: https://webrtc.googlesource.com/src/+/main/docs/native-code/android/index.md
* Upload the binaries to the release

