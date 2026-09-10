$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$transient = Join-Path $root "gui\src\nax5\connection\nax5transienthost.cpp"
$sessionH = Join-Path $root "gui\include\streamsession.h"
$sessionCpp = Join-Path $root "gui\src\streamsession.cpp"
$settings = Join-Path $root "gui\src\settings.cpp"
$hostCpp = Join-Path $root "gui\src\host.cpp"
$lib = Join-Path $root "lib"

if (-not (Test-Path $transient)) { throw "missing nax5transienthost.cpp" }

$src = Get-Content -Raw $transient
if ($src -notmatch 'StreamSessionConnectInfo\(') { throw "adapter must call StreamSessionConnectInfo" }
if ($src -notmatch 'settings,') { throw "adapter must pass Settings*" }
if ($src -notmatch 'QString\(\)') { throw "adapter must pass empty duid" }
if ($src -match 'AddRegisteredHost|SaveRegisteredHosts|SetManualHost') {
    throw "transient adapter must not persist hosts"
}

foreach ($protected in @($sessionCpp, $settings, $hostCpp)) {
    if (-not (Test-Path $protected)) { throw "missing $protected" }
}

if (-not (Test-Path (Join-Path $lib "include\chiaki\session.h"))) {
    throw "missing chiaki session.h"
}

$header = Get-Content -Raw $sessionH
if ($header -notmatch 'Settings \*settings') { throw "StreamSessionConnectInfo must still take Settings*" }

Write-Host "ok settings-contract: transient adapter reuses StreamSessionConnectInfo(Settings*)"
Write-Host "ok protected core files are present and not referenced for persistence"
