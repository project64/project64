@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%

cd /d %origdir%

echo copy 3rdParty/7zip
xcopy "%base_dir%/Source/3rdParty/7zip" "%base_dir%/Android/jni/3rdParty/7zip/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy 3rdParty/png
xcopy "%base_dir%/Source/3rdParty/png" "%base_dir%/Android/jni/3rdParty/png/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy 3rdParty/zlib
xcopy "%base_dir%/Source/3rdParty/zlib" "%base_dir%/Android/jni/3rdParty/zlib/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy Common
xcopy "%base_dir%/Source/Common" "%base_dir%/Android/jni/Common/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy PluginAudio
xcopy "%base_dir%/Source/Android/PluginAudio" "%base_dir%/Android/jni/PluginAudio/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy PluginInput
xcopy "%base_dir%/Source/Android/PluginInput" "%base_dir%/Android/jni/PluginInput/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy PluginRSP
xcopy "%base_dir%/Source/Android/PluginRSP" "%base_dir%/Android/jni/PluginRSP/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy Project64-bridge
xcopy "%base_dir%/Source/Android/Bridge" "%base_dir%/Android/jni/Project64-bridge/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy Project64-core
xcopy "%base_dir%/Source/Project64-core" "%base_dir%/Android/jni/Project64-core/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy Project64-video
xcopy "%base_dir%/Source/Project64-video" "%base_dir%/Android/jni/Project64-video/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo copy Settings
xcopy "%base_dir%/Source/Settings" "%base_dir%/Android/jni/Settings/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

ENDLOCAL
exit /B 0
