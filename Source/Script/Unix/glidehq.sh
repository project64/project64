src=./../../GlideHQ
obj=./GlideHQ

mkdir -p $obj

FLAGS_x86="\
 -S \
 -fPIC \
 -I$src/inc \
 -I$src/.. \
 -I$src/../3rdParty \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=cc
AS=as

echo Compiling GlideHQ library sources for Glide64...
$CC -o $obj/TextureFilters.asm          $src/TextureFilters.cpp $C_FLAGS
$CC -o $obj/TextureFilters_2xsai.asm    $src/TextureFilters_2xsai.cpp $C_FLAGS
$CC -o $obj/TextureFilters_hq2x.asm     $src/TextureFilters_hq2x.cpp $C_FLAGS
$CC -o $obj/TextureFilters_hq4x.asm     $src/TextureFilters_hq4x.cpp $C_FLAGS
$CC -o $obj/TxCache.asm                 $src/TxCache.cpp $C_FLAGS
$CC -o $obj/TxDbg.asm                   $src/TxDbg.cpp $C_FLAGS
$CC -o $obj/TxFilter.asm                $src/TxFilter.cpp $C_FLAGS
$CC -o $obj/TxFilterExport.asm          $src/TxFilterExport.cpp $C_FLAGS
$CC -o $obj/TxHiResCache.asm            $src/TxHiResCache.cpp $C_FLAGS
$CC -o $obj/TxImage.asm                 $src/TxImage.cpp $C_FLAGS
$CC -o $obj/TxQuantize.asm              $src/TxQuantize.cpp $C_FLAGS
$CC -o $obj/TxReSample.asm              $src/TxReSample.cpp $C_FLAGS
$CC -o $obj/TxTexCache.asm              $src/TxTexCache.cpp $C_FLAGS
$CC -o $obj/TxUtil.asm                  $src/TxUtil.cpp $C_FLAGS
$CC -o $obj/dxtn.asm                    $src/tc-1.1+/dxtn.c $C_FLAGS
$CC -o $obj/fxt1.asm                    $src/tc-1.1+/fxt1.c $C_FLAGS
$CC -o $obj/texstore.asm                $src/tc-1.1+/texstore.c $C_FLAGS
$CC -o $obj/wrapper.asm                 $src/tc-1.1+/wrapper.c $C_FLAGS

echo Assembling GlideHQ library sources...
$AS -o $obj/TextureFilters.o            $obj/TextureFilters.asm
$AS -o $obj/TextureFilters_2xsai.o      $obj/TextureFilters_2xsai.asm
$AS -o $obj/TextureFilters_hq2x.o       $obj/TextureFilters_hq2x.asm
$AS -o $obj/TextureFilters_hq4x.o       $obj/TextureFilters_hq4x.asm
$AS -o $obj/TxCache.o                   $obj/TxCache.asm
$AS -o $obj/TxDbg.o                     $obj/TxDbg.asm
$AS -o $obj/TxFilter.o                  $obj/TxFilter.asm
$AS -o $obj/TxFilterExport.o            $obj/TxFilterExport.asm
$AS -o $obj/TxHiResCache.o              $obj/TxHiResCache.asm
$AS -o $obj/TxImage.o                   $obj/TxImage.asm
$AS -o $obj/TxQuantize.o                $obj/TxQuantize.asm
$AS -o $obj/TxReSample.o                $obj/TxReSample.asm
$AS -o $obj/TxTexCache.o                $obj/TxTexCache.asm
$AS -o $obj/TxUtil.o                    $obj/TxUtil.asm
$AS -o $obj/dxtn.o                      $obj/dxtn.asm
$AS -o $obj/fxt1.o                      $obj/fxt1.asm
$AS -o $obj/texstore.o                  $obj/texstore.asm
$AS -o $obj/wrapper.o                   $obj/wrapper.asm

OBJ_LIST="\
 $obj/wrapper.o \
 $obj/texstore.o \
 $obj/fxt1.o \
 $obj/dxtn.o \
 $obj/TxUtil.o \
 $obj/TxTexCache.o \
 $obj/TxReSample.o \
 $obj/TxQuantize.o \
 $obj/TxImage.o \
 $obj/TxHiResCache.o \
 $obj/TxFilterExport.o \
 $obj/TxFilter.o \
 $obj/TxDbg.o \
 $obj/TxCache.o \
 $obj/TextureFilters_hq4x.o \
 $obj/TextureFilters_hq2x.o \
 $obj/TextureFilters_2xsai.o \
 $obj/TextureFilters.o"

echo Linking static library objects for GlideHQ...
ar rcs $obj/libglidehq.a $OBJ_LIST
