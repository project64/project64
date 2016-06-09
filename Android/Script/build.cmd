@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

set NDK-BUILDER=
if exist "C:\Android\android-ndk-r11c\ndk-build.cmd" ( set NDK-BUILDER="C:\Android\android-ndk-r11c\ndk-build.cmd" )
if %NDK-BUILDER% == "" ( 
	echo can not find android NDK
	goto :EndErr
)

call "%base_dir%\Android\Script\buildAssets.cmd"
IF %ERRORLEVEL% NEQ 0 goto :EndErr

call "%base_dir%\Android\Script\copySource.cmd"
IF %ERRORLEVEL% NEQ 0 goto :EndErr

cd /d %base_dir%\Android
call %NDK-BUILDER% clean
call %NDK-BUILDER%
cd /d %origdir%

echo Build ok
goto :end

:EndErr
ENDLOCAL
echo Build failed
exit /B 1

:End
ENDLOCAL
exit /B 0