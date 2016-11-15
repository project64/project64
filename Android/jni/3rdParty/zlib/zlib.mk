#########################
# zlib
#########################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./3rdParty/zlib

LOCAL_MODULE := zlib

LOCAL_C_INCLUDES :=         \

LOCAL_SRC_FILES :=                       \
    $(SRCDIR)/adler32.c           \
    $(SRCDIR)/compress.c          \
    $(SRCDIR)/crc32.c             \
    $(SRCDIR)/deflate.c           \
    $(SRCDIR)/gzclose.c           \
    $(SRCDIR)/gzlib.c             \
    $(SRCDIR)/gzread.c            \
    $(SRCDIR)/gzwrite.c           \
    $(SRCDIR)/infback.c           \
    $(SRCDIR)/inffast.c           \
    $(SRCDIR)/inflate.c           \
    $(SRCDIR)/inftrees.c          \
    $(SRCDIR)/trees.c             \
    $(SRCDIR)/uncompr.c           \
    $(SRCDIR)/zutil.c                   \
    $(SRCDIR)/contrib/minizip/ioapi.c   \
    $(SRCDIR)/contrib/minizip/miniunz.c \
    $(SRCDIR)/contrib/minizip/minizip.c \
    $(SRCDIR)/contrib/minizip/mztools.c \
    $(SRCDIR)/contrib/minizip/unzip.c   \
    $(SRCDIR)/contrib/minizip/zip.c     \

LOCAL_CFLAGS := $(COMMON_CFLAGS)        \
    -DIOAPI_NO_64                       \
    
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)

include $(BUILD_STATIC_LIBRARY)