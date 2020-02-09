CXX := g++
LD  := g++
WINDRES := windres

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
	-luuid \
	-static -lwinpthread

LDFLAGS := \
	-mwindows \
	-static-libgcc \
	-static-libstdc++

DEPFILES := $(addprefix .deps/,$(OBJECTS:.o=.d))

.PHONY: all clean

all: .deps $(TARGET)

-include $(DEPFILES)

clean:
	rm -f $(TARGET) $(OBJECTS) $(RESOURCE) $(DEPFILES)
	rm -rf .deps

.deps:
	mkdir -p $@

%.res: %.rc
	$(WINDRES) -O coff -o $@ -i $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -MF .deps/$(@:.o=.d) -o $@ -c $<

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(LD) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
