param([string]$Exe = ".\build\FolderLens.exe")
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
if (-not (Test-Path $Exe)) { throw "Missing executable: $Exe" }
$output = & $Exe demo_data --top 3 --csv build\smoke_report.csv | Out-String
if ($LASTEXITCODE -ne 0) { throw "FolderLens failed to run" }
if ($output -notmatch 'SMART FILE ANALYZER') { throw "Missing report title" }
if ($output -notmatch 'Read-only scan complete') { throw "Missing successful completion" }
if (-not (Test-Path build\smoke_report.csv)) { throw "CSV was not created" }
$rows = Import-Csv build\smoke_report.csv
if ($rows.Count -lt 3) { throw "Expected at least 3 demo files" }
Write-Host "Smoke test passed: $($rows.Count) files found." -ForegroundColor Green
