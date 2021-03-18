#include "JavaRomList.h"
#include "JavaBridge.h"

#ifdef ANDROID
extern JavaBridge * g_JavaBridge;

void CJavaRomList::RomListReset(void)
{
    if (g_JavaBridge)
    {
        g_JavaBridge->RomListReset();
    }
}

void CJavaRomList::RomAddedToList(int32_t ListPos)
{
    if (g_JavaBridge)
    {
        ROM_INFO * pRomInfo = &m_RomInfo[ListPos];
        g_JavaBridge->RomListAddItem(pRomInfo->szFullFileName, pRomInfo->FileName, pRomInfo->GoodName, pRomInfo->TextColor);
    }
}

void CJavaRomList::RomListLoaded(void)
{
    if (g_JavaBridge)
    {
        g_JavaBridge->RomListLoaded();
    }
}

#endif
