@echo off
setlocal

if "%VITTE_BIN%"=="" set "VITTE_BIN=vitte"
if "%WORKDIR%"=="" set "WORKDIR=%TEMP%\vitte-real-install-smoke"

if not exist "%WORKDIR%" mkdir "%WORKDIR%"

> "%WORKDIR%\smoke.vit" echo proc main() -^> int {
>> "%WORKDIR%\smoke.vit" echo   give 0
>> "%WORKDIR%\smoke.vit" echo }

pushd "%WORKDIR%" || exit /b 1

rem Required post-install contract:
"%VITTE_BIN%" --help >nul || exit /b 1
"%VITTE_BIN%" check smoke.vit || exit /b 1
"%VITTE_BIN%" build smoke.vit -o smoke || exit /b 1

if exist smoke.exe smoke.exe || exit /b 1

echo [real-install-smoke] OK bin=%VITTE_BIN% workdir=%WORKDIR%
popd
