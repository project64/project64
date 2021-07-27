######################
# Project64-input-android
######################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./PluginInput

LOCAL_MODULE := Project64-input-android
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=

LOCAL_SRC_FILES :=             \
    $(SRCDIR)/Main.cpp        \

LOCAL_CFLAGS := $(COMMON_CFLAGS)

LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS) \
     -D__STDC_LIMIT_MACROS           \

LOCAL_LDLIBS :=         \
    -llog               \

include $(BUILD_SHARED_LIBRARY)
