@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build-cpp-ninja -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
if errorlevel 1 goto fail
cmake --build build-cpp-ninja --target key_note_overlay_cpp
if errorlevel 1 goto fail
echo.
echo Built: %~dp0build-cpp-ninja\key_note_overlay_cpp.exe
exit /b 0

:fail
echo.
echo C++ overlay build failed.
set /p _=Press Enter to close...
exit /b 1
