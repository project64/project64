src=./../../3rdParty/zlib
obj=./zlib

mkdir -p $obj/contrib/minizip

FLAGS_x86="\
 -S \
 -fPIC \
 -Wall \
 -pedantic \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=cc
AS=as

echo Compiling zlib sources...
$CC -o $obj/adler32.asm                 $src/adler32.c $C_FLAGS
$CC -o $obj/compress.asm                $src/compress.c $C_FLAGS
$CC -o $obj/crc32.asm                   $src/crc32.c $C_FLAGS
$CC -o $obj/deflate.asm                 $src/deflate.c $C_FLAGS
$CC -o $obj/gzclose.asm                 $src/gzclose.c $C_FLAGS
$CC -o $obj/gzlib.asm                   $src/gzlib.c $C_FLAGS
$CC -o $obj/gzread.asm                  $src/gzread.c $C_FLAGS
$CC -o $obj/gzwrite.asm                 $src/gzwrite.c $C_FLAGS
$CC -o $obj/infback.asm                 $src/infback.c $C_FLAGS
$CC -o $obj/inffast.asm                 $src/inffast.c $C_FLAGS
$CC -o $obj/inflate.asm                 $src/inflate.c $C_FLAGS
$CC -o $obj/inftrees.asm                $src/inftrees.c $C_FLAGS
$CC -o $obj/trees.asm                   $src/trees.c $C_FLAGS
$CC -o $obj/uncompr.asm                 $src/uncompr.c $C_FLAGS
$CC -o $obj/zutil.asm                   $src/zutil.c $C_FLAGS
$CC -o $obj/contrib/minizip/ioapi.asm   $src/contrib/minizip/ioapi.c $C_FLAGS
#$CC -o $obj/contrib/minizip/iowin32.asm $src/contrib/minizip/iowin32.c $C_FLAGS
$CC -o $obj/contrib/minizip/mztools.asm $src/contrib/minizip/mztools.c $C_FLAGS
$CC -o $obj/contrib/minizip/unzip.asm   $src/contrib/minizip/unzip.c $C_FLAGS
$CC -o $obj/contrib/minizip/zip.asm     $src/contrib/minizip/zip.c $C_FLAGS

echo Assembling zlib sources...
$AS -o $obj/adler32.o                   $obj/adler32.asm
$AS -o $obj/compress.o                  $obj/compress.asm
$AS -o $obj/crc32.o                     $obj/crc32.asm
$AS -o $obj/deflate.o                   $obj/deflate.asm
$AS -o $obj/gzclose.o                   $obj/gzclose.asm
$AS -o $obj/gzlib.o                     $obj/gzlib.asm
$AS -o $obj/gzread.o                    $obj/gzread.asm
$AS -o $obj/gzwrite.o                   $obj/gzwrite.asm
$AS -o $obj/infback.o                   $obj/infback.asm
$AS -o $obj/inffast.o                   $obj/inffast.asm
$AS -o $obj/inflate.o                   $obj/inflate.asm
$AS -o $obj/inftrees.o                  $obj/inftrees.asm
$AS -o $obj/trees.o                     $obj/trees.asm
$AS -o $obj/uncompr.o                   $obj/uncompr.asm
$AS -o $obj/zutil.o                     $obj/zutil.asm
$AS -o $obj/contrib/minizip/ioapi.o     $obj/contrib/minizip/ioapi.asm
#$AS -o $obj/contrib/minizip/iowin32.o   $obj/contrib/minizip/iowin32.asm
$AS -o $obj/contrib/minizip/mztools.o   $obj/contrib/minizip/mztools.asm
$AS -o $obj/contrib/minizip/unzip.o     $obj/contrib/minizip/unzip.asm
$AS -o $obj/contrib/minizip/zip.o       $obj/contrib/minizip/zip.asm

OBJ_LIST="\
 $obj/adler32.o \
 $obj/compress.o \
 $obj/crc32.o \
 $obj/deflate.o \
 $obj/gzclose.o \
 $obj/gzlib.o \
 $obj/gzread.o \
 $obj/gzwrite.o \
 $obj/infback.o \
 $obj/inffast.o \
 $obj/inflate.o \
 $obj/inftrees.o \
 $obj/trees.o \
 $obj/uncompr.o \
 $obj/zutil.o \
 $obj/contrib/minizip/ioapi.o \
 $obj/contrib/minizip/mztools.o \
 $obj/contrib/minizip/unzip.o \
 $obj/contrib/minizip/zip.o"

echo Linking static library objects for zlib...
ar rcs $obj/libpj64zip.a $OBJ_LIST
