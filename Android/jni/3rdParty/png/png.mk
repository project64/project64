#########################
# png
#########################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./3rdParty/png

LOCAL_MODULE := png

LOCAL_C_INCLUDES :=               \
    ./jni/3rdParty/               \
                
LOCAL_SRC_FILES := \
    $(SRCDIR)/png.c               \
    $(SRCDIR)/pngerror.c          \
    $(SRCDIR)/pngget.c            \
    $(SRCDIR)/pngmem.c            \
    $(SRCDIR)/pngpread.c          \
    $(SRCDIR)/pngread.c           \
    $(SRCDIR)/pngrio.c            \
    $(SRCDIR)/pngrtran.c          \
    $(SRCDIR)/pngrutil.c          \
    $(SRCDIR)/pngset.c            \
    $(SRCDIR)/pngtest.c           \
    $(SRCDIR)/pngtrans.c          \
    $(SRCDIR)/pngwio.c            \
    $(SRCDIR)/pngwrite.c          \
    $(SRCDIR)/pngwtran.c          \
    $(SRCDIR)/pngwutil.c          \

LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)

include $(BUILD_STATIC_LIBRARY)