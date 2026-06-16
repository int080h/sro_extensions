@echo off
setlocal
pushd "%~dp0" || exit /b 1

echo Configuring SroWorldEditor with CMake...
cmake -B build -G "Visual Studio 17 2022" -A Win32 || goto :fail

echo Building SroWorldEditor (Release)...
cmake --build build --config Release --parallel || goto :fail

echo.
echo Build succeeded: output\Release\SroWorldEditor\SroWorldEditor.exe
popd
exit /b 0

:fail
echo Build failed with code %ERRORLEVEL%.
popd
exit /b %ERRORLEVEL%
