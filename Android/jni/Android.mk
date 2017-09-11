JNI_LOCAL_PATH := $(call my-dir)

AE_BRIDGE_INCLUDES := $(JNI_LOCAL_PATH)/ae-bridge/
PJ64_SRC := $(JNI_LOCAL_PATH)/

COMMON_CFLAGS :=                    \
    -O3                             \
    -ffast-math                     \
    -fno-strict-aliasing            \
    -fomit-frame-pointer            \
    -frename-registers              \
    -fsingle-precision-constant     \
    -fvisibility=hidden             \
    -DANDROID                       \
    -DNO_ASM                        \

COMMON_CPPFLAGS :=                  \
    -fvisibility-inlines-hidden     \
    -fexceptions                    \

include $(JNI_LOCAL_PATH)/3rdParty/png/png.mk
include $(JNI_LOCAL_PATH)/3rdParty/zlib/zlib.mk
include $(JNI_LOCAL_PATH)/Common/common.mk
include $(JNI_LOCAL_PATH)/PluginInput/PluginInput.mk
include $(JNI_LOCAL_PATH)/PluginRSP/PluginRSP.mk
include $(JNI_LOCAL_PATH)/Project64-audio/Project64-audio.mk
include $(JNI_LOCAL_PATH)/Project64-bridge/Project64-bridge.mk
include $(JNI_LOCAL_PATH)/Project64-core/Project64-core.mk
include $(JNI_LOCAL_PATH)/Project64-video/Project64-video.mk
include $(JNI_LOCAL_PATH)/Settings/Settings.mk