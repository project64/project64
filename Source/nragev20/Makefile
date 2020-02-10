IS_GIT := $(if $(shell git rev-parse --show-toplevel 2>/dev/null),1)
IS_HG := $(if $(shell hg root 2>/dev/null),1)

ifeq ($(IS_HG),1)
$(info This is a hg repository)
GIT_VERSION := "hg:$(shell hg identify --id)"
VERSION_BUILD := 0
endif

ifeq ($(IS_GIT),1)
$(info This is a git repository)
GIT_VERSION := "git:$(shell git describe --always --long --dirty=+ 2>/dev/null)"
VERSION_BUILD := 0
endif

ifeq ($(or $(IS_GIT),$(IS_HG)),)
$(info Not inside a repository)
GIT_VERSION := "?"
VERSION_BUILD := 0
endif


CXX ?= g++
LD  := $(CXX)
WINDRES ?= windres

OBJECTS := \
	Debug.o \
	DirectInput.o \
	FileAccess.o \
	GBCart.o \
	Interface.o \
	International.o \
	NRagePluginV2.o \
	PakIO.o \
	XInputController.o

RESOURCE := NRagePluginV2.res

TARGET := PJ64_NRage.dll

CPPFLAGS := \
	-DGIT_VERSION=\"$(GIT_VERSION)\" \
	-DVERSION_BUILD=$(VERSION_BUILD) \
	-DNOMINMAX

CXXFLAGS := \
	-shared \
	-fPIC \
	-Os \
	-Wno-write-strings

LIBS := \
	-lcomctl32 \
	-lcomdlg32 \
	-ldinput8 \
	-lgdi32 \
	-lole32 \
	-loleaut32 \
	-luuid

ifeq ($(DYN_WINPTHREAD),1)
LIBS += -lwinpthread
else
LIBS += -static -lwinpthread
endif

LDFLAGS := \
	-mwindows \
	-static-libgcc \
	-static-libstdc++

DEPFILES := $(addprefix .deps/,$(OBJECTS:.o=.d))

.PHONY: all clean

all: .deps Version.h $(TARGET)

-include $(DEPFILES)

clean:
	-rm -f $(TARGET)
	-rm -f $(OBJECTS) $(RESOURCE)
	-rm -f $(DEPFILES)
	-rm -rf .deps
	-rm -f Version.h

.deps:
	mkdir -p $@

Version.h: Version.h.in
	cp $< $@

%.res: %.rc
	$(WINDRES) -O coff -o $@ -i $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -MF .deps/$(@:.o=.d) -o $@ -c $<

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(LD) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
