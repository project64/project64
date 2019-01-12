#########################
# project64-core
#########################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./Project64-core

LOCAL_MODULE := Project64-core
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := common   \
                          zlib     \

LOCAL_C_INCLUDES := ../     \
    ../3rdParty/            \

               
LOCAL_SRC_FILES :=                                                     \
    $(SRCDIR)/AppInit.cpp                                              \
    $(SRCDIR)/logging.cpp                                              \
    $(SRCDIR)/Settings.cpp                                             \
    $(SRCDIR)/MemoryExceptionFilter.cpp                                \
    $(SRCDIR)/Multilanguage/LanguageClass.cpp                          \
    $(SRCDIR)/Settings/LoggingSettings.cpp                             \
    $(SRCDIR)/Settings/RecompilerSettings.cpp                          \
    $(SRCDIR)/N64System/Interpreter/InterpreterCPU.cpp                 \
    $(SRCDIR)/N64System/Interpreter/InterpreterOps.cpp                 \
    $(SRCDIR)/N64System/Interpreter/InterpreterOps32.cpp               \
    $(SRCDIR)/N64System/Mips/Audio.cpp                                 \
    $(SRCDIR)/N64System/Mips/Dma.cpp                                   \
    $(SRCDIR)/N64System/Mips/Disk.cpp                                  \
    $(SRCDIR)/N64System/Mips/Eeprom.cpp                                \
    $(SRCDIR)/N64System/Mips/FlashRam.cpp                              \
    $(SRCDIR)/N64System/Mips/GBCart.cpp                                \
    $(SRCDIR)/N64System/Mips/MemoryVirtualMem.cpp                      \
    $(SRCDIR)/N64System/Mips/Mempak.cpp                                \
    $(SRCDIR)/N64System/Mips/OpcodeName.cpp                            \
    $(SRCDIR)/N64System/Mips/PifRam.cpp                                \
    $(SRCDIR)/N64System/Mips/RegisterClass.cpp                         \
    $(SRCDIR)/N64System/Mips/Rumblepak.cpp                             \
    $(SRCDIR)/N64System/Mips/Transferpak.cpp                           \
    $(SRCDIR)/N64System/Mips/Sram.cpp                                  \
    $(SRCDIR)/N64System/Mips/SystemEvents.cpp                          \
    $(SRCDIR)/N64System/Mips/SystemTiming.cpp                          \
    $(SRCDIR)/N64System/Mips/TLBclass.cpp                              \
    $(SRCDIR)/N64System/Recompiler/CodeBlock.cpp                       \
    $(SRCDIR)/N64System/Recompiler/CodeSection.cpp                     \
    $(SRCDIR)/N64System/Recompiler/SectionInfo.cpp                     \
    $(SRCDIR)/N64System/Recompiler/FunctionInfo.cpp                    \
    $(SRCDIR)/N64System/Recompiler/FunctionMapClass.cpp                \
    $(SRCDIR)/N64System/Recompiler/LoopAnalysis.cpp                    \
    $(SRCDIR)/N64System/Recompiler/RecompilerClass.cpp                 \
    $(SRCDIR)/N64System/Recompiler/RecompilerCodeLog.cpp               \
    $(SRCDIR)/N64System/Recompiler/RecompilerMemory.cpp                \
    $(SRCDIR)/N64System/Recompiler/RegBase.cpp                         \
    $(SRCDIR)/N64System/Recompiler/Arm/ArmOps.cpp                      \
    $(SRCDIR)/N64System/Recompiler/Arm/ArmRecompilerOps.cpp            \
    $(SRCDIR)/N64System/Recompiler/Arm/ArmRegInfo.cpp                  \
    $(SRCDIR)/N64System/Recompiler/x86/x86ops.cpp                      \
    $(SRCDIR)/N64System/Recompiler/x86/x86RecompilerOps.cpp            \
    $(SRCDIR)/N64System/Recompiler/x86/x86RegInfo.cpp                  \
    $(SRCDIR)/N64System/CheatClass.cpp                                 \
    $(SRCDIR)/N64System/FramePerSecondClass.cpp                        \
    $(SRCDIR)/N64System/N64Class.cpp                                   \
    $(SRCDIR)/N64System/N64RomClass.cpp                                \
    $(SRCDIR)/N64System/ProfilingClass.cpp                             \
    $(SRCDIR)/N64System/SpeedLimiterClass.cpp                          \
    $(SRCDIR)/N64System/SystemGlobals.cpp                              \
    $(SRCDIR)/N64System/EmulationThread.cpp                            \
    $(SRCDIR)/N64System/N64DiskClass.cpp                               \
    $(SRCDIR)/Plugins/AudioPlugin.cpp                                  \
    $(SRCDIR)/Plugins/GFXplugin.cpp                                    \
    $(SRCDIR)/Plugins/ControllerPlugin.cpp                             \
    $(SRCDIR)/Plugins/RSPPlugin.cpp                                    \
    $(SRCDIR)/Plugins/PluginBase.cpp                                   \
    $(SRCDIR)/Plugins/PluginClass.cpp                                  \
    $(SRCDIR)/RomList/RomList.cpp                                      \
    $(SRCDIR)/Settings/SettingType/SettingsType-Application.cpp        \
    $(SRCDIR)/Settings/SettingType/SettingsType-ApplicationIndex.cpp   \
    $(SRCDIR)/Settings/SettingType/SettingsType-ApplicationPath.cpp    \
    $(SRCDIR)/Settings/SettingType/SettingsType-Cheats.cpp             \
    $(SRCDIR)/Settings/SettingType/SettingsType-Enhancements.cpp       \
    $(SRCDIR)/Settings/SettingType/SettingsType-GameSetting.cpp        \
    $(SRCDIR)/Settings/SettingType/SettingsType-GameSettingIndex.cpp   \
    $(SRCDIR)/Settings/SettingType/SettingsType-RelativePath.cpp       \
    $(SRCDIR)/Settings/SettingType/SettingsType-RDBCpuType.cpp         \
    $(SRCDIR)/Settings/SettingType/SettingsType-RDBOnOff.cpp           \
    $(SRCDIR)/Settings/SettingType/SettingsType-RDBRamSize.cpp         \
    $(SRCDIR)/Settings/SettingType/SettingsType-RDBSaveChip.cpp        \
    $(SRCDIR)/Settings/SettingType/SettingsType-RDBYesNo.cpp           \
    $(SRCDIR)/Settings/SettingType/SettingsType-RomDatabase.cpp        \
    $(SRCDIR)/Settings/SettingType/SettingsType-RomDatabaseIndex.cpp   \
    $(SRCDIR)/Settings/SettingType/SettingsType-RomDatabaseSetting.cpp \
    $(SRCDIR)/Settings/SettingType/SettingsType-SelectedDirectory.cpp  \
    $(SRCDIR)/Settings/SettingType/SettingsType-TempBool.cpp           \
    $(SRCDIR)/Settings/SettingType/SettingsType-TempNumber.cpp         \
    $(SRCDIR)/Settings/SettingType/SettingsType-TempString.cpp         \
    $(SRCDIR)/Settings/DebugSettings.cpp                               \
    $(SRCDIR)/Settings/GameSettings.cpp                                \
    $(SRCDIR)/Settings/N64SystemSettings.cpp                           \

LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    # Use for ARM7a:
    LOCAL_SRC_FILES += $(SRCDIR)/N64System/Recompiler/Arm/asm_functions.S
    LOCAL_CFLAGS += -mfloat-abi=softfp
    LOCAL_CFLAGS += -mfpu=vfp

else ifeq ($(TARGET_ARCH_ABI), armeabi)
    # Use for ARM7a:
    LOCAL_SRC_FILES += $(SRCDIR)/N64System/Recompiler/Arm/asm_functions.S

endif

include $(BUILD_STATIC_LIBRARY)
