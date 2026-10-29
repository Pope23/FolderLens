$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path "build" | Out-Null

if (Get-Command g++ -ErrorAction SilentlyContinue) {
    & g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o "build\FolderLens.exe" "src\main.cpp"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} elseif (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    Push-Location build
    try {
        & cl.exe /nologo /std:c++17 /EHsc /W4 /O2 /Fe:FolderLens.exe ..\src\main.cpp
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally {
        Pop-Location
    }
} else {
    Write-Error "No C++ compiler found. Install Visual Studio with 'Desktop development with C++', then open FolderLens.sln (or use Developer PowerShell)."
    exit 1
}
Write-Host "Build successful: build\FolderLens.exe" -ForegroundColor Green
& ".\build\FolderLens.exe" ".\demo_data" --top 5 --csv ".\build\report.csv"
