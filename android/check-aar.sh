#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

#/ Usage: check-aar.sh <AAR>
#/
#/ Fails when classes.jar references org.webrtc or org.jni_zero classes it does not contain,
#/ like the *Jni classes jni_zero generates, which would crash the app at runtime.

if [[ $# -ne 1 ]]; then
  grep '^#/' "$0" | cut -c4-
  exit 1
fi

aar=$(realpath "$1")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

unzip -q "$aar" classes.jar -d "$tmp"
unzip -q "$tmp/classes.jar" -d "$tmp/classes"
cd "$tmp/classes"

find org -name '*.class' | sed 's/\.class$//' | sort > "$tmp/present"
find org -name '*.class' -print0 \
  | xargs -0 grep -ahoE 'org/(webrtc|jni_zero)/[A-Za-z0-9_/$]+' \
  | sort -u > "$tmp/referenced"

missing=$(comm -13 "$tmp/present" "$tmp/referenced")
if [[ -n $missing ]]; then
  echo "classes.jar references classes it does not contain:"
  echo "$missing"
  exit 1
fi

echo "All referenced org.webrtc and org.jni_zero classes are in classes.jar"
