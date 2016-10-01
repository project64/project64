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
#include <Project64-core/N64System/Mips/RegisterClass.h>

class CArmRegInfo :
    public CRegBase,
    public CArmOps,
    private CSystemRegisters
{
public:
    //enums
    enum REG_MAPPED
    {
        NotMapped = 0,
        GPR_Mapped = 1,
        Temp_Mapped = 2,
        Variable_Mapped = 3,
    };

    enum VARIABLE_MAPPED
    {
        VARIABLE_UNKNOWN = 0,
        VARIABLE_GPR = 1,
        VARIABLE_FPR = 2,
        VARIABLE_TLB_READMAP = 3,
        VARIABLE_NEXT_TIMER = 4,
    };

    CArmRegInfo();
    CArmRegInfo(const CArmRegInfo&);
    ~CArmRegInfo();

    CArmRegInfo& operator=(const CArmRegInfo&);

    bool operator==(const CArmRegInfo& right) const;
    bool operator!=(const CArmRegInfo& right) const;

    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void FixRoundModel(FPU_ROUND RoundMethod);
    ArmReg FreeArmReg();
    void WriteBackRegisters();

    ArmReg Map_TempReg(ArmReg Reg, int32_t MipsReg, bool LoadHiWord);
    ArmReg Map_Variable(VARIABLE_MAPPED variable);
    inline uint32_t GetArmRegMapOrder(ArmReg Reg) const { return m_ArmReg_MapOrder[Reg]; }
    inline bool GetArmRegProtected(ArmReg Reg) const { return m_ArmReg_Protected[Reg]; }
    inline REG_MAPPED GetArmRegMapped(ArmReg Reg) const { return m_ArmReg_MappedTo[Reg]; }
    inline void SetArmRegMapOrder(ArmReg Reg, uint32_t Order) { m_ArmReg_MapOrder[Reg] = Order; }
    inline void SetArmRegProtected(ArmReg Reg, bool Protected) { m_ArmReg_Protected[Reg] = Protected; }
    inline void SetArmRegMapped(ArmReg Reg, REG_MAPPED Mapping) { m_ArmReg_MappedTo[Reg] = Mapping; }
private:
    uint32_t m_ArmReg_MapOrder[16];
    bool m_ArmReg_Protected[16];
    REG_MAPPED m_ArmReg_MappedTo[16];
    VARIABLE_MAPPED m_Variable_MappedTo[16];
    bool m_InCallDirect;
};
#endif
