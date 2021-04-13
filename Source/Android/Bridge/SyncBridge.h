#pragma once

#ifdef ANDROID

#include "JavaBridge.h"

class SyncBridge : 
	public RenderWindow
{
public:
	SyncBridge (JavaBridge * javaBridge);

    // Render window functions
	void GfxThreadInit();
    void GfxThreadDone();
    void SwapWindow();

private:
	SyncBridge(void);
	SyncBridge(const SyncBridge&);
    SyncBridge& operator=(const SyncBridge&);

	JavaBridge * m_JavaBridge;
};

#endif
