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

if [[ $# -eq 0 ]] ; then
    echo 'no branch set as parameter'
    exit 0
fi

BRANCH=$1

docker build --tag webrtc-build android 

mkdir -p workdir
# docker unshare chown 1000:1000 -R workdir/

chmod 777 workdir

#if docker container exists webrtc-build; then
#  docker container rm webrtc-build
#fi
docker container run --interactive \
  --tty \
  --env WEBRTC_BRANCH="$BRANCH" \
  --name webrtc-build \
  --volume "$(pwd)"/workdir:/webrtc/workdir:rw \
  webrtc-build:latest

mkdir -p result/android
cp workdir/src/*.aar result/android
