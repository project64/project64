@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

echo buildAssets
call "%base_dir%\Android\Script\buildAssets.cmd"
set Result=%ERRORLEVEL%
echo Done - ERRORLEVEL: %Result%
IF %Result% NEQ 0 goto :EndErr

echo UpdateVersionNumber
call "%base_dir%\Android\Script\UpdateVersionNumber.cmd"
set Result=%ERRORLEVEL%
echo Done - ERRORLEVEL: %Result%
IF %Result% NEQ 0 goto :EndErr

echo copySource
call "%base_dir%\Android\Script\copySource.cmd"
set Result=%ERRORLEVEL%
echo Done - ERRORLEVEL: %Result%
IF %Result% NEQ 0 goto :EndErr

goto :end

:EndErr
ENDLOCAL
echo Build failed
exit /B 1

:End
ENDLOCAL
exit /B 0
