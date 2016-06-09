#########################
# Settings
#########################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./Settings

LOCAL_MODULE := Settings

LOCAL_C_INCLUDES := ../     \
               
LOCAL_SRC_FILES := \
    $(SRCDIR)/Settings.cpp        \

LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)

include $(BUILD_STATIC_LIBRARY)