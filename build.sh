#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'  

#/ Usage: build.sh <BRANCH>
#/ Builds 'libwebrtc.aar' for the passed WebRTC branch.
usage() {
    grep '^#/' "$0" | cut -c4-
    exit 0
}
expr "$*" : ".*--help" > /dev/null && usage

BRANCH=${1:-"master"}

podman image exists webrtc-build || podman build --tag webrtc-build android 

mkdir -p workdir
podman unshare chown 1000:1000 -R workdir/

if podman container exists webrtc-build; then
  podman container rm webrtc-build
fi
podman container run --interactive \
  --tty \
  --env WEBRTC_BRANCH="$BRANCH" \
  --name webrtc-build \
  --volume "$(pwd)"/workdir:/webrtc/workdir:rw \
  localhost/webrtc-build

mkdir -p result/android
cp workdir/src/*.aar result/android
