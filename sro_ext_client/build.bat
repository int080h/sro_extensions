@echo off
setlocal
pushd "%~dp0" || exit /b 1
cmake --preset win32-release || goto :fail
cmake --build --preset win32-release || goto :fail
echo.
echo Output: ..\output\Release\version.dll + ext_client.dll
popd
exit /b 0

:fail
set "status=%ERRORLEVEL%"
popd
exit /b %status%
