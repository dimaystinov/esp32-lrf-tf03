#!/bin/sh
set -eu
cd "$(dirname "$0")"
target="${1:-c3}"
port="${2:-/dev/cu.usbmodem101}"
case "$target" in c3|c6) ;; *) echo 'Usage: ./flash.sh c3|c6 [port]' >&2; exit 2;; esac
./build.sh "$target"
fqbn="esp32:esp32:esp32${target}:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M"
arduino-cli upload --fqbn "$fqbn" --port "$port" --input-dir "build/$target" firmware/lrf_hf_bridge
