###############################
# Project64-gfx-glide64
###############################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./

LOCAL_MODULE := Project64-gfx-glide64
LOCAL_STATIC_LIBRARIES := common                \
                          zlib                  \
                          Settings              \
                          png                   \

LOCAL_C_INCLUDES :=                             \
    $(LOCAL_PATH)/$(SRCDIR)/Glitch64/inc        \
    $(LOCAL_PATH)/$(SRCDIR)/3rdParty            \

LOCAL_SRC_FILES :=                              \
    $(SRCDIR)/Glitch64/OGLEScombiner.cpp        \
    $(SRCDIR)/Glitch64/OGLESgeometry.cpp        \
    $(SRCDIR)/Glitch64/OGLESglitchmain.cpp      \
    $(SRCDIR)/Glitch64/OGLEStextures.cpp        \
    $(SRCDIR)/Glitch64/OGLESwrappers.cpp        \
    $(SRCDIR)/Glide64/3dmath.cpp                \
    $(SRCDIR)/Glide64/Combine.cpp               \
    $(SRCDIR)/Glide64/Config.cpp                \
    $(SRCDIR)/Glide64/CRC.cpp                   \
    $(SRCDIR)/Glide64/Debugger.cpp              \
    $(SRCDIR)/Glide64/DepthBufferRender.cpp     \
    $(SRCDIR)/Glide64/FBtoScreen.cpp            \
    $(SRCDIR)/Glide64/Keys.cpp                  \
    $(SRCDIR)/Glide64/Main.cpp                  \
    $(SRCDIR)/Glide64/rdp.cpp                   \
    $(SRCDIR)/Glide64/Settings.cpp              \
    $(SRCDIR)/Glide64/TexBuffer.cpp             \
    $(SRCDIR)/Glide64/TexCache.cpp              \
    $(SRCDIR)/Glide64/trace.cpp                 \
    $(SRCDIR)/Glide64/Util.cpp                  \
    $(SRCDIR)/Glide64/Ext_TxFilter.cpp          \
    $(SRCDIR)/GlideHQ/TxFilterExport.cpp        \
    $(SRCDIR)/GlideHQ/TxFilter.cpp              \
    $(SRCDIR)/GlideHQ/TxCache.cpp               \
    $(SRCDIR)/GlideHQ/TxTexCache.cpp            \
    $(SRCDIR)/GlideHQ/TxHiResCache.cpp          \
    $(SRCDIR)/GlideHQ/TxQuantize.cpp            \
    $(SRCDIR)/GlideHQ/TxUtil.cpp                \
    $(SRCDIR)/GlideHQ/TextureFilters.cpp        \
    $(SRCDIR)/GlideHQ/TextureFilters_2xsai.cpp  \
    $(SRCDIR)/GlideHQ/TextureFilters_hq2x.cpp   \
    $(SRCDIR)/GlideHQ/TextureFilters_hq4x.cpp   \
    $(SRCDIR)/GlideHQ/TxImage.cpp               \
    $(SRCDIR)/GlideHQ/TxReSample.cpp            \
    $(SRCDIR)/GlideHQ/TxDbg.cpp                 \
    $(SRCDIR)/GlideHQ/tc-1.1+/fxt1.c            \
    $(SRCDIR)/GlideHQ/tc-1.1+/dxtn.c            \
    $(SRCDIR)/GlideHQ/tc-1.1+/wrapper.c         \
    $(SRCDIR)/GlideHQ/tc-1.1+/texstore.c        \

LOCAL_CFLAGS :=         \
    $(COMMON_CFLAGS)    \
    -DUSE_FRAMESKIPPER  \
    -DNOSSE             \
    -DUSE_GLES          \
    -fsigned-char       \
    
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
    
LOCAL_CPP_FEATURES := exceptions

LOCAL_LDLIBS :=         \
    -ldl                \
    -lGLESv2            \
    -llog               \
    -latomic            \

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    # Use for ARM7a:
    LOCAL_CFLAGS += -mfpu=vfp
    LOCAL_CFLAGS += -mfloat-abi=softfp
    
endif

include $(BUILD_SHARED_LIBRARY)