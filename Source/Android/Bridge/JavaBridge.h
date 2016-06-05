#pragma once

#ifdef ANDROID

#include <Project64-core/Plugins/PluginClass.h>
#include <jni.h>

class JavaBridge : 
	public RenderWindow
{
public:
	JavaBridge (JavaVM* vm);

    //Render window functions
	void GfxThreadInit();
    void GfxThreadDone();
    void SwapWindow();

    //Rom List
    void RomListReset(void);
    void RomListAddItem(const char * FullFileName, const char * FileName, const char * GoodName, uint32_t TextColor );
    void RomListLoaded(void);

    //Notification
    void DisplayMessage(const char * Message);

private:
	JavaBridge(void);		                    // Disable default constructor
	JavaBridge(const JavaBridge&);				// Disable copy constructor
    JavaBridge& operator=(const JavaBridge&);	// Disable assignment

	JavaVM* m_vm;
    jclass m_GalleryActivityClass;
    jclass m_NotifierClass;
};

#endif