@echo off
setlocal
cd /d "%~dp0"
if not exist "build-cpp-ninja\key_note_overlay_cpp.exe" (
  call "%~dp0build_cpp_overlay.bat"
  if errorlevel 1 exit /b 1
)
start "" /D "%~dp0" "%~dp0build-cpp-ninja\key_note_overlay_cpp.exe"
