@ECHO OFF
SETLOCAL

set origdir=%cd%
cd /d %~dp0..\..\
set base_dir=%cd%
cd /d %origdir%

set VersionName=
if not "%1" == "" set VersionName= %1

set NDK-BUILDER=""
if exist "C:\Android\android-ndk-r10d\ndk-build.cmd" ( set NDK-BUILDER="C:\Android\android-ndk-r10d\ndk-build.cmd" )
if exist "C:\Android\android-ndk-r11c\ndk-build.cmd" ( set NDK-BUILDER="C:\Android\android-ndk-r11c\ndk-build.cmd" )
if exist "C:\Android\android-ndk-r13b\ndk-build.cmd" ( set NDK-BUILDER="C:\Android\android-ndk-r13b\ndk-build.cmd" )
if %NDK-BUILDER% == "" ( 
    echo can not find android NDK
    goto :EndErr
)

set ANDROID_SDK=""
if exist "C:\Android\android-sdk" ( set ANDROID_SDK="C:\Android\android-sdk" )
if %ANDROID_SDK% == "" ( 
    echo can not find android SDK
    goto :EndErr
)

call "%base_dir%\Android\Script\buildAssets.cmd"
IF %ERRORLEVEL% NEQ 0 goto :EndErr

call "%base_dir%\Android\Script\copySource.cmd"
IF %ERRORLEVEL% NEQ 0 goto :EndErr

cd /d %base_dir%\Android
call %NDK-BUILDER% clean
IF %ERRORLEVEL% NEQ 0 goto :EndErr
call %NDK-BUILDER%
IF %ERRORLEVEL% NEQ 0 goto :EndErr
call ant clean release -Dsdk.dir=%ANDROID_SDK%
IF %ERRORLEVEL% NEQ 0 goto :EndErr
cd /d %origdir%

:: Make sure the sign environment variables exist
IF NOT DEFINED project64_cert_keystore ( exit /B 0 )
IF NOT DEFINED project64_cert_password ( exit /B 0 )

:: Sign the APK
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -tsa http://timestamp.digicert.com -keystore "%project64_cert_keystore%" -storepass %project64_cert_password% -keypass %project64_cert_password% "%base_dir%\Android\bin\Project64-release-unsigned.apk" project64

:: Align the APK
zipalign -v 4 "%base_dir%\Android\bin\Project64-release-unsigned.apk" "%base_dir%\Package\Project64%VersionName%.apk"

echo Build ok
goto :end

:EndErr
ENDLOCAL
echo Build failed
exit /B 1

:End
ENDLOCAL
exit /B 0