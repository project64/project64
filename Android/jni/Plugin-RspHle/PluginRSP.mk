######################
# Project64-rsp-hle
######################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./PluginRSP

LOCAL_MODULE := Project64-rsp-hle
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=

LOCAL_SRC_FILES :=             \
    $(SRCDIR)/alist.cpp        \
    $(SRCDIR)/alist_audio.cpp  \
    $(SRCDIR)/alist_naudio.cpp \
    $(SRCDIR)/alist_nead.cpp   \
    $(SRCDIR)/audio.cpp        \
    $(SRCDIR)/cicx105.cpp      \
    $(SRCDIR)/hle.cpp          \
    $(SRCDIR)/jpeg.cpp         \
    $(SRCDIR)/main.cpp         \
    $(SRCDIR)/mem.cpp          \
    $(SRCDIR)/mp3.cpp          \
    $(SRCDIR)/musyx.cpp        \

LOCAL_CFLAGS := $(COMMON_CFLAGS)

LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS) \
     -D__STDC_LIMIT_MACROS           \

include $(BUILD_SHARED_LIBRARY)
