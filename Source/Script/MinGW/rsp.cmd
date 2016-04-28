@ECHO OFF
TITLE MinGW Compiler Suite Invocation

set src=%CD%\..\..\RSP
set obj=%CD%\RSP

if not exist %obj% (
mkdir %obj%
)

    set MinGW=C:\MinGW
REM set MinGW=C:\msys64\mingw64\x86_64-w64-mingw32\..

set FLAGS_x86=^
 -Wno-write-strings^
 -I%src%\..^
 -S^
 -masm=intel^
 -march=native^
 -Os

set C_FLAGS=%FLAGS_x86%

cd %MinGW%\bin
set CC=%MinGW%\bin\gcc.exe
set AS=%MinGW%\bin\as.exe

ECHO Compiling RSP plugin sources...
%CC% -o %obj%\Main.asm                  %src%\Main.cpp %C_FLAGS%
%CC% -o %obj%\Cpu.asm                   %src%\Cpu.c %C_FLAGS%
%CC% -o %obj%\memory.asm                %src%\memory.c %C_FLAGS%
%CC% -o %obj%\dma.asm                   %src%\dma.c %C_FLAGS%
%CC% -o %obj%\breakpoint.asm            %src%\breakpoint.c %C_FLAGS%
%CC% -o %obj%\log.asm                   %src%\log.cpp %C_FLAGS%
%CC% -o %obj%\InterpreterCPU.asm        %src%\InterpreterCPU.c %C_FLAGS%
%CC% -o %obj%\RecompilerCPU.asm         %src%\RecompilerCPU.c %C_FLAGS%
%CC% -o %obj%\InterpreterOps.asm        %src%\InterpreterOps.c %C_FLAGS%
%CC% -o %obj%\RecompilerOps.asm         %src%\RecompilerOps.c %C_FLAGS%
%CC% -o %obj%\Profiling.asm             %src%\Profiling.cpp %C_FLAGS%
%CC% -o %obj%\RSPCommand.asm            %src%\RSPCommand.c %C_FLAGS%
%CC% -o %obj%\RSPRegister.asm           %src%\RSPRegister.c %C_FLAGS%
%CC% -o %obj%\RecompilerSections.asm    %src%\RecompilerSections.c %C_FLAGS%
%CC% -o %obj%\RecompilerAnalysis.asm    %src%\RecompilerAnalysis.c %C_FLAGS%
%CC% -o %obj%\X86.asm                   %src%\X86.c %C_FLAGS%
%CC% -o %obj%\Mmx.asm                   %src%\Mmx.c %C_FLAGS%
%CC% -o %obj%\Sse.asm                   %src%\Sse.c %C_FLAGS%

ECHO Assembling RSP sources...
%AS% -o %obj%\Main.o                    %obj%\Main.asm
%AS% -o %obj%\Cpu.o                     %obj%\Cpu.asm
%AS% -o %obj%\memory.o                  %obj%\memory.asm
%AS% -o %obj%\dma.o                     %obj%\dma.asm
%AS% -o %obj%\breakpoint.o              %obj%\breakpoint.asm
%AS% -o %obj%\log.o                     %obj%\log.asm
%AS% -o %obj%\InterpreterCPU.o          %obj%\InterpreterCPU.asm
%AS% -o %obj%\RecompilerCPU.o           %obj%\RecompilerCPU.asm
%AS% -o %obj%\InterpreterOps.o          %obj%\InterpreterOps.asm
%AS% -o %obj%\RecompilerOps.o           %obj%\RecompilerOps.asm
%AS% -o %obj%\Profiling.o               %obj%\Profiling.asm
%AS% -o %obj%\RSPCommand.o              %obj%\RSPCommand.asm
%AS% -o %obj%\RSPRegister.o             %obj%\RSPRegister.asm
%AS% -o %obj%\RecompilerSections.o      %obj%\RecompilerSections.asm
%AS% -o %obj%\RecompilerAnalysis.o      %obj%\RecompilerAnalysis.asm
%AS% -o %obj%\X86.o                     %obj%\X86.asm
%AS% -o %obj%\Mmx.o                     %obj%\Mmx.asm
%AS% -o %obj%\Sse.o                     %obj%\Sse.asm
ECHO.

set OBJ_LIST=^
 -mwindows^
 -L%obj%\..\Common^
 -lcommon^
 -lgdi32^
 %obj%\RSP.res^
 %obj%\Sse.o^
 %obj%\Mmx.o^
 %obj%\X86.o^
 %obj%\RecompilerAnalysis.o^
 %obj%\RecompilerSections.o^
 %obj%\RSPRegister.o^
 %obj%\RSPCommand.o^
 %obj%\Profiling.o^
 %obj%\RecompilerOps.o^
 %obj%\InterpreterOps.o^
 %obj%\RecompilerCPU.o^
 %obj%\InterpreterCPU.o^
 %obj%\log.o^
 %obj%\breakpoint.o^
 %obj%\dma.o^
 %obj%\memory.o^
 %obj%\Cpu.o^
 %obj%\Main.o

ECHO Linking RSP objects...
%MinGW%\bin\windres.exe -o %obj%\RSP.res -i %src%\RSP.rc -O coff
%MinGW%\bin\g++.exe -o %obj%\RSP_1.7.dll %OBJ_LIST% -s
PAUSE
