###############################
# Project64-gfx-Project64
###############################
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./

LOCAL_MODULE := Project64-gfx
LOCAL_STATIC_LIBRARIES := common                \
                          zlib                  \
                          Settings              \
                          png                   \

LOCAL_C_INCLUDES :=                             \
    $(LOCAL_PATH)/$(SRCDIR)/Project64-video/Renderer/inc        \
    $(LOCAL_PATH)/$(SRCDIR)/3rdParty            \

LOCAL_SRC_FILES :=                              \
    $(SRCDIR)/Project64-video/3dmath.cpp                \
    $(SRCDIR)/Project64-video/Android.cpp               \
    $(SRCDIR)/Project64-video/Combine.cpp               \
    $(SRCDIR)/Project64-video/Config.cpp                \
    $(SRCDIR)/Project64-video/CRC.cpp                   \
    $(SRCDIR)/Project64-video/Debugger.cpp              \
    $(SRCDIR)/Project64-video/DepthBufferRender.cpp     \
    $(SRCDIR)/Project64-video/F3DTEXA.cpp               \
    $(SRCDIR)/Project64-video/FBtoScreen.cpp            \
    $(SRCDIR)/Project64-video/Main.cpp                  \
    $(SRCDIR)/Project64-video/rdp.cpp                   \
    $(SRCDIR)/Project64-video/ScreenResolution.cpp      \
    $(SRCDIR)/Project64-video/Settings.cpp              \
    $(SRCDIR)/Project64-video/TexBuffer.cpp             \
    $(SRCDIR)/Project64-video/TexCache.cpp              \
    $(SRCDIR)/Project64-video/trace.cpp                 \
    $(SRCDIR)/Project64-video/turbo3D.cpp               \
    $(SRCDIR)/Project64-video/ucode.cpp                 \
    $(SRCDIR)/Project64-video/ucode00.cpp               \
    $(SRCDIR)/Project64-video/ucode01.cpp               \
    $(SRCDIR)/Project64-video/ucode02.cpp               \
    $(SRCDIR)/Project64-video/ucode03.cpp               \
    $(SRCDIR)/Project64-video/ucode04.cpp               \
    $(SRCDIR)/Project64-video/ucode05.cpp               \
    $(SRCDIR)/Project64-video/ucode06.cpp               \
    $(SRCDIR)/Project64-video/ucode07.cpp               \
    $(SRCDIR)/Project64-video/ucode08.cpp               \
    $(SRCDIR)/Project64-video/ucode09.cpp               \
    $(SRCDIR)/Project64-video/ucode09rdp.cpp            \
    $(SRCDIR)/Project64-video/ucodeFB.cpp                  \
    $(SRCDIR)/Project64-video/Util.cpp                  \
    $(SRCDIR)/Project64-video/Ext_TxFilter.cpp          \
    $(SRCDIR)/Project64-video/Renderer/OGLEScombiner.cpp        \
    $(SRCDIR)/Project64-video/Renderer/OGLESgeometry.cpp        \
    $(SRCDIR)/Project64-video/Renderer/OGLESglitchmain.cpp      \
    $(SRCDIR)/Project64-video/Renderer/OGLEStextures.cpp        \
    $(SRCDIR)/Project64-video/Renderer/OGLESwrappers.cpp        \
    $(SRCDIR)/Project64-video/Renderer/Renderer.cpp        \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxFilterExport.cpp        \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxFilter.cpp              \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxCache.cpp               \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxTexCache.cpp            \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxHiResCache.cpp          \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxQuantize.cpp            \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxUtil.cpp                \
    $(SRCDIR)/Project64-video/TextureEnhancer/TextureFilters.cpp        \
    $(SRCDIR)/Project64-video/TextureEnhancer/TextureFilters_2xsai.cpp  \
    $(SRCDIR)/Project64-video/TextureEnhancer/TextureFilters_hq2x.cpp   \
    $(SRCDIR)/Project64-video/TextureEnhancer/TextureFilters_hq4x.cpp   \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxImage.cpp               \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxReSample.cpp            \
    $(SRCDIR)/Project64-video/TextureEnhancer/TxDbg.cpp                 \
    $(SRCDIR)/Project64-video/TextureEnhancer/tc-1.1+/fxt1.c            \
    $(SRCDIR)/Project64-video/TextureEnhancer/tc-1.1+/dxtn.c            \
    $(SRCDIR)/Project64-video/TextureEnhancer/tc-1.1+/wrapper.c         \
    $(SRCDIR)/Project64-video/TextureEnhancer/tc-1.1+/texstore.c        \

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