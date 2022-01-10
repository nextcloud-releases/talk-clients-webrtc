# Fork of WebRTC client library

This is a fork of https://webrtc.googlesource.com/src to host our binaries for the iOS and Android clients.

## Update todo

* Checkout the `git checkout main` branch
* Pull from https://webrtc.googlesource.com/src: `git pull upstream main`
* Push updated main to nextcloud-releases `git push nextcloud main`
* Create a tag: `git tag -s -a 'YYYY-MM-DD' -m 'Tag YYYY-MM-DD'`
* Push the tag: `git push nextcloud YYYY-MM-DD`
* Make the tag a release: https://github.com/nextcloud-releases/talk-clients-webrtc/tags
* Build the library for:
    * iOS: https://webrtc.googlesource.com/src/+/main/docs/native-code/ios/index.md
    * Android: https://webrtc.googlesource.com/src/+/main/docs/native-code/android/index.md
* Upload the binaries to the release: https://github.com/nextcloud-releases/talk-clients-webrtc/releases
