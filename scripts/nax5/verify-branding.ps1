$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$requiredFiles = @(
    "gui\nax5.ico",
    "gui\res\nax5.png",
    "gui\res\nax5-logo.png"
)

foreach ($rel in $requiredFiles) {
    $path = Join-Path $root $rel
    if (-not (Test-Path $path)) {
        throw "Missing branding asset: $rel"
    }
}

$qmlFiles = @(
    "gui\src\qml\LoginView.qml",
    "gui\src\qml\MainView.qml",
    "gui\src\qml\SettingsDialog.qml"
)

foreach ($rel in $qmlFiles) {
    $content = Get-Content -Raw (Join-Path $root $rel)
    if ($content -match 'nax5-logo\.svg|chiaking-logo') {
        throw "$rel still references legacy SVG logo"
    }
    if ($content -notmatch 'nax5-logo\.png') {
        throw "$rel must reference qrc:/icons/nax5-logo.png"
    }
}

$mainCpp = Get-Content -Raw (Join-Path $root "gui\src\main.cpp")
if ($mainCpp -notmatch 'QIcon\(":/icons/nax5\.png"\)') {
    throw "main.cpp must use :/icons/nax5.png for the window icon"
}

$qrc = Get-Content -Raw (Join-Path $root "gui\res\resources.qrc")
foreach ($asset in @("nax5.png", "nax5-logo.png")) {
    if ($qrc -notmatch $asset) {
        throw "resources.qrc must include $asset"
    }
}

$icoBytes = [System.IO.File]::ReadAllBytes((Join-Path $root "gui\nax5.ico"))
$layerCount = [BitConverter]::ToUInt16($icoBytes, 4)
if ($layerCount -lt 7) {
    throw "gui/nax5.ico must contain 7 size layers, found $layerCount"
}

Write-Host "ok branding: launcher assets present and wired to PNG logos"
