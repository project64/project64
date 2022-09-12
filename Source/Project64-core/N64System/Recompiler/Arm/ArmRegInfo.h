#pragma once
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/RegBase.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>
#include <Project64-core/N64System/Mips/Register.h>

class CArmRegInfo :
    public CRegBase,
    private CDebugSettings,
    private CSystemRegisters
{
public:
    // Enums
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
        VARIABLE_TLB_WRITEMAP = 4,
        VARIABLE_TLB_LOAD_ADDRESS = 5,
        VARIABLE_TLB_STORE_ADDRESS = 6,
        VARIABLE_NEXT_TIMER = 7,
    };

    CArmRegInfo(CCodeBlock & CodeBlock, CArmOps & Assembler);
    CArmRegInfo(const CArmRegInfo&);
    ~CArmRegInfo();

    CArmRegInfo& operator=(const CArmRegInfo&);

    bool operator==(const CArmRegInfo& right) const;
    bool operator!=(const CArmRegInfo& right) const;

    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void FixRoundModel(FPU_ROUND RoundMethod);
    void Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad);
    void Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad);
    CArmOps::ArmReg FreeArmReg(bool TempMapping);
    void WriteBackRegisters();

    CArmOps::ArmReg Map_TempReg(CArmOps::ArmReg Reg, int32_t MipsReg, bool LoadHiWord);
    CArmOps::ArmReg Map_Variable(VARIABLE_MAPPED variable, CArmOps::ArmReg Reg = CArmOps::Arm_Any);
    CArmOps::ArmReg GetVariableReg(VARIABLE_MAPPED variable) const;
    void ProtectGPR(uint32_t Reg);
    void UnProtectGPR(uint32_t Reg);
    void UnMap_AllFPRs();
    CArmOps::ArmReg UnMap_TempReg(bool TempMapping);
    void UnMap_GPR(uint32_t Reg, bool WriteBackValue);
    void WriteBack_GPR(uint32_t MipsReg, bool Unmapping);
    bool UnMap_ArmReg(CArmOps::ArmReg Reg);
    void ResetRegProtection();

    inline CArmOps::ArmReg GetMipsRegMapLo(int32_t Reg) const { return m_RegMapLo[Reg]; }
    inline CArmOps::ArmReg GetMipsRegMapHi(int32_t Reg) const { return m_RegMapHi[Reg]; }
    inline void SetMipsRegMapLo(int32_t GetMipsReg, CArmOps::ArmReg Reg) { m_RegMapLo[GetMipsReg] = Reg; }
    inline void SetMipsRegMapHi(int32_t GetMipsReg, CArmOps::ArmReg Reg) { m_RegMapHi[GetMipsReg] = Reg; }

    inline uint32_t GetArmRegMapOrder(CArmOps::ArmReg Reg) const { return m_ArmReg_MapOrder[Reg]; }
    inline bool GetArmRegProtected(CArmOps::ArmReg Reg) const { return m_ArmReg_Protected[Reg]; }
    inline REG_MAPPED GetArmRegMapped(CArmOps::ArmReg Reg) const { return m_ArmReg_MappedTo[Reg]; }

    inline void SetArmRegMapOrder(CArmOps::ArmReg Reg, uint32_t Order) { m_ArmReg_MapOrder[Reg] = Order; }
    inline void SetArmRegProtected(CArmOps::ArmReg Reg, bool Protected) { m_ArmReg_Protected[Reg] = Protected; }
    inline void SetArmRegMapped(CArmOps::ArmReg Reg, REG_MAPPED Mapping) { m_ArmReg_MappedTo[Reg] = Mapping; }

    inline VARIABLE_MAPPED GetVariableMappedTo(CArmOps::ArmReg Reg) const { return m_Variable_MappedTo[Reg]; }
    inline void SetVariableMappedTo(CArmOps::ArmReg Reg, VARIABLE_MAPPED variable) { m_Variable_MappedTo[Reg] = variable; }
    static const char * VariableMapName(VARIABLE_MAPPED variable);

    void LogRegisterState(void);

private:
    CArmRegInfo();

    CCodeBlock & m_CodeBlock;
    CArmOps & m_Assembler;

    bool ShouldPushPopReg (CArmOps::ArmReg Reg);

    CArmOps::ArmReg m_RegMapHi[32];
    CArmOps::ArmReg m_RegMapLo[32];
    uint32_t m_ArmReg_MapOrder[16];
    bool m_ArmReg_Protected[16];
    REG_MAPPED m_ArmReg_MappedTo[16];
    VARIABLE_MAPPED m_Variable_MappedTo[16];
    bool m_InCallDirect;
};
#endif
