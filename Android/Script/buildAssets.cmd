@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%

cd /d %origdir%

echo Building Lang files
xcopy "%base_dir%/Lang" "%base_dir%/Android/assets/project64_data/Lang/" /D /I /F /Y /E
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

echo Copy config files
IF NOT EXIST "%base_dir%/Android/assets/project64_data/Config/" mkdir "%base_dir%/Android/assets/project64_data/Config/"
copy "%base_dir%\Config\Project64Video.rdb" "%base_dir%\Android\assets\project64_data\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Project64.cht" "%base_dir%\Android\assets\project64_data\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Project64.rdb" "%base_dir%\Android\assets\project64_data\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

copy "%base_dir%\Config\Project64.rdx" "%base_dir%\Android\assets\project64_data\Config\"
IF %ERRORLEVEL% NEQ 0 (exit /B 1)

goto :end

:End
ENDLOCAL
exit /B 0
