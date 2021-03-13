#include "SyncBridge.h"

#ifdef ANDROID
SyncBridge::SyncBridge(JavaBridge * javaBridge) :
    m_JavaBridge(javaBridge)
{
}

void SyncBridge::GfxThreadInit()
{
}

void SyncBridge::GfxThreadDone()
{
}

void SyncBridge::SwapWindow()
{
    if (m_JavaBridge)
    {
        m_JavaBridge->SwapWindow();
    }
}

#endif
