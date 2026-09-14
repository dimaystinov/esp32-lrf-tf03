#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
finish() {
  result=$?
  if [ "$result" -ne 0 ]; then echo 'Flash failed. Read the error above; check USB cable, port and BOOT mode.'; fi
  if [ -t 0 ]; then read -r -p 'Press Enter to close...' unused || true; fi
  exit "$result"
}
trap finish EXIT
version=1.5.1
cli="$PWD/.tools/arduino-cli"
if [ ! -x "$cli" ]; then
  if command -v arduino-cli >/dev/null 2>&1 && arduino-cli version | grep -q 'Version: 1.5.1'; then
    cli="$(command -v arduino-cli)"
  else
    case "$(uname -m)" in
      arm64) arch=ARM64 ;;
      x86_64) arch=64bit ;;
      *) echo 'Unsupported Mac architecture'; exit 1 ;;
    esac
    mkdir -p .tools
    asset="arduino-cli_${version}_macOS_${arch}.tar.gz"
    base="https://github.com/arduino/arduino-cli/releases/download/v$version"
    echo 'Downloading Arduino CLI...'
    curl --fail --location --retry 2 "$base/$asset" -o ".tools/$asset"
    curl --fail --location --retry 2 "$base/$version-checksums.txt" -o .tools/checksums.txt
    expected=$(awk -v name="$asset" '$2==name {print $1}' .tools/checksums.txt)
    actual=$(shasum -a 256 ".tools/$asset" | awk '{print $1}')
    [ -n "$expected" ] && [ "$expected" = "$actual" ] || { echo 'Download checksum mismatch'; exit 1; }
    tar -xzf ".tools/$asset" -C .tools arduino-cli
    chmod +x "$cli"
    rm ".tools/$asset"
  fi
fi
cores=$("$cli" core list)
if ! echo "$cores" | grep -Eq '^esp32:esp32[[:space:]]+3\.3\.11([[:space:]]|$)'; then
  echo 'Installing ESP32 core 3.3.11 (first run may take several minutes)...'
  index=https://espressif.github.io/arduino-esp32/package_esp32_index.json
  "$cli" core update-index --additional-urls "$index"
  "$cli" core install esp32:esp32@3.3.11 --additional-urls "$index"
fi
if [ ! -f firmware/lrf_hf_bridge/Secrets.h ]; then
  cp firmware/lrf_hf_bridge/Secrets.example.h firmware/lrf_hf_bridge/Secrets.h
fi
shopt -s nullglob
ports=(/dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial*)
if [ "${#ports[@]}" -eq 0 ]; then echo 'No USB serial port. Connect ESP using a data USB cable; try BOOT mode.'; exit 1; fi
if [ "${#ports[@]}" -eq 1 ]; then
  port="${ports[0]}"
else
  echo 'Select the ESP port:'
  select port in "${ports[@]}"; do [ -n "$port" ] && break; done
  [ -n "${port:-}" ] || exit 1
fi
echo "Building ESP32-C3 firmware and uploading to $port..."
fqbn=esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M
"$cli" compile --fqbn "$fqbn" --build-path build/cache/c3 --output-dir build/c3 firmware/lrf_hf_bridge
"$cli" upload --fqbn "$fqbn" --port "$port" --input-dir build/c3 firmware/lrf_hf_bridge
echo 'SUCCESS. Wi-Fi: lidar_bridge / 00000000 (unless customized), http://legion.lidar, first 5 minutes.'
