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
	SyncBridge(void);		                    // Disable default constructor
	SyncBridge(const SyncBridge&);				// Disable copy constructor
    SyncBridge& operator=(const SyncBridge&);	// Disable assignment

	JavaBridge * m_JavaBridge;
};

#endif
