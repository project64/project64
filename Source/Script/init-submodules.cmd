@echo off
setlocal

git submodule update --init --recursive
if not errorlevel 1 exit /b 0

echo GLideN64 commit is not on the upstream repository yet. Fetching chronic8000/GLideN64.
for /f %%i in ('git rev-parse HEAD:Source/GLideN64/Source') do set SHA=%%i
if not defined SHA exit /b 1

git -C Source/GLideN64/Source fetch https://github.com/chronic8000/GLideN64.git arm64-windows
if errorlevel 1 exit /b 1
git -C Source/GLideN64/Source checkout --detach %SHA%
if errorlevel 1 exit /b 1

git submodule update --init --recursive external/sdl
exit /b %ERRORLEVEL%
