$ErrorActionPreference = "Stop"
$script = Join-Path $PSScriptRoot "start-nax5-local.cmd"
$operator = Join-Path $PSScriptRoot "start-nax5-operator-local.cmd"
if (-not (Test-Path $script)) { throw "missing start-nax5-local.cmd" }
if (-not (Test-Path $operator)) { throw "missing start-nax5-operator-local.cmd" }

$local = Get-Content -Raw $script
$op = Get-Content -Raw $operator

if ($local -notmatch 'NAX5_API_BASE_URL=http://127\.0\.0\.1:8000') { throw "local helper must set localhost API URL" }
if ($local -match 'start\s+""') { throw "local helper must not use start, which drops the environment" }
if ($local -notmatch '"%NAX5_EXE%"') { throw "local helper must launch chiaki.exe in-process" }
if ($op -notmatch 'NAX5_OPERATOR_MODE=1') { throw "operator helper must set operator mode" }
if ($op -match 'start\s+""') { throw "operator helper must not use start" }

Write-Host "ok start-nax5-local.cmd"
Write-Host "ok start-nax5-operator-local.cmd"
