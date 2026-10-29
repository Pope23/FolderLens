# FolderLens – Pornire în Windows (fără Ubuntu / WSL / make)

## Varianta cea mai simplă: Visual Studio (NU Visual Studio Code)

1. Deschide `FolderLens.sln` prin dublu-click.
2. Dacă Visual Studio cere să instalezi componentele C++, alege `Desktop development with C++`.
3. În bara de sus alege `Release` și `x64`.
4. Apasă `Ctrl + Shift + B` (Build Solution).
5. Deschide PowerShell în folderul `FolderLens` și rulează:

```powershell
.\build\Release\FolderLens.exe .\demo_data --top 5 --csv .\build\report.csv
```

Va apărea un raport cu tipurile de fișiere și cele mai mari fișiere.
Fișierul `build\report.csv` poate fi deschis în Excel.

Ca să analizezi un folder propriu (nu e modificat nimic), folosește:

```powershell
.\build\Release\FolderLens.exe "E:\Proiecte pt cv" --top 10 --csv .\build\report.csv
```

## Dacă ai deja g++ (MinGW) instalat

Deschide PowerShell în directorul FolderLens:

```powershell
g++ --version
```

Dacă afișează versiunea, rulează:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -o FolderLens.exe src\main.cpp
.\FolderLens.exe .\demo_data --top 5 --csv report.csv
```

Dacă g++ nu este recunoscut, Visual Studio cu `Desktop development with C++` este cea mai simplă cale.

## Dacă întâmpini o eroare

- `g++ is not recognized` – compilatorul g++ nu este instalat sau nu e în PATH. Folosește Visual Studio.
- `FolderLens.exe is not recognized` – fișierul nu a fost compilat sau calea este greșită.
- `folder does not exist` – pune calea între ghilimele, mai ales dacă are spații.
- `cannot write CSV` – schimbă calea raportului într-un folder în care ai drept de scriere.

## Demonstrație pentru GitHub

1. Compilează și rulează aplicația pe `demo_data`.
2. Fă o captură de ecran în terminal.
3. Adaugă captura în `docs/` și în README.
4. Publică **sursele**, README și fișierele demo. Nu urca `build/`, `.exe` sau rapoarte cu date personale.
5. Adaugă o secțiune cu ce ai învățat: `std::filesystem`, `std::vector`, `std::map`, sortare și error handling.
