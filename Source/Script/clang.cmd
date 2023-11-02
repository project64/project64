@echo off
setlocal EnableDelayedExpansion

set origdir=%cd%
cd /d %~dp0..\..
set base_dir=%cd%
cd /d %origdir%

set ScanDir[0]="%base_dir%\Source\Common"
set ScanDir[1]="%base_dir%\Source\Project64"
set ScanDir[2]="%base_dir%\Source\Project64-core"
set ScanDir[3]="%base_dir%\Source\Project64-rsp"
set ScanDir[4]="%base_dir%\Source\Project64-rsp-core"
set ScanDir[5]="%base_dir%\Source\Android\Bridge"
set ScanDir[6]="%base_dir%\Source\Android\PluginRSP"

set ScanFiles[0]="*.cpp"
set ScanFiles[1]="*.h"

set Exclude[0]="%base_dir%\Source\Project64-core\Version.h"
set Exclude[1]="%base_dir%\Source\Project64\UserInterface\resource.h"
set Exclude[2]="%base_dir%\Source\Project64-rsp-core\Version.h"
set Exclude[3]="%base_dir%\Source\Project64-rsp\resource.h"

set ValidParam=0
if "%1" == "check" set ValidParam=1
if "%1" == "format" set ValidParam=1
IF %ValidParam%==0 GOTO :Usage

if "%1" == "check" echo Checking code formatting
if "%1" == "check" set ClangParm=-style=file -Werror --dry-run
if "%1" == "format" set ClangParm=-i -style=file

set /a Result=0

set /a DirectoryIndex=0
:DirectoryLoop 
if defined ScanDir[%DirectoryIndex%] ( 
	CALL :ProcessDirectory %DirectoryIndex%

	set /a DirectoryIndex += 1
	GOTO :DirectoryLoop 
)
exit /b %Result%

:ProcessDirectory
call set Directory=%%ScanDir[%1]%%
set /a ScanFilesIndex=0

:ScanFilesLoop 
if defined ScanFiles[%ScanFilesIndex%] ( 
	CALL :ProcessDirectoryFiles %ScanFilesIndex%

	set /a ScanFilesIndex += 1
	GOTO :ScanFilesLoop 
)
goto :end

:ProcessDirectoryFiles
call set Files=%%ScanFiles[%1]%%
For /R %Directory% %%A In (%Files%) Do (
	::"%base_dir%\bin\clang-format-12.exe" -i -style=file "%%A"
	CALL :ProcessFile "%%A"
)
goto :end

:ProcessFile
set /a ExcludeIndex=0
:ExcludeLoop 
if defined Exclude[%ExcludeIndex%] ( 
	call set ExcludeFile=%%Exclude[%ExcludeIndex%]%%
	if %1==!ExcludeFile! ( 
		goto :end
	)
	set /a ExcludeIndex += 1
	GOTO :ExcludeLoop 
)
"%base_dir%\bin\clang-format-12.exe" %ClangParm% %1
IF %ERRORLEVEL% NEQ 0 set /a Result=1
goto :end

:Usage
echo clang.cmd [format/check]
echo check - checks to see if any code would have to change
echo format - change the code to meet clang formating


:end