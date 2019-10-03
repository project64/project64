@echo off
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..
set base_dir=%cd%
cd /d %origdir%

for /f %%i in ('git describe --tags --long') do set GIT_DESCRIBE=%%i > nul

for /F "tokens=1,2,3 delims=-" %%i in ("%GIT_DESCRIBE%") do call :process_git_desc %%i %%j %%k
for /F "tokens=1,2,3" %%i in (%base_dir%\Source\Project64-core\version.h.in) do call :process_version %%i %%j %%k
set VERSION=v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_REVISION%-%VERSION_BUILD%-%VERSION_COMMIT%

echo %VERSION%
echo GIT_DESCRIBE = %VERSION% > "%base_dir%\git.properties"
goto :EOF


:process_git_desc
set VERSION_BUILD=%2
set VERSION_COMMIT=%3
goto :EOF

:process_version
if "%1" == "#define" if "%2" == "VERSION_MAJOR" set VERSION_MAJOR=%3
if "%1" == "#define" if "%2" == "VERSION_MINOR" set VERSION_MINOR=%3
if "%1" == "#define" if "%2" == "VERSION_REVISION" set VERSION_REVISION=%3
goto :EOF
