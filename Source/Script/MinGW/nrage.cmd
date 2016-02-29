@ECHO OFF
TITLE MinGW Compiler Suite Invocation

set src=%CD%\..\..\nragev20
set obj=%CD%\N-Rage

if not exist %obj% (
mkdir %obj%
)

    set MinGW=C:\MinGW
REM set MinGW=C:\msys64\mingw64\x86_64-w64-mingw32\..

set FLAGS_x86=^
 -I%src%\..\3rdParty\directx\include^
 -Wno-write-strings^
 -S^
 -masm=intel^
 -march=native^
 -Os

set C_FLAGS=%FLAGS_x86%

cd %MinGW%\bin
set CC=%MinGW%\bin\g++.exe
set AS=%MinGW%\bin\as.exe

ECHO Compiling N-Rage plugin sources...
%CC% -o %obj%\NRagePluginV2.asm         %src%\NRagePluginV2.cpp %C_FLAGS%
%CC% -o %obj%\Interface.asm             %src%\Interface.cpp %C_FLAGS%
%CC% -o %obj%\FileAccess.asm            %src%\FileAccess.cpp %C_FLAGS%
%CC% -o %obj%\PakIO.asm                 %src%\PakIO.cpp %C_FLAGS%
%CC% -o %obj%\GBCart.asm                %src%\GBCart.cpp %C_FLAGS%
%CC% -o %obj%\International.asm         %src%\International.cpp %C_FLAGS%
%CC% -o %obj%\DirectInput.asm           %src%\DirectInput.cpp %C_FLAGS%
%CC% -o %obj%\XInputController.asm      %src%\XInputController.cpp %C_FLAGS%

ECHO Assembling N-Rage sources...
%AS% -o %obj%\NRagePluginV2.o           %obj%\NRagePluginV2.asm
%AS% -o %obj%\Interface.o               %obj%\Interface.asm
%AS% -o %obj%\FileAccess.o              %obj%\FileAccess.asm
%AS% -o %obj%\PakIO.o                   %obj%\PakIO.asm
%AS% -o %obj%\GBCart.o                  %obj%\GBCart.asm
%AS% -o %obj%\International.o           %obj%\International.asm
%AS% -o %obj%\DirectInput.o             %obj%\DirectInput.asm
%AS% -o %obj%\XInputController.o        %obj%\XInputController.asm
ECHO.

set OBJ_LIST=^
 %obj%\XInputController.o^
 %obj%\DirectInput.o^
 %obj%\International.o^
 %obj%\GBCart.o^
 %obj%\PakIO.o^
 %obj%\FileAccess.o^
 %obj%\Interface.o^
 %obj%\NRagePluginV2.o^
 -ldinput8^
 -loleaut32^
 -lole32^
 -luuid^
 -lcomctl32^
 -mwindows^
 -lcomdlg32^
 -lgdi32^
 %obj%\NRagePluginV2.res

ECHO Linking N-Rage objects...
%MinGW%\bin\windres.exe -o %obj%\NRagePluginV2.res -i %src%\NRagePluginV2.rc -O coff
%MinGW%\bin\g++.exe -o %obj%\PJ64_NRage.dll %OBJ_LIST% -shared -shared-libgcc
PAUSE
