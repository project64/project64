@echo off
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..
set base_dir=%cd%
cd /d %origdir%

if "%~4"=="" (
    echo Usage: UpdateVersion.cmd [BuildMode] [Platform] [InFile] [OutFile]
    goto :eof
)

if not "%1" == "" set BuildMode=%~1
if not "%~2" == "" set Platform=%~2
if not "%~2" == "" set InFile="%~3"
if not "%~3" == "" set OutFile="%~4"

FOR /F "tokens=1 delims=" %%A in ('git describe --tags --long --dirty') do SET current_tag=%%A

if %Platform%==x64 set BuildMode=%BuildMode%64

REM If host OS is ARM64, use this tool flavor of UpdateVersion. Else, use Win32 (default).
for /F "tokens=3" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v "PROCESSOR_ARCHITECTURE"') DO (set HOST_PROCESSOR_ARCHITECTURE=%%A)
if "%HOST_PROCESSOR_ARCHITECTURE%|%Platform%"=="ARM64|ARM64" set BuildMode=%BuildMode%ARM64

echo "%base_dir%\Bin\%BuildMode%\UpdateVersion.exe" %InFile% %OutFile% "%current_tag%"
"%base_dir%\Bin\%BuildMode%\UpdateVersion.exe" %InFile% %OutFile% "%current_tag%"
