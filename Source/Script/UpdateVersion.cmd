@echo off
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..
set base_dir=%cd%
cd /d %origdir%


if exist "C:\Program Files\Git\usr\bin\sed.exe" ( set SED="C:\Program Files\Git\usr\bin\sed.exe")
if exist "C:\Program Files (x86)\Git\bin\sed.exe" ( set SED="C:\Program Files (x86)\Git\bin\sed.exe")

if %SED% == "" ( 
	echo can not find sed.exe
	goto :end
)

SETLOCAL EnableDelayedExpansion
FOR /F "tokens=1 delims=" %%A in ('git describe --tags --long') do SET current_tag=%%A
FOR /F "tokens=1 delims=" %%A in ('echo !current_tag! ^| !sed! "s/v[0-9]*\.[0-9]*\.[0-9]*-\([0-9]*\).*/\1/"') do SET commits_since_tag=%%A

call :setVersion "%base_dir%\Source\Project64-core\version.h" !commits_since_tag!
call :setVersion "%base_dir%\Source\nragev20\version.h" !commits_since_tag!
call :setVersion "%base_dir%\Source\RSP\version.h" !commits_since_tag!
call :setVersion "%base_dir%\Source\Project64-video\version.h" !commits_since_tag!

ENDLOCAL

goto :eof

:setVersion
set version_file=%~1
set out_file=%~1.out
set build_no=%~2

if exist "%out_file%" del "%out_file%"

SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ "%version_file%""`) do (
    set "line=%%a"
    SETLOCAL EnableDelayedExpansion
    set "line=!line:9999=%build_no%!"
    set "line=!line:*:=!"
    echo(!line!>>!out_file!
    ENDLOCAL
)
ENDLOCAL

if exist "%out_file%" ( 
	if exist "%version_file%" ( 
		del "%version_file%"
		move "%out_file%" "%version_file%"
	)
)

goto :eof
