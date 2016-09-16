#########################
# common
#########################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./Common

LOCAL_MODULE := common

LOCAL_C_INCLUDES := ../     \
    $(SDL_INCLUDES)         \
               
LOCAL_SRC_FILES := \
    $(SRCDIR)/CriticalSection.cpp        \
    $(SRCDIR)/DateTimeClass.cpp          \
    $(SRCDIR)/FileClass.cpp              \
    $(SRCDIR)/HighResTimeStamp.cpp       \
    $(SRCDIR)/IniFileClass.cpp           \
    $(SRCDIR)/LogClass.cpp               \
    $(SRCDIR)/md5.cpp                    \
    $(SRCDIR)/MemoryManagement.cpp       \
    $(SRCDIR)/path.cpp                   \
    $(SRCDIR)/Platform.cpp               \
    $(SRCDIR)/StdString.cpp              \
    $(SRCDIR)/SyncEvent.cpp              \
    $(SRCDIR)/Thread.cpp                 \
    $(SRCDIR)/Trace.cpp                  \
    $(SRCDIR)/Util.cpp                   \

LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)

include $(BUILD_STATIC_LIBRARY)