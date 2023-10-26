#include "JavaRomList.h"
#include "JavaBridge.h"
#include <Project64-core/Multilanguage.h>

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
        if (pRomInfo->GoodName[0] == '#' && strcmp("#340#", pRomInfo->GoodName) == 0 && g_Lang != nullptr)
        {
            std::string GoodName = g_Lang->GetString(RB_NOT_GOOD_FILE);
            strncpy(pRomInfo->GoodName, GoodName.c_str(), sizeof(pRomInfo->GoodName) / sizeof(char));
        }
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
