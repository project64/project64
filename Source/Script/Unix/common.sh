src=./../../Common
obj=./Common

mkdir -p $obj

FLAGS_x86="\
 -S \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=g++
AS=as

echo Compiling common library sources for Project64...
$CC -o $obj/CriticalSection.asm         $src/CriticalSection.cpp $C_FLAGS
$CC -o $obj/FileClass.asm               $src/FileClass.cpp $C_FLAGS
$CC -o $obj/IniFileClass.asm            $src/IniFileClass.cpp $C_FLAGS
$CC -o $obj/LogClass.asm                $src/LogClass.cpp $C_FLAGS
$CC -o $obj/md5.asm                     $src/md5.cpp $C_FLAGS
$CC -o $obj/MemTest.asm                 $src/MemTest.cpp $C_FLAGS
$CC -o $obj/path.asm                    $src/path.cpp $C_FLAGS
$CC -o $obj/stdstring.asm               $src/StdString.cpp $C_FLAGS
$CC -o $obj/SyncEvent.asm               $src/SyncEvent.cpp $C_FLAGS
$CC -o $obj/Trace.asm                   $src/Trace.cpp $C_FLAGS
$CC -o $obj/Util.asm                    $src/Util.cpp $C_FLAGS

echo Assembling common library sources...
$AS -o $obj/CriticalSection.o           $obj/CriticalSection.asm
$AS -o $obj/FileClass.o                 $obj/FileClass.asm
$AS -o $obj/IniFileClass.o              $obj/IniFileClass.asm
$AS -o $obj/LogClass.o                  $obj/LogClass.asm
$AS -o $obj/md5.o                       $obj/md5.asm
$AS -o $obj/MemTest.o                   $obj/MemTest.asm
$AS -o $obj/path.o                      $obj/path.asm
$AS -o $obj/stdstring.o                 $obj/stdstring.asm
$AS -o $obj/SyncEvent.o                 $obj/SyncEvent.asm
$AS -o $obj/Trace.o                     $obj/Trace.asm
$AS -o $obj/Util.o                      $obj/Util.asm

OBJ_LIST="\
 $obj/Util.o \
 $obj/Trace.o \
 $obj/SyncEvent.o \
 $obj/stdstring.o \
 $obj/path.o \
 $obj/MemTest.o \
 $obj/md5.o \
 $obj/LogClass.o \
 $obj/IniFileClass.o \
 $obj/FileClass.o \
 $obj/CriticalSection.o"

echo Linking static library objects for Common...
ar rcs $obj/libcommon.a $OBJ_LIST
