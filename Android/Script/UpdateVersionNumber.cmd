@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Android\config\version.gradle.in" "%base_dir%\Android\config\version.gradle"
"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Source\Project64-core\Version.h.in" "%base_dir%\Source\Project64-core\Version.h"
"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Source\Project64-rsp-core\Version.h.in" "%base_dir%\Source\Project64-rsp-core\Version.h"
"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Source\Project64-audio\Version.h.in" "%base_dir%\Source\Project64-audio\Version.h"
"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Source\Project64-video\Version.h.in" "%base_dir%\Source\Project64-video\Version.h"
"%base_dir%\Android\Script\UpdateVersion.exe" "%base_dir%\Source\RSP\Version.h.in" "%base_dir%\Source\RSP\Version.h"

:End
ENDLOCAL
exit /B 0