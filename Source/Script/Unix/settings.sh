src=./../../Settings
obj=./Settings

mkdir -p $obj

FLAGS_x86="\
 -S \
 -fPIC \
 -I$src/.. \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=g++
AS=as

echo Compiling settings library sources for Project64...
$CC -o $obj/Settings.asm                $src/Settings.cpp $C_FLAGS

echo Assembling settings library sources...
$AS -o $obj/Settings.o                  $obj/Settings.asm

OBJ_LIST="\
 $obj/Settings.o"

echo Linking static library objects for Settings...
ar rcs $obj/libsettings.a $OBJ_LIST
