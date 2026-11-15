# Sticky Lab in den zweiten Firmware-Platz (app1, 0x410000) schreiben – EDC in app0 bleibt, wie sie ist.
# SPEC Abschnitt 6 „EDC und Lab auf einem Gerät“. Danach startet der Sticky weiter mit EDC; ins Lab geht es
# über die Menü-Kachel „Sticky Lab“ (EDC ab 0.33.0) oder mit ▲ beim Einschalten (1 s halten).
#
#   powershell -ExecutionPolicy Bypass -File werkzeuge\lab-nach-app1.ps1 -Port COM10
#   … -OhneBauen   nimmt das zuletzt gebaute Abbild
#
# Nicht `arduino-cli upload` fürs Lab nehmen: das schreibt nach app0 und überschreibt EDC.
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [switch]$OhneBauen
)
$ErrorActionPreference = 'Stop'
$board = 'esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc'
$bau = Join-Path $env:LOCALAPPDATA 'sticky-build\sticky-lab'
$sketch = Join-Path $PSScriptRoot '..\firmware'

if (-not $OhneBauen) {
    arduino-cli compile -b $board --build-property build.flash_size=32MB --build-path $bau $sketch
    if ($LASTEXITCODE -ne 0) { throw 'Bauen fehlgeschlagen' }
}
$bin = Join-Path $bau 'firmware.ino.bin'
if (-not (Test-Path $bin)) { throw "Kein Abbild unter $bin" }
$groesse = (Get-Item $bin).Length
if ($groesse -gt 0x400000) { throw "Abbild zu groß für app1: $groesse Byte (max. 4 MB)" }

$esptool = Get-ChildItem (Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\tools\esptool_py') -Recurse -Filter esptool.exe |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $esptool) { throw 'esptool.exe nicht gefunden (Arduino-ESP32-Kern installiert?)' }

Write-Host "Lab ($([math]::Round($groesse / 1MB, 2)) MB) → app1 (0x410000) über $Port"
& $esptool.FullName --chip esp32s3 --port $Port --baud 921600 write_flash 0x410000 $bin
if ($LASTEXITCODE -ne 0) { throw 'Schreiben fehlgeschlagen (bei Abbrüchen --baud 460800 probieren)' }
Write-Host 'Fertig. Der Sticky startet weiter mit EDC; ins Lab über Menü → „Sticky Lab“ oder ▲ beim Einschalten.'
