/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/RegBase.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>

class CArmRegInfo :
    public CRegBase,
    public CArmOps
{
public:
    CArmRegInfo();
    CArmRegInfo(const CArmRegInfo&);
    ~CArmRegInfo();

    CArmRegInfo& operator=(const CArmRegInfo&);

    bool operator==(const CArmRegInfo& right) const;
    bool operator!=(const CArmRegInfo& right) const;
    
    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void WriteBackRegisters();

private:
    bool m_InCallDirect;
};
#endif
