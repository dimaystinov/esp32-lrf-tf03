param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^COM[0-9]+$')]
    [string]$Port
)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
        throw 'Install Arduino CLI and reopen PowerShell.'
    }
    if (-not (Test-Path 'firmware/lrf_hf_bridge/Secrets.h')) {
        throw 'Copy Secrets.example.h to Secrets.h and configure Wi-Fi first.'
    }
    $boardFqbn = 'esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M'
    & arduino-cli compile --fqbn $boardFqbn --build-path build/cache/c3 --output-dir build/c3 firmware/lrf_hf_bridge
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; upload cancelled.' }
    & arduino-cli upload --fqbn $boardFqbn --port $Port --input-dir build/c3 firmware/lrf_hf_bridge
    if ($LASTEXITCODE -ne 0) { throw 'Upload failed. Check USB cable, COM port and BOOT mode.' }
    Write-Host 'Upload complete. Open Serial Monitor at 115200 baud.'
} finally {
    Pop-Location
}
