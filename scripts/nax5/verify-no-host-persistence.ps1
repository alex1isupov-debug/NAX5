$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$nax5 = Join-Path $root "gui\src\nax5"
$hits = Get-ChildItem $nax5 -Recurse -Filter *.cpp | Select-String -Pattern "AddRegisteredHost|SaveRegisteredHosts|SetManualHost"
if ($hits) {
    throw "NAX5 sources persist hosts: $($hits.Path -join ', ')"
}
Write-Host "ok NAX5 connection path does not persist registered/manual hosts"
