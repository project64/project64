#pragma once

#ifdef ANDROID

#include <Project64-core/Plugins/PluginClass.h>
#include <jni.h>

class JavaBridge :
    public RenderWindow
{
public:
    JavaBridge(JavaVM* vm);

    // Render window functions
    void GfxThreadInit();
    void GfxThreadDone();
    void SwapWindow();

    // ROM list
    void RomListReset(void);
    void RomListAddItem(const char * FullFileName, const char * FileName, const char * GoodName, uint32_t TextColor);
    void RomListLoaded(void);
    void RecentRomsUpdated(void);

    // Notification
    void DisplayError(const char * Message);
    void DisplayMessage(const char * Message, int DisplayTime);
    void DisplayMessage2(const char * Message);
    void EmulationStarted(void);
    void EmulationStopped(void);

private:
    JavaBridge(void);
    JavaBridge(const JavaBridge&);
    JavaBridge& operator=(const JavaBridge&);

    JavaVM* m_vm;
    jclass m_GalleryActivityClass;
    jclass m_NotifierClass;
};

#endif
