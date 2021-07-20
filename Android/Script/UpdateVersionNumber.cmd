@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Android\config\version.gradle.in" "%base_dir%\Android\config\version.gradle"

:End
ENDLOCAL
exit /B 0