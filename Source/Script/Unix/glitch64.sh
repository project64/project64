src=./../../Glitch64
obj=./Glitch64

mkdir -p $obj

FLAGS_x86="\
 -S \
 -fPIC \
 -I$src/inc \
 -I$src/.. \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=g++
AS=as

echo Compiling Glitch64 library sources for Glide64...
$CC -o $obj/OGLcombiner.asm             $src/OGLcombiner.cpp $C_FLAGS
$CC -o $obj/OGLgeometry.asm             $src/OGLgeometry.cpp $C_FLAGS
$CC -o $obj/OGLglitchmain.asm           $src/OGLglitchmain.cpp $C_FLAGS
$CC -o $obj/OGLtextures.asm             $src/OGLtextures.cpp $C_FLAGS
#$CC -o $obj/OGLEScombiner.asm           $src/OGLEScombiner.cpp $C_FLAGS
#$CC -o $obj/OGLESgeometry.asm           $src/OGLESgeometry.cpp $C_FLAGS
#$CC -o $obj/OGLESglitchmain.asm         $src/OGLESglitchmain.cpp $C_FLAGS
#$CC -o $obj/OGLEStextures.asm           $src/OGLEStextures.cpp $C_FLAGS

echo Assembling Glitch64 library sources...
$AS -o $obj/OGLcombiner.o               $obj/OGLcombiner.asm
$AS -o $obj/OGLgeometry.o               $obj/OGLgeometry.asm
$AS -o $obj/OGLglitchmain.o             $obj/OGLglitchmain.asm
$AS -o $obj/OGLtextures.o               $obj/OGLtextures.asm
#$AS -o $obj/OGLEScombiner.o             $obj/OGLEScombiner.asm
#$AS -o $obj/OGLESgeometry.o             $obj/OGLESgeometry.asm
#$AS -o $obj/OGLESglitchmain.o           $obj/OGLESglitchmain.asm
#$AS -o $obj/OGLEStextures.o             $obj/OGLEStextures.asm

OBJ_LIST="\
 $obj/OGLtextures.o \
 $obj/OGLglitchmain.o \
 $obj/OGLgeometry.o \
 $obj/OGLcombiner.o"
#$obj/OGLEStextures.o $obj/OGLESglitchmain.o $obj/OGLESgeometry.o $obj/OGLEScombiner.o"

echo Linking static library objects for Glitch64...
ar rcs $obj/libglitch64.a $OBJ_LIST
