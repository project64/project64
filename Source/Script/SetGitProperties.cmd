@echo off
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..
set base_dir=%cd%
cd /d %origdir%

for /f %%i in ('git rev-parse --short HEAD') do set GIT_REVISION_SHORT=%%i > nul
for /f %%i in ('git rev-list --count HEAD') do set GIT_BUILD_VERSION=%%i > nul

for /F "tokens=1,2,3" %%i in (%base_dir%\Source\Project64-core\version.h.in) do call :process_version %%i %%j %%k
set GIT_REVISION_SHORT=%GIT_REVISION_SHORT: =%
set GIT_BUILD_VERSION=%GIT_BUILD_VERSION: =%
set VERSION=%VERSION_PREFIX%%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_REVISION%-%GIT_BUILD_VERSION%-%GIT_REVISION_SHORT%

echo %VERSION%
echo GIT_DESCRIBE = %VERSION% > "%base_dir%\git.properties"
goto :EOF

:process_version
if "%1" == "#define" if "%2" == "VERSION_MAJOR" set VERSION_MAJOR=%3
if "%1" == "#define" if "%2" == "VERSION_MINOR" set VERSION_MINOR=%3
if "%1" == "#define" if "%2" == "VERSION_REVISION" set VERSION_REVISION=%3
if "%1" == "#define" if "%2" == "VERSION_PREFIX" set VERSION_PREFIX=%~3
goto :EOF
