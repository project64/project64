@ECHO OFF
TITLE MinGW Compiler Suite Invocation

set src=%CD%\..\..\Glide64
set obj=%CD%\Glide64

if not exist %obj% (
mkdir %obj%
)

    set MinGW=C:\MinGW
REM set MinGW=C:\msys64\mingw64\x86_64-w64-mingw32\..

set FLAGS_x86=^
 -I%src%\..^
 -I%src%\..\3rdParty\wx\include^
 -I%src%\..\3rdParty\wx\lib\vc_lib\msw^
 -I%src%\..\Glitch64\inc^
 -S^
 -masm=intel^
 -march=native^
 -Os

set C_FLAGS=%FLAGS_x86%

cd %MinGW%\bin
set CC=%MinGW%\bin\g++.exe
set AS=%MinGW%\bin\as.exe

ECHO Compiling Glide64 plugin sources...
%CC% -o %obj%\Main.asm                  %src%\Main.cpp %C_FLAGS%
%CC% -o %obj%\FBtoScreen.asm            %src%\FBtoScreen.cpp %C_FLAGS%
%CC% -o %obj%\rdp.asm                   %src%\rdp.cpp %C_FLAGS%
%CC% -o %obj%\Keys.asm                  %src%\Keys.cpp %C_FLAGS%
%CC% -o %obj%\Config.asm                %src%\Config.cpp %C_FLAGS%
%CC% -o %obj%\CRC.asm                   %src%\CRC.cpp %C_FLAGS%
%CC% -o %obj%\Debugger.asm              %src%\Debugger.cpp %C_FLAGS%
%CC% -o %obj%\Util.asm                  %src%\Util.cpp %C_FLAGS%
%CC% -o %obj%\TexCache.asm              %src%\TexCache.cpp %C_FLAGS%
%CC% -o %obj%\3dmath.asm                %src%\3dmath.cpp %C_FLAGS%
%CC% -o %obj%\Combine.asm               %src%\Combine.cpp %C_FLAGS%
%CC% -o %obj%\DepthBufferRender.asm     %src%\DepthBufferRender.cpp %C_FLAGS%
%CC% -o %obj%\Ext_TxFilter.asm          %src%\Ext_TxFilter.cpp %C_FLAGS%
%CC% -o %obj%\TexBuffer.asm             %src%\TexBuffer.cpp %C_FLAGS%

ECHO Assembling Glide64 sources...
%AS% -o %obj%\Main.o                    %obj%\Main.asm
%AS% -o %obj%\FBtoScreen.o              %obj%\FBtoScreen.asm
%AS% -o %obj%\rdp.o                     %obj%\rdp.asm
%AS% -o %obj%\Keys.o                    %obj%\Keys.asm
%AS% -o %obj%\Config.o                  %obj%\Config.asm
%AS% -o %obj%\CRC.o                     %obj%\CRC.asm
%AS% -o %obj%\Debugger.o                %obj%\Debugger.asm
%AS% -o %obj%\Util.o                    %obj%\Util.asm
%AS% -o %obj%\TexCache.o                %obj%\TexCache.asm
%AS% -o %obj%\3dmath.o                  %obj%\3dmath.asm
%AS% -o %obj%\Combine.o                 %obj%\Combine.asm
%AS% -o %obj%\DepthBufferRender.o       %obj%\DepthBufferRender.asm
%AS% -o %obj%\Ext_TxFilter.o            %obj%\Ext_TxFilter.asm
%AS% -o %obj%\TexBuffer.o               %obj%\TexBuffer.asm
ECHO.

set OBJ_LIST=^
 %obj%\TexBuffer.o^
 %obj%\Ext_TxFilter.o^
 %obj%\DepthBufferRender.o^
 %obj%\Combine.o^
 %obj%\3dmath.o^
 %obj%\TexCache.o^
 %obj%\Util.o^
 %obj%\Debugger.o^
 %obj%\CRC.o^
 %obj%\Config.o^
 %obj%\Keys.o^
 %obj%\rdp.o^
 %obj%\FBtoScreen.o^
 %obj%\Main.o

ECHO Linking PJGlide64 objects...
%MinGW%\bin\g++.exe -o %obj%\PJ64Glide64.dll %OBJ_LIST% -shared -shared-libgcc
PAUSE
