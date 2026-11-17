@echo off
rem Build the selected application on Windows.
setlocal
set ROOT=%~dp0..
make -C "%ROOT%" app
if errorlevel 1 exit /b 1
endlocal
