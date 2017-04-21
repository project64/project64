src=./../../RSP
obj=./RSP

mkdir -p $obj

FLAGS_x86="\
 -S \
 -I$src/.. \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=cc
AS=as

echo Compiling RSP plugin sources...
$CC -o $obj/Main.asm                    $src/Main.cpp $C_FLAGS
$CC -o $obj/Cpu.asm                     $src/Cpu.c $C_FLAGS
$CC -o $obj/memory.asm                  $src/memory.c $C_FLAGS
$CC -o $obj/dma.asm                     $src/dma.c $C_FLAGS
$CC -o $obj/breakpoint.asm              $src/breakpoint.c $C_FLAGS
$CC -o $obj/log.asm                     $src/log.cpp $C_FLAGS
$CC -o $obj/InterpreterCPU.asm         "$src/Interpreter CPU.c" $C_FLAGS
$CC -o $obj/RecompilerCPU.asm          "$src/Recompiler CPU.c" $C_FLAGS
$CC -o $obj/InterpreterOps.asm         "$src/Interpreter Ops.c" $C_FLAGS
$CC -o $obj/RecompilerOps.asm          "$src/Recompiler Ops.c" $C_FLAGS
$CC -o $obj/Profiling.asm               $src/Profiling.cpp $C_FLAGS
$CC -o $obj/RSPCommand.asm             "$src/RSP Command.c" $C_FLAGS
$CC -o $obj/RSPRegister.asm            "$src/RSP Register.c" $C_FLAGS
$CC -o $obj/RecompilerSections.asm     "$src/Recompiler Sections.c" $C_FLAGS
$CC -o $obj/RecompilerAnalysis.asm     "$src/Recompiler Analysis.c" $C_FLAGS
$CC -o $obj/X86.asm                     $src/X86.c $C_FLAGS
$CC -o $obj/Mmx.asm                     $src/Mmx.c $C_FLAGS
$CC -o $obj/Sse.asm                     $src/Sse.c $C_FLAGS

echo Assembling RSP sources...
$AS -o $obj/Main.o                     $obj/Main.asm
$AS -o $obj/Cpu.o                      $obj/Cpu.asm
$AS -o $obj/memory.o                   $obj/memory.asm
$AS -o $obj/dma.o                      $obj/dma.asm
$AS -o $obj/breakpoint.o               $obj/breakpoint.asm
$AS -o $obj/log.o                      $obj/log.asm
$AS -o $obj/InterpreterCPU.o           $obj/InterpreterCPU.asm
$AS -o $obj/RecompilerCPU.o            $obj/RecompilerCPU.asm
$AS -o $obj/InterpreterOps.o           $obj/InterpreterOps.asm
$AS -o $obj/RecompilerOps.o            $obj/RecompilerOps.asm
$AS -o $obj/Profiling.o                $obj/Profiling.asm
$AS -o $obj/RSPCommand.o               $obj/RSPCommand.asm
$AS -o $obj/RSPRegister.o              $obj/RSPRegister.asm
$AS -o $obj/RecompilerSections.o       $obj/RecompilerSections.asm
$AS -o $obj/RecompilerAnalysis.o       $obj/RecompilerAnalysis.asm
$AS -o $obj/X86.o                      $obj/X86.asm
$AS -o $obj/Mmx.o                      $obj/Mmx.asm
$AS -o $obj/Sse.o                      $obj/Sse.asm

set OBJ_LIST="\
 -L$obj/../Common \
 -lcommon \
 $obj/Sse.o \
 $obj/Mmx.o \
 $obj/X86.o \
 $obj/RecompilerAnalysis.o \
 $obj/RecompilerSections.o \
 $obj/RSPRegister.o \
 $obj/RSPCommand.o \
 $obj/Profiling.o \
 $obj/RecompilerOps.o \
 $obj/InterpreterOps.o \
 $obj/RecompilerCPU.o \
 $obj/InterpreterCPU.o \
 $obj/log.o \
 $obj/breakpoint.o \
 $obj/dma.o \
 $obj/memory.o \
 $obj/Cpu.o \
 $obj/Main.o"

echo Linking RSP objects...
g++ -o $obj/RSP_1_7 $OBJ_LIST -s
