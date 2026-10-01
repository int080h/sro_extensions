@echo off
setlocal

set "CONFIG=Release"
if /i "%1"=="debug" set "CONFIG=Debug"

set "CHECK="
if /i "%1"=="check" set "CHECK=-Check"
if /i "%2"=="check" set "CHECK=-Check"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration %CONFIG% %CHECK%
exit /b %ERRORLEVEL%
