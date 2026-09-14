#!/bin/sh
set -eu
cd "$(dirname "$0")"
target="${1:-c3}"
case "$target" in c3|c6) ;; *) echo 'Usage: ./build.sh c3|c6' >&2; exit 2;; esac
fqbn="esp32:esp32:esp32${target}:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M"
arduino-cli compile --fqbn "$fqbn" --build-path "build/cache/$target" --output-dir "build/$target" firmware/lrf_hf_bridge
