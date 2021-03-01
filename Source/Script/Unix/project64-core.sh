src=./../../Project64-core
obj=./Project64-core

mkdir -p $obj/Multilanguage
mkdir -p $obj/N64System/Mips
mkdir -p $obj/N64System/interp
mkdir -p $obj/N64System/dynarec
mkdir -p $obj/N64System/enhan
mkdir -p $obj/Plugins
mkdir -p $obj/Settings/type

FLAGS_x86="\
 -I$src \
 -I$src/.. \
 -I$src/../3rdParty \
 -fpermissive \
 -S \
 -masm=intel \
 -march=native \
 -Os"

C_FLAGS=$FLAGS_x86

CC=g++
AS=as

echo Compiling core sources for Project64...
#$CC -o $obj/7zip.asm                    $src/3rdParty/7zip.cpp $C_FLAGS # precompiled?
$CC -o $obj/AppInit.asm                 $src/AppInit.cpp $C_FLAGS
$CC -o $obj/logging.asm                 $src/Logging.cpp $C_FLAGS
$CC -o $obj/MemoryExceptionFilter.asm   $src/MemoryExceptionFilter.cpp $C_FLAGS
$CC -o $obj/Multilanguage/LangClass.asm $src/Multilanguage/LanguageClass.cpp $C_FLAGS
$CC -o $obj/N64System/enhan/Enhance.asm $src/N64System/Enhancement/Enhancement.cpp $C_FLAGS
$CC -o $obj/N64System/enhan/List.asm    $src/N64System/Enhancement/EnhancementList.cpp $C_FLAGS
$CC -o $obj/N64System/enhan/File.asm    $src/N64System/Enhancement/EnhancementFile.cpp $C_FLAGS
$CC -o $obj/N64System/enhan/Enhans.asm  $src/N64System/Enhancement/Enhancements.cpp $C_FLAGS
$CC -o $obj/N64System/EmuThread.asm     $src/N64System/EmulationThread.cpp $C_FLAGS
$CC -o $obj/N64System/FPSClass.asm      $src/N64System/FramePerSecondClass.cpp $C_FLAGS
$CC -o $obj/N64System/interp/CPU.asm    $src/N64System/Interpreter/InterpreterCPU.cpp $C_FLAGS
$CC -o $obj/N64System/interp/Ops.asm    $src/N64System/Interpreter/InterpreterOps.cpp $C_FLAGS
$CC -o $obj/N64System/interp/Ops32.asm  $src/N64System/Interpreter/InterpreterOps32.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Audio.asm    $src/N64System/Mips/Audio.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Disk.asm     $src/N64System/Mips/Disk.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Dma.asm      $src/N64System/Mips/Dma.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Eeprom.asm   $src/N64System/Mips/Eeprom.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/FlashRam.asm $src/N64System/Mips/FlashRam.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/GBCart.asm   $src/N64System/Mips/GBCart.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Memory.asm   $src/N64System/Mips/MemoryVirtualMem.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Mempak.asm   $src/N64System/Mips/Mempak.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/OpName.asm   $src/N64System/Mips/OpcodeName.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/PifRam.asm   $src/N64System/Mips/PifRam.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/RegClass.asm $src/N64System/Mips/RegisterClass.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Rumble.asm   $src/N64System/Mips/Rumblepak.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Sram.asm     $src/N64System/Mips/Sram.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/SyEvents.asm $src/N64System/Mips/SystemEvents.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/SyTiming.asm $src/N64System/Mips/SystemTiming.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/TLBclass.asm $src/N64System/Mips/TLBclass.cpp $C_FLAGS
$CC -o $obj/N64System/Mips/Transfer.asm $src/N64System/Mips/Transferpak.cpp $C_FLAGS
$CC -o $obj/N64System/N64Class.asm      $src/N64System/N64Class.cpp $C_FLAGS
$CC -o $obj/N64System/N64DiskClass.asm  $src/N64System/N64DiskClass.cpp $C_FLAGS
$CC -o $obj/N64System/N64RomClass.asm   $src/N64System/N64RomClass.cpp $C_FLAGS
$CC -o $obj/N64System/ProfileClass.asm  $src/N64System/ProfilingClass.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Block.asm $src/N64System/Recompiler/CodeBlock.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/CSect.asm $src/N64System/Recompiler/CodeSection.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/FnNfo.asm $src/N64System/Recompiler/FunctionInfo.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/FnMap.asm $src/N64System/Recompiler/FunctionMapClass.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Loop.asm  $src/N64System/Recompiler/LoopAnalysis.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Class.asm $src/N64System/Recompiler/RecompilerClass.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Mem.asm   $src/N64System/Recompiler/RecompilerMemory.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Ops.asm   $src/N64System/Recompiler/x86/x86RecompilerOps.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Reg.asm   $src/N64System/Recompiler/x86/x86RegInfo.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/Sect.asm  $src/N64System/Recompiler/SectionInfo.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/log.asm   $src/N64System/Recompiler/RecompilerCodeLog.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/RegB.asm  $src/N64System/Recompiler/RegBase.cpp $C_FLAGS
$CC -o $obj/N64System/dynarec/x86op.asm $src/N64System/Recompiler/x86/x86ops.cpp $C_FLAGS
$CC -o $obj/N64System/SpeedLimiter.asm  $src/N64System/SpeedLimiterClass.cpp $C_FLAGS
$CC -o $obj/N64System/SystemGlobals.asm $src/N64System/SystemGlobals.cpp $C_FLAGS
$CC -o $obj/Plugins/Audio.asm           $src/Plugins/AudioPlugin.cpp $C_FLAGS
$CC -o $obj/Plugins/Controller.asm      $src/Plugins/ControllerPlugin.cpp $C_FLAGS
$CC -o $obj/Plugins/Graphics.asm        $src/Plugins/GFXPlugin.cpp $C_FLAGS
$CC -o $obj/Plugins/PluginBase.asm      $src/Plugins/PluginBase.cpp $C_FLAGS
$CC -o $obj/Plugins/PluginClass.asm     $src/Plugins/PluginClass.cpp $C_FLAGS
$CC -o $obj/Plugins/RSP.asm             $src/Plugins/RSPPlugin.cpp $C_FLAGS
$CC -o $obj/Settings/RomList.asm        $src/RomList/RomList.cpp $C_FLAGS
$CC -o $obj/Settings/Debug.asm          $src/Settings/DebugSettings.cpp $C_FLAGS
$CC -o $obj/Settings/Game.asm           $src/Settings/GameSettings.cpp $C_FLAGS
$CC -o $obj/Settings/Logging.asm        $src/Settings/LoggingSettings.cpp $C_FLAGS
$CC -o $obj/Settings/N64System.asm      $src/Settings/N64SystemSettings.cpp $C_FLAGS
$CC -o $obj/Settings/Recompiler.asm     $src/Settings/RecompilerSettings.cpp $C_FLAGS
$CC -o $obj/Settings.asm                $src/Settings.cpp $C_FLAGS
$CC -o $obj/Settings/type/App.asm       $src/Settings/SettingType/SettingsType-Application.cpp $C_FLAGS
$CC -o $obj/Settings/type/AppIndex.asm  $src/Settings/SettingType/SettingsType-ApplicationIndex.cpp $C_FLAGS
$CC -o $obj/Settings/type/AppPath.asm   $src/Settings/SettingType/SettingsType-ApplicationPath.cpp $C_FLAGS
$CC -o $obj/Settings/type/GSetting.asm  $src/Settings/SettingType/SettingsType-GameSetting.cpp $C_FLAGS
$CC -o $obj/Settings/type/GSettingX.asm $src/Settings/SettingType/SettingsType-GameSettingIndex.cpp $C_FLAGS
$CC -o $obj/Settings/type/RDBCpu.asm    $src/Settings/SettingType/SettingsType-RDBCpuType.cpp $C_FLAGS
$CC -o $obj/Settings/type/RDBOnOff.asm  $src/Settings/SettingType/SettingsType-RDBOnOff.cpp $C_FLAGS
$CC -o $obj/Settings/type/RDBRamSz.asm  $src/Settings/SettingType/SettingsType-RDBRamSize.cpp $C_FLAGS
$CC -o $obj/Settings/type/RDBSaves.asm  $src/Settings/SettingType/SettingsType-RDBSaveChip.cpp $C_FLAGS
$CC -o $obj/Settings/type/RDBYesNo.asm  $src/Settings/SettingType/SettingsType-RDBYesNo.cpp $C_FLAGS
$CC -o $obj/Settings/type/RelPath.asm   $src/Settings/SettingType/SettingsType-RelativePath.cpp $C_FLAGS
$CC -o $obj/Settings/type/RomDb.asm     $src/Settings/SettingType/SettingsType-RomDatabase.cpp $C_FLAGS
$CC -o $obj/Settings/type/RomDbInx.asm  $src/Settings/SettingType/SettingsType-RomDatabaseIndex.cpp $C_FLAGS
$CC -o $obj/Settings/type/RomDbSet.asm  $src/Settings/SettingType/SettingsType-RomDatabaseSetting.cpp $C_FLAGS
$CC -o $obj/Settings/type/SelectDir.asm $src/Settings/SettingType/SettingsType-SelectedDirectory.cpp $C_FLAGS
$CC -o $obj/Settings/type/TmpBool.asm   $src/Settings/SettingType/SettingsType-TempBool.cpp $C_FLAGS
$CC -o $obj/Settings/type/TmpNumber.asm $src/Settings/SettingType/SettingsType-TempNumber.cpp $C_FLAGS
$CC -o $obj/Settings/type/TmpString.asm $src/Settings/SettingType/SettingsType-TempString.cpp $C_FLAGS
#$CC -o $obj/stdafx.asm                  $src/stdafx.cpp $C_FLAGS

echo Assembling Project64 core sources...
#$AS -o $obj/7zip.o                      $obj/7zip.asm
$AS -o $obj/AppInit.o                   $obj/AppInit.asm
$AS -o $obj/logging.o                   $obj/logging.asm
$AS -o $obj/MemoryExceptionFilter.o     $obj/MemoryExceptionFilter.asm
$AS -o $obj/Multilanguage/LangClass.o   $obj/Multilanguage/LangClass.asm
$AS -o $obj/N64System/enhan/Enhance.o   $obj/N64System/enhan/Enhance.asm
$AS -o $obj/N64System/enhan/List.o      $obj/N64System/enhan/List.asm
$AS -o $obj/N64System/enhan/File.o      $obj/N64System/enhan/File.asm
$AS -o $obj/N64System/enhan/Enhans.o    $obj/N64System/enhan/Enhans.asm
$AS -o $obj/N64System/EmuThread.o       $obj/N64System/EmuThread.asm
$AS -o $obj/N64System/FPSClass.o        $obj/N64System/FPSClass.asm
$AS -o $obj/N64System/interp/CPU.o      $obj/N64System/interp/CPU.asm
$AS -o $obj/N64System/interp/Ops.o      $obj/N64System/interp/Ops.asm
$AS -o $obj/N64System/interp/Ops32.o    $obj/N64System/interp/Ops32.asm
$AS -o $obj/N64System/Mips/Audio.o      $obj/N64System/Mips/Audio.asm
$AS -o $obj/N64System/Mips/Disk.o       $obj/N64System/Mips/Disk.asm
$AS -o $obj/N64System/Mips/Dma.o        $obj/N64System/Mips/Dma.asm
$AS -o $obj/N64System/Mips/Eeprom.o     $obj/N64System/Mips/Eeprom.asm
$AS -o $obj/N64System/Mips/FlashRam.o   $obj/N64System/Mips/FlashRam.asm
$AS -o $obj/N64System/Mips/GBCart.o     $obj/N64System/Mips/GBCart.asm
$AS -o $obj/N64System/Mips/Memory.o     $obj/N64System/Mips/Memory.asm
$AS -o $obj/N64System/Mips/Mempak.o     $obj/N64System/Mips/Mempak.asm
$AS -o $obj/N64System/Mips/OpName.o     $obj/N64System/Mips/OpName.asm
$AS -o $obj/N64System/Mips/PifRam.o     $obj/N64System/Mips/PifRam.asm
$AS -o $obj/N64System/Mips/RegClass.o   $obj/N64System/Mips/RegClass.asm
$AS -o $obj/N64System/Mips/Rumble.o     $obj/N64System/Mips/Rumble.asm
$AS -o $obj/N64System/Mips/Sram.o       $obj/N64System/Mips/Sram.asm
$AS -o $obj/N64System/Mips/SyEvents.o   $obj/N64System/Mips/SyEvents.asm
$AS -o $obj/N64System/Mips/SyTiming.o   $obj/N64System/Mips/SyTiming.asm
$AS -o $obj/N64System/Mips/TLBclass.o   $obj/N64System/Mips/TLBclass.asm
$AS -o $obj/N64System/Mips/Transfer.o   $obj/N64System/Mips/Transfer.asm
$AS -o $obj/N64System/N64Class.o        $obj/N64System/N64Class.asm
$AS -o $obj/N64System/N64DiskClass.o    $obj/N64System/N64DiskClass.asm
$AS -o $obj/N64System/N64RomClass.o     $obj/N64System/N64RomClass.asm
$AS -o $obj/N64System/ProfileClass.o    $obj/N64System/ProfileClass.asm
$AS -o $obj/N64System/dynarec/Block.o   $obj/N64System/dynarec/Block.asm
$AS -o $obj/N64System/dynarec/CSect.o   $obj/N64System/dynarec/CSect.asm
$AS -o $obj/N64System/dynarec/FnNfo.o   $obj/N64System/dynarec/FnNfo.asm
$AS -o $obj/N64System/dynarec/FnMap.o   $obj/N64System/dynarec/FnMap.asm
$AS -o $obj/N64System/dynarec/Loop.o    $obj/N64System/dynarec/Loop.asm
$AS -o $obj/N64System/dynarec/Class.o   $obj/N64System/dynarec/Class.asm
$AS -o $obj/N64System/dynarec/Mem.o     $obj/N64System/dynarec/Mem.asm
$AS -o $obj/N64System/dynarec/Ops.o     $obj/N64System/dynarec/Ops.asm
$AS -o $obj/N64System/dynarec/Reg.o     $obj/N64System/dynarec/Reg.asm
$AS -o $obj/N64System/dynarec/Sect.o    $obj/N64System/dynarec/Sect.asm
$AS -o $obj/N64System/dynarec/log.o     $obj/N64System/dynarec/log.asm
$AS -o $obj/N64System/dynarec/RegB.o    $obj/N64System/dynarec/RegB.asm
$AS -o $obj/N64System/dynarec/x86op.o   $obj/N64System/dynarec/x86op.asm
$AS -o $obj/N64System/SpeedLimiter.o    $obj/N64System/SpeedLimiter.asm
$AS -o $obj/N64System/SystemGlobals.o   $obj/N64System/SystemGlobals.asm
$AS -o $obj/Plugins/Audio.o             $obj/Plugins/Audio.asm
$AS -o $obj/Plugins/Controller.o        $obj/Plugins/Controller.asm
$AS -o $obj/Plugins/Graphics.o          $obj/Plugins/Graphics.asm
$AS -o $obj/Plugins/PluginBase.o        $obj/Plugins/PluginBase.asm
$AS -o $obj/Plugins/PluginClass.o       $obj/Plugins/PluginClass.asm
$AS -o $obj/Plugins/RSP.o               $obj/Plugins/RSP.asm
$AS -o $obj/Settings/RomList.o          $obj/Settings/RomList.asm
$AS -o $obj/Settings/Debug.o            $obj/Settings/Debug.asm
$AS -o $obj/Settings/Game.o             $obj/Settings/Game.asm
$AS -o $obj/Settings/Logging.o          $obj/Settings/Logging.asm
$AS -o $obj/Settings/N64System.o        $obj/Settings/N64System.asm
$AS -o $obj/Settings/Recompiler.o       $obj/Settings/Recompiler.asm
$AS -o $obj/Settings.o                  $obj/Settings.asm
$AS -o $obj/Settings/type/App.o         $obj/Settings/type/App.asm
$AS -o $obj/Settings/type/AppIndex.o    $obj/Settings/type/AppIndex.asm
$AS -o $obj/Settings/type/AppPath.o     $obj/Settings/type/AppPath.asm
$AS -o $obj/Settings/type/GSetting.o    $obj/Settings/type/GSetting.asm
$AS -o $obj/Settings/type/GSettingX.o   $obj/Settings/type/GSettingX.asm
$AS -o $obj/Settings/type/RDBCpu.o      $obj/Settings/type/RDBCpu.asm
$AS -o $obj/Settings/type/RDBOnOff.o    $obj/Settings/type/RDBOnOff.asm
$AS -o $obj/Settings/type/RDBRamSz.o    $obj/Settings/type/RDBRamSz.asm
$AS -o $obj/Settings/type/RDBSaves.o    $obj/Settings/type/RDBSaves.asm
$AS -o $obj/Settings/type/RDBYesNo.o    $obj/Settings/type/RDBYesNo.asm
$AS -o $obj/Settings/type/RelPath.o     $obj/Settings/type/RelPath.asm
$AS -o $obj/Settings/type/RomDb.o       $obj/Settings/type/RomDb.asm
$AS -o $obj/Settings/type/RomDbInx.o    $obj/Settings/type/RomDbInx.asm
$AS -o $obj/Settings/type/RomDbSet.o    $obj/Settings/type/RomDbSet.asm
$AS -o $obj/Settings/type/SelectDir.o   $obj/Settings/type/SelectDir.asm
$AS -o $obj/Settings/type/TmpBool.o     $obj/Settings/type/TmpBool.asm
$AS -o $obj/Settings/type/TmpNumber.o   $obj/Settings/type/TmpNumber.asm
$AS -o $obj/Settings/type/TmpString.o   $obj/Settings/type/TmpString.asm

# $obj/7zip.o \
OBJ_LIST="\
$obj/AppInit.o \
$obj/logging.o \
$obj/MemoryExceptionFilter.o \
$obj/Multilanguage/LangClass.o \
$obj/N64System/enhan/Enhance.o \
$obj/N64System/enhan/List.o \
$obj/N64System/enhan/File.o \
$obj/N64System/enhan/Enhans.o \
$obj/N64System/EmuThread.o \
$obj/N64System/FPSClass.o \
$obj/N64System/interp/CPU.o \
$obj/N64System/interp/Ops.o \
$obj/N64System/interp/Ops32.o \
$obj/N64System/Mips/Audio.o \
$obj/N64System/Mips/Disk.o \
$obj/N64System/Mips/Dma.o \
$obj/N64System/Mips/Eeprom.o \
$obj/N64System/Mips/FlashRam.o \
$obj/N64System/Mips/GBCart.o \
$obj/N64System/Mips/Memory.o \
$obj/N64System/Mips/Mempak.o \
$obj/N64System/Mips/OpName.o \
$obj/N64System/Mips/PifRam.o \
$obj/N64System/Mips/RegClass.o \
$obj/N64System/Mips/Rumble.o \
$obj/N64System/Mips/Sram.o \
$obj/N64System/Mips/SyEvents.o \
$obj/N64System/Mips/SyTiming.o \
$obj/N64System/Mips/TLBclass.o \
$obj/N64System/Mips/Transfer.o \
$obj/N64System/N64Class.o \
$obj/N64System/N64DiskClass.o \
$obj/N64System/N64RomClass.o \
$obj/N64System/ProfileClass.o \
$obj/N64System/dynarec/Block.o \
$obj/N64System/dynarec/CSect.o \
$obj/N64System/dynarec/FnNfo.o \
$obj/N64System/dynarec/FnMap.o \
$obj/N64System/dynarec/Loop.o \
$obj/N64System/dynarec/Class.o \
$obj/N64System/dynarec/Mem.o \
$obj/N64System/dynarec/Ops.o \
$obj/N64System/dynarec/Reg.o \
$obj/N64System/dynarec/Sect.o \
$obj/N64System/dynarec/log.o \
$obj/N64System/dynarec/RegB.o \
$obj/N64System/dynarec/x86op.o \
$obj/N64System/SpeedLimiter.o \
$obj/N64System/SystemGlobals.o \
$obj/Plugins/Audio.o \
$obj/Plugins/Controller.o \
$obj/Plugins/Graphics.o \
$obj/Plugins/PluginBase.o \
$obj/Plugins/PluginClass.o \
$obj/Plugins/RSP.o \
$obj/Settings/RomList.o \
$obj/Settings/Debug.o \
$obj/Settings/Game.o \
$obj/Settings/Logging.o \
$obj/Settings/N64System.o \
$obj/Settings/Recompiler.o \
$obj/Settings.o \
$obj/Settings/type/App.o \
$obj/Settings/type/AppIndex.o \
$obj/Settings/type/AppPath.o \
$obj/Settings/type/GSetting.o \
$obj/Settings/type/GSettingX.o \
$obj/Settings/type/RDBCpu.o \
$obj/Settings/type/RDBOnOff.o \
$obj/Settings/type/RDBRamSz.o \
$obj/Settings/type/RDBSaves.o \
$obj/Settings/type/RDBYesNo.o \
$obj/Settings/type/RelPath.o \
$obj/Settings/type/RomDb.o \
$obj/Settings/type/RomDbInx.o \
$obj/Settings/type/RomDbSet.o \
$obj/Settings/type/SelectDir.o \
$obj/Settings/type/TmpBool.o \
$obj/Settings/type/TmpNumber.o \
$obj/Settings/type/TmpString.o"

echo Linking static library objects for Project64-core...
ar rcs $obj/libproject64-core.a $OBJ_LIST
