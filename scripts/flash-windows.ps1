$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    if (-not [Environment]::Is64BitOperatingSystem) { throw '64-bit Windows is required.' }
    $version = '1.5.1'
    $toolsDir = Join-Path $root '.tools'
    $cli = Join-Path $toolsDir 'arduino-cli.exe'
    if (-not (Test-Path $cli)) {
        New-Item -ItemType Directory -Force $toolsDir | Out-Null
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $asset = "arduino-cli_${version}_Windows_64bit.zip"
        $base = "https://github.com/arduino/arduino-cli/releases/download/v$version"
        $archive = Join-Path $toolsDir $asset
        Write-Host 'Downloading Arduino CLI...'
        Invoke-WebRequest "$base/$asset" -OutFile $archive -UseBasicParsing
        $checks = (Invoke-WebRequest "$base/$version-checksums.txt" -UseBasicParsing).Content
        $line = @($checks -split "`n" | Where-Object { $_.Trim().EndsWith($asset) })
        if ($line.Count -ne 1) { throw 'Missing download checksum.' }
        $expected = ($line[0].Trim() -split '\s+')[0]
        if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expected) { throw 'Download checksum mismatch.' }
        Expand-Archive -Path $archive -DestinationPath $toolsDir -Force
        Remove-Item $archive
    }
    function Invoke-Arduino {
        & $cli @args
        if ($LASTEXITCODE -ne 0) { throw "Arduino CLI failed: $($args -join ' ')" }
    }
    $cores = Invoke-Arduino core list --format json | ConvertFrom-Json
    $installed = @($cores.platforms | Where-Object { $_.id -eq 'esp32:esp32' -and $_.installed_version -eq '3.3.11' })
    if ($installed.Count -eq 0) {
        Write-Host 'Installing ESP32 core 3.3.11 (first run may take several minutes)...'
        $index = 'https://espressif.github.io/arduino-esp32/package_esp32_index.json'
        Invoke-Arduino core update-index --additional-urls $index
        Invoke-Arduino core install esp32:esp32@3.3.11 --additional-urls $index
    }
    $secrets = 'firmware/lrf_hf_bridge/Secrets.h'
    if (-not (Test-Path $secrets)) { Copy-Item 'firmware/lrf_hf_bridge/Secrets.example.h' $secrets }
    $boards = Invoke-Arduino board list --format json | ConvertFrom-Json
    $ports = @($boards.detected_ports | Where-Object { $_.port.protocol -eq 'serial' -and $_.port.address -match '^COM[0-9]+$' })
    $esp = @($ports | Where-Object { $_.port.properties.vid -eq '0x303A' -and $_.port.properties.pid -eq '0x1001' })
    if ($esp.Count -eq 1) { $port = $esp[0].port.address }
    else {
        if ($ports.Count -eq 0) { throw 'No COM port found. Connect ESP using a data USB cable; try BOOT mode.' }
        $ports | ForEach-Object { Write-Host ($_.port.address + ' ' + $_.port.label) }
        $port = (Read-Host 'Enter the ESP COM port (for example COM5)').Trim().ToUpperInvariant()
        if ($port -notin @($ports | ForEach-Object { $_.port.address })) { throw 'Port is not in the detected list.' }
    }
    Write-Host "Building ESP32-C3 firmware and uploading to $port..."
    $fqbn = 'esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M'
    Invoke-Arduino compile --fqbn $fqbn --build-path build/cache/c3 --output-dir build/c3 firmware/lrf_hf_bridge
    Invoke-Arduino upload --fqbn $fqbn --port $port --input-dir build/c3 firmware/lrf_hf_bridge
    Write-Host 'SUCCESS. Wi-Fi: lidar_bridge / 00000000 (unless customized), http://legion.lidar, first 5 minutes.'
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    exit 1
} finally {
    Pop-Location
}
