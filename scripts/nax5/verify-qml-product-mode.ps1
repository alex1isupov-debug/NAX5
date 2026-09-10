$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$qml = Join-Path $root "gui\src\qml"

$settings = Get-Content -Raw (Join-Path $qml "SettingsDialog.qml")
$mainView = Get-Content -Raw (Join-Path $qml "MainView.qml")
$operator = Get-Content -Raw (Join-Path $qml "Nax5OperatorPanel.qml")
$play = Get-Content -Raw (Join-Path $qml "Nax5PlayPanel.qml")
$qrc = Get-Content -Raw (Join-Path $qml "qml.qrc")

if ($settings -notmatch 'id: consoles\s+visible: Chiaki.operatorMode') { throw "Consoles tab must be operator-only" }
if ($settings -notmatch 'id: remote\s+visible: Chiaki.operatorMode') { throw "Remote/PSN tab must be operator-only" }
if ($settings -notmatch 'id: exportButton\s+visible: Chiaki.operatorMode') { throw "Export must be operator-only" }
if ($settings -notmatch 'id: importButton\s+visible: Chiaki.operatorMode') { throw "Import must be operator-only" }
if ($mainView -notmatch 'visible: Chiaki.operatorMode') { throw "MainView consoles UI must be operator-only" }
if ($operator -notmatch 'visible: Chiaki.operatorMode') { throw "Operator panel must be operator-only" }
if ($operator -match 'nax5ProvisionHost\(0,') { throw "Operator provision must not hardcode host index 0" }
if ($operator -notmatch 'nax5ProvisionHost\(hostIndex,') { throw "Operator provision must use selected hostIndex" }
if ($play -notmatch 'Nax5Session.play\(\)') { throw "Play UI must stay visible in product" }
if ($qrc -notmatch 'Nax5OperatorPanel.qml') { throw "Operator QML must be in qml.qrc" }
if ($qrc -notmatch 'Nax5PlayPanel.qml') { throw "Play QML must be in qml.qrc" }

Write-Host "ok product QML hides Register/Consoles/PSN/export"
Write-Host "ok operator QML remains available behind operatorMode"
