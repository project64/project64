@ECHO OFF
SETLOCAL

set BuildMode=Release
if not "%1" == "" set BuildMode=%1
if "%1" == "debug" set BuildMode=Debug
if "%1" == "release" set BuildMode=Release

set MSVC-BUILDER=
set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

if exist "C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" ( set MSVC-BUILDER="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe")
if exist "C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" ( set MSVC-BUILDER="C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe")

if %MSVC-BUILDER% == "" ( 
	echo can not find visual studio 2008
	goto :end
)

:: Build Win32 version of the software
IF EXIST "%base_dir%\output.txt" del "%base_dir%\output.txt"
%MSVC-BUILDER% "%base_dir%\Project64.vs2008.sln" /rebuild "%BuildMode%|Win32" /out "%base_dir%\output.txt"
set Result=%ERRORLEVEL%
type "%base_dir%\output.txt"
echo Done - ERRORLEVEL: %Result%
IF %Result% NEQ 0 goto :EndErr

echo Build ok
goto :end

:EndErr
ENDLOCAL
echo Build failed
exit /B 1

:End
ENDLOCAL
exit /B 0
