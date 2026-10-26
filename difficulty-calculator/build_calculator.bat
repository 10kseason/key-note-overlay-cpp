@echo off
setlocal
cd /d "%~dp0"
if not defined CXX (
  if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "CXX=C:\msys64\mingw64\bin\g++.exe"
  ) else (
    set "CXX=g++"
  )
)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="%CXX%"
if errorlevel 1 exit /b 1
cmake --build build --parallel 4
if errorlevel 1 exit /b 1
ctest --test-dir build --output-on-failure
if errorlevel 1 exit /b 1
echo Built: build\ChartDifficultyCalculator.exe
