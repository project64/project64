@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%

cd /d %origdir%

echo Building Lang files
xcopy "%base_dir%/Lang" "%base_dir%/Android/assets/Lang/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo Copy config files
IF NOT EXIST "%base_dir%/Android/assets/Config/" mkdir "%base_dir%/Android/assets/Config/"
copy "%base_dir%\Config\Audio.rdb" "%base_dir%\Android\assets\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Video.rdb" "%base_dir%\Android\assets\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Project64.rdb" "%base_dir%\Android\assets\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Project64.rdx" "%base_dir%\Android\assets\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

IF NOT EXIST "%base_dir%/Android/assets/Config/Cheats/" mkdir "%base_dir%/Android/assets/Config/Cheats/"
xcopy "%base_dir%/Config/Cheats" "%base_dir%/Android/assets/Config/Cheats/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

IF NOT EXIST "%base_dir%/Android/assets/Config/Enhancements/" mkdir "%base_dir%/Android/assets/Config/Enhancements/"
xcopy "%base_dir%/Config/Enhancements" "%base_dir%/Android/assets/Config/Enhancements/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

IF NOT EXIST "%base_dir%/Android/app/src/main/assets/" mkdir "%base_dir%/Android/app/src/main/assets/"
IF EXIST "%base_dir%/Android/app/src/main/assets/assets.zip" del "%base_dir%\Android\app\src\main\assets\assets.zip"
powershell Compress-Archive "%base_dir%/Android/assets/*" "%base_dir%/Android/app/src/main/assets/assets.zip"

goto :end

:End
ENDLOCAL
exit /B 0
