@echo off
rem Remove generated objects and dependency files.
setlocal
set ROOT=%~dp0..
make -C "%ROOT%" clean
if errorlevel 1 exit /b 1
endlocal
