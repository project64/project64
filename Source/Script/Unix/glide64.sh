src=./../../Glide64
obj=./Glide64

mkdir -p $obj

FLAGS_x86="\
 -I$src/.. \
 -I$src/../3rdParty \
 -I$src/../Glitch64/inc \
 -S \
 -fPIC \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=g++
AS=as

echo Compiling Glide64 plugin sources...
$CC -o $obj/Main.asm                    $src/Main.cpp $C_FLAGS
$CC -o $obj/FBtoScreen.asm              $src/FBtoScreen.cpp $C_FLAGS
$CC -o $obj/rdp.asm                     $src/rdp.cpp $C_FLAGS
$CC -o $obj/Keys.asm                    $src/Keys.cpp $C_FLAGS
$CC -o $obj/CRC.asm                     $src/CRC.cpp $C_FLAGS
$CC -o $obj/Debugger.asm                $src/Debugger.cpp $C_FLAGS
$CC -o $obj/Util.asm                    $src/Util.cpp $C_FLAGS
$CC -o $obj/TexCache.asm                $src/TexCache.cpp $C_FLAGS
$CC -o $obj/3dmath.asm                  $src/3dmath.cpp $C_FLAGS
$CC -o $obj/Combine.asm                 $src/Combine.cpp $C_FLAGS
$CC -o $obj/DepthBufferRender.asm       $src/DepthBufferRender.cpp $C_FLAGS
$CC -o $obj/Ext_TxFilter.asm            $src/Ext_TxFilter.cpp $C_FLAGS
$CC -o $obj/TexBuffer.asm               $src/TexBuffer.cpp $C_FLAGS
$CC -o $obj/trace.asm                   $src/trace.cpp $C_FLAGS
$CC -o $obj/Settings.asm                $src/Settings.cpp $C_FLAGS
$CC -o $obj/Config.asm                  $src/Config.cpp $C_FLAGS

echo Assembling Glide64 sources...
$AS -o $obj/Main.o                      $obj/Main.asm
$AS -o $obj/FBtoScreen.o                $obj/FBtoScreen.asm
$AS -o $obj/rdp.o                       $obj/rdp.asm
$AS -o $obj/Keys.o                      $obj/Keys.asm
$AS -o $obj/CRC.o                       $obj/CRC.asm
$AS -o $obj/Debugger.o                  $obj/Debugger.asm
$AS -o $obj/Util.o                      $obj/Util.asm
$AS -o $obj/TexCache.o                  $obj/TexCache.asm
$AS -o $obj/3dmath.o                    $obj/3dmath.asm
$AS -o $obj/Combine.o                   $obj/Combine.asm
$AS -o $obj/DepthBufferRender.o         $obj/DepthBufferRender.asm
$AS -o $obj/Ext_TxFilter.o              $obj/Ext_TxFilter.asm
$AS -o $obj/TexBuffer.o                 $obj/TexBuffer.asm
$AS -o $obj/trace.o                     $obj/trace.asm
$AS -o $obj/Settings.o                  $obj/Settings.asm
$AS -o $obj/Config.o                    $obj/Config.asm

OBJ_LIST="\
 $obj/Config.o \
 $obj/Settings.o \
 $obj/trace.o \
 $obj/TexBuffer.o \
 $obj/Ext_TxFilter.o \
 $obj/DepthBufferRender.o \
 $obj/Combine.o \
 $obj/3dmath.o \
 $obj/TexCache.o \
 $obj/Util.o \
 $obj/Debugger.o \
 $obj/CRC.o \
 $obj/Keys.o \
 $obj/rdp.o \
 $obj/FBtoScreen.o \
 $obj/Main.o \
 -L$obj/../Glitch64 \
 -L$obj/../GlideHQ \
 -L$obj/../Common \
 -L$obj/../Settings"

echo Linking PJGlide64 objects...
g++ -o $obj/PJ64Glide64.so $OBJ_LIST -lglitch64 -lglidehq -lcommon -lsettings -shared -shared-libgcc
