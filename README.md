# FolderLens — Smart File Analyzer (C++17)

A **read-only, dependency-free C++17 CLI tool** for discovering what's taking up disk space. Built as a Windows-friendly portfolio project with a real use case and clean STL-based implementation.

## Features

- Recursive directory scan using `std::filesystem`
- Total file count, subfolder count, and occupied space
- Extension breakdown sorted by total bytes
- Top N largest files
- Optional CSV export (open the report in Excel)
- Error handling for inaccessible files and directories
- **Read-only**: never deletes, renames or modifies the files it scans

## Quick start on Windows

### Option A — Visual Studio (recommended)

1. Open `FolderLens.sln` in Visual Studio (the full IDE, **not** VS Code).
2. Ensure **Desktop development with C++** is installed.
3. Select **Release** and **x64**, then **Build > Build Solution** (`Ctrl+Shift+B`).
4. Open PowerShell in this repository and run:

```powershell
.\build\Release\FolderLens.exe .\demo_data --top 5 --csv .\build\report.csv
```

### Option B — g++ already installed

In PowerShell, at the repository root:

```powershell
.\build_windows.ps1
```

If PowerShell disallows scripts, run the one-line compiler command instead:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -o FolderLens.exe src\main.cpp
.\FolderLens.exe .\demo_data --top 5 --csv report.csv
```

If `g++` is not recognized, you need a C++ compiler; VS Code alone does not install one.

## Usage

```powershell
.\build\Release\FolderLens.exe "E:\Proiecte pt cv" --top 10 --csv report.csv
```

```text
Usage: FolderLens [FOLDER] [--top NUMBER] [--csv FILE]
  --top NUMBER   Top largest files (default 10)
  --csv FILE     Save per-file CSV report
  --help         Display help
```

## What it demonstrates

- Modern C++17 and the Standard Template Library (STL)
- Recursive filesystem operations and `std::error_code`
- Data modeling with `struct`, `std::vector`, `std::map`
- Aggregation, sorting, and partial sorting
- Command-line argument parsing and CSV serialization
- Release builds and basic PowerShell smoke tests

## Limitations / roadmap

- No UI yet: intentional CLI-first design
- Scans file sizes rather than actual *allocated disk blocks*
- Does not follow symlink directory targets
- Some filesystem errors may prevent traversing the remainder of a branch
- Next: dedicated unit tests, tests with large datasets, extension filters, JSON output, GUI

## Project structure

```text
FolderLens/
├── FolderLens.sln             Visual Studio solution
├── FolderLens.vcxproj         MSVC project (C++17)
├── src/main.cpp               Scanner, reporting and CSV export
├── demo_data/                 Example folder to scan
├── tests/smoke_test.ps1       Automated smoke test
├── build_windows.ps1          Quick build with g++ or cl.exe
├── START_AICI_RO.md           Romanian setup guide
├── docs/demo_output.txt       Example console output
└── README.md
```

## Safety

The application only reads metadata. The CSV output is written only when `--csv` is explicitly provided. Do not scan private folders when demonstrating the tool publicly, and don't commit personal filesystem reports to GitHub.

## License

MIT (see `LICENSE`).
