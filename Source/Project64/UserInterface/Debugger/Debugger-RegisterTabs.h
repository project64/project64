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
#include "Breakpoints.h"

class CEditReg64 : 
    public CWindowImpl<CEditReg64, CEdit>,
    private CDebugSettings
{
public:
    static uint64_t ParseValue(char* wordPair);
    BOOL Attach(HWND hWndNew);
    LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLostFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    uint64_t GetValue();
    void SetValue(uint32_t h, uint32_t l);
    void SetValue(uint64_t value);

    BEGIN_MSG_MAP_EX(CRegEdit64)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnLostFocus)
    END_MSG_MAP()
};

class CRegisterTabs : 
    public CTabCtrl,
    public CDebugSettings
{
    enum TAB_ID
    {
        TabDefault,
        TabGPR,
        TabFPR
    };

public:
    CRegisterTabs(void);
    ~CRegisterTabs();

    void Attach(HWND hWndNew, CDebuggerUI* debugger);
    HWND Detach();

    CWindow AddTab(char* caption, int dialogId, DLGPROC dlgProc);
    void ShowTab(int nPage);
    CRect GetPageRect();
    void RedrawCurrentTab();
    void RefreshEdits();
    void SetColorsEnabled(bool bColorsEnabled);

private:
    CRegisterTabs(const CRegisterTabs&);              // Disable copy constructor
    CRegisterTabs& operator=(const CRegisterTabs&);   // Disable assignment

    static void RegisterChanged(HWND hDlg, TAB_ID srcTabId, WPARAM wParam);

    static INT_PTR CALLBACK TabProcDefault(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    static void InitRegisterEdit(CWindow& tab, CEditNumber32& edit, WORD ctrlId);
    static void InitRegisterEdits(CWindow& tab, CEditNumber32* edits, const WORD* ctrlIds, uint32_t ctrlIdsCount);
    static void InitRegisterEdit64(CWindow& tab, CEditReg64& edit, WORD ctrlId);
    static void InitRegisterEdits64(CWindow& tab, CEditReg64* edits, const WORD* ctrlIds, uint32_t ctrlIdsCount);
    static void ZeroRegisterEdit(CEditNumber32& edit);
    static void ZeroRegisterEdits(CEditNumber32* edits, uint32_t ctrlIdsCount);
    static void ZeroRegisterEdit64(CEditReg64& edit);
    static void ZeroRegisterEdits64(CEditReg64* edits, uint32_t ctrlIdsCount);

    typedef union
    {
        uint32_t intval;

        struct
        {
            unsigned : 2;
            unsigned exceptionCode : 5;
            unsigned : 1;
            unsigned pendingInterrupts : 8;
            unsigned : 12;
            unsigned coprocessor : 2;
            unsigned : 1;
            unsigned fromDelaySlot : 1;
        };
    } CAUSE;

    static constexpr WORD GPRLabelIds[] =
    {
        IDC_R0_LBL,  IDC_R1_LBL,  IDC_R2_LBL,  IDC_R3_LBL,
        IDC_R4_LBL,  IDC_R5_LBL,  IDC_R6_LBL,  IDC_R7_LBL,
        IDC_R8_LBL,  IDC_R9_LBL,  IDC_R10_LBL, IDC_R11_LBL,
        IDC_R12_LBL, IDC_R13_LBL, IDC_R14_LBL, IDC_R15_LBL,
        IDC_R16_LBL, IDC_R17_LBL, IDC_R18_LBL, IDC_R19_LBL,
        IDC_R20_LBL, IDC_R21_LBL, IDC_R22_LBL, IDC_R23_LBL,
        IDC_R24_LBL, IDC_R25_LBL, IDC_R26_LBL, IDC_R27_LBL,
        IDC_R28_LBL, IDC_R29_LBL, IDC_R30_LBL, IDC_R31_LBL,
        NULL
    };

    static constexpr WORD GPREditIds[] =
    {
        IDC_R0_EDIT,  IDC_R1_EDIT,  IDC_R2_EDIT,  IDC_R3_EDIT,
        IDC_R4_EDIT,  IDC_R5_EDIT,  IDC_R6_EDIT,  IDC_R7_EDIT,
        IDC_R8_EDIT,  IDC_R9_EDIT,  IDC_R10_EDIT, IDC_R11_EDIT,
        IDC_R12_EDIT, IDC_R13_EDIT, IDC_R14_EDIT, IDC_R15_EDIT,
        IDC_R16_EDIT, IDC_R17_EDIT, IDC_R18_EDIT, IDC_R19_EDIT,
        IDC_R20_EDIT, IDC_R21_EDIT, IDC_R22_EDIT, IDC_R23_EDIT,
        IDC_R24_EDIT, IDC_R25_EDIT, IDC_R26_EDIT, IDC_R27_EDIT,
        IDC_R28_EDIT, IDC_R29_EDIT, IDC_R30_EDIT, IDC_R31_EDIT,
        NULL
    };

    static constexpr WORD FPREditIds[] =
    {
        IDC_F0_EDIT,  IDC_F1_EDIT,  IDC_F2_EDIT,  IDC_F3_EDIT,
        IDC_F4_EDIT,  IDC_F5_EDIT,  IDC_F6_EDIT,  IDC_F7_EDIT,
        IDC_F8_EDIT,  IDC_F9_EDIT,  IDC_F10_EDIT, IDC_F11_EDIT,
        IDC_F12_EDIT, IDC_F13_EDIT, IDC_F14_EDIT, IDC_F15_EDIT,
        IDC_F16_EDIT, IDC_F17_EDIT, IDC_F18_EDIT, IDC_F19_EDIT,
        IDC_F20_EDIT, IDC_F21_EDIT, IDC_F22_EDIT, IDC_F23_EDIT,
        IDC_F24_EDIT, IDC_F25_EDIT, IDC_F26_EDIT, IDC_F27_EDIT,
        IDC_F28_EDIT, IDC_F29_EDIT, IDC_F30_EDIT, IDC_F31_EDIT,
        NULL
    };

    static constexpr WORD COP0EditIds[] =
    {
        IDC_COP0_0_EDIT,  IDC_COP0_1_EDIT,  IDC_COP0_2_EDIT,  IDC_COP0_3_EDIT,
        IDC_COP0_4_EDIT,  IDC_COP0_5_EDIT,  IDC_COP0_6_EDIT,  IDC_COP0_7_EDIT,
        IDC_COP0_8_EDIT,  IDC_COP0_9_EDIT,  IDC_COP0_10_EDIT, IDC_COP0_11_EDIT,
        IDC_COP0_12_EDIT, IDC_COP0_13_EDIT, IDC_COP0_14_EDIT, IDC_COP0_15_EDIT,
        IDC_COP0_16_EDIT, IDC_COP0_17_EDIT, IDC_COP0_18_EDIT,
        NULL
    };

    static constexpr WORD RDRAMEditIds[] =
    {
        IDC_RDRAM00_EDIT, IDC_RDRAM04_EDIT, IDC_RDRAM08_EDIT, IDC_RDRAM0C_EDIT,
        IDC_RDRAM10_EDIT, IDC_RDRAM14_EDIT, IDC_RDRAM18_EDIT, IDC_RDRAM1C_EDIT,
        IDC_RDRAM20_EDIT, IDC_RDRAM24_EDIT,
        NULL
    };

    static constexpr WORD SPEditIds[] =
    {
        IDC_SP00_EDIT, IDC_SP04_EDIT, IDC_SP08_EDIT, IDC_SP0C_EDIT,
        IDC_SP10_EDIT, IDC_SP14_EDIT, IDC_SP18_EDIT, IDC_SP1C_EDIT,
        NULL
    };

    static constexpr WORD DPCEditIds[] =
    {
        IDC_DPC00_EDIT, IDC_DPC04_EDIT, IDC_DPC08_EDIT, IDC_DPC0C_EDIT,
        IDC_DPC10_EDIT, IDC_DPC14_EDIT, IDC_DPC18_EDIT, IDC_DPC1C_EDIT,
        NULL
    };

    static constexpr WORD MIEditIds[] =
    {
        IDC_MI00_EDIT, IDC_MI04_EDIT, IDC_MI08_EDIT, IDC_MI0C_EDIT,
        NULL
    };

    static constexpr WORD VIEditIds[] =
    {
        IDC_VI00_EDIT, IDC_VI04_EDIT, IDC_VI08_EDIT, IDC_VI0C_EDIT,
        IDC_VI10_EDIT, IDC_VI14_EDIT, IDC_VI18_EDIT, IDC_VI1C_EDIT,
        IDC_VI20_EDIT, IDC_VI24_EDIT, IDC_VI28_EDIT, IDC_VI2C_EDIT,
        IDC_VI30_EDIT, IDC_VI34_EDIT,
        NULL
    };

    static constexpr WORD AIEditIds[] =
    {
        IDC_AI00_EDIT, IDC_AI04_EDIT, IDC_AI08_EDIT, IDC_AI0C_EDIT,
        IDC_AI10_EDIT, IDC_AI14_EDIT,
        NULL
    };

    static constexpr WORD PIEditIds[] =
    {
        IDC_PI00_EDIT, IDC_PI04_EDIT, IDC_PI08_EDIT, IDC_PI0C_EDIT,
        IDC_PI10_EDIT, IDC_PI14_EDIT, IDC_PI18_EDIT, IDC_PI1C_EDIT,
        IDC_PI20_EDIT, IDC_PI24_EDIT, IDC_PI28_EDIT, IDC_PI2C_EDIT,
        IDC_PI30_EDIT,
        NULL
    };

    static constexpr WORD RIEditIds[] =
    {
        IDC_RI00_EDIT, IDC_RI04_EDIT, IDC_RI08_EDIT, IDC_RI0C_EDIT,
        IDC_RI10_EDIT, IDC_RI14_EDIT, IDC_RI18_EDIT, IDC_RI1C_EDIT,
        NULL
    };

    static constexpr WORD SIEditIds[] =
    {
        IDC_SI00_EDIT, IDC_SI04_EDIT, IDC_SI08_EDIT, IDC_SI0C_EDIT,
        NULL
    };

    static constexpr WORD DDEditIds[] =
    {
        IDC_DD00_EDIT, IDC_DD04_EDIT, IDC_DD08_EDIT, IDC_DD0C_EDIT,
        IDC_DD10_EDIT, IDC_DD14_EDIT, IDC_DD18_EDIT, IDC_DD1C_EDIT,
        IDC_DD20_EDIT, IDC_DD24_EDIT, IDC_DD28_EDIT, IDC_DD2C_EDIT,
        IDC_DD30_EDIT, IDC_DD34_EDIT, IDC_DD38_EDIT, IDC_DD3C_EDIT,
        IDC_DD40_EDIT, IDC_DD44_EDIT, IDC_DD48_EDIT,
        NULL
    };

    // return reg num associated with the control or -1 if ctrl isn't in map
    static int GetCtrlRegNum(WORD ctrlId, const WORD* map)
    {
        for (uint32_t i = 0; map[i] != NULL; i++)
        {
            if (map[i] == ctrlId)
            {
                return i;
            }
        }
        return -1;
    }

    static constexpr char* ExceptionCodes[] =
    {
        "Interrupt",
        "TLB mod",
        "TLB load/fetch",
        "TLB store",
        "Address error (load/fetch)",
        "Address error (store)",
        "Bus error (instruction fetch)",
        "Bus error (data load/store)",
        "Syscall",
        "Breakpoint",
        "Reserved instruction",
        "Coprocessor unusable",
        "Arithmetic overflow",
        "Trap",
        "Virtual coherency (instruction)",
        "Floating-point",
        "? 16",
        "? 17",
        "? 18",
        "? 19",
        "? 20",
        "? 21",
        "? 22",
        "Watch",
        "? 24",
        "? 25",
        "? 26",
        "? 27",
        "? 28",
        "? 29",
        "? 30",
        "Virtual coherency (data)"
    };

    // for static dlgprocs, assumes single instance
    static bool m_bColorsEnabled;
    static CDebuggerUI* m_Debugger;

    vector<CWindow> m_TabWindows;
    bool m_attached;

    CWindow m_GPRTab;
    CEditReg64 m_GPREdits[sizeof(GPREditIds) / sizeof(GPREditIds[0])];
    CEditReg64 m_HIEdit;
    CEditReg64 m_LOEdit;

    CWindow m_FPRTab;
    CEditNumber32 m_FPREdits[sizeof(FPREditIds) / sizeof(FPREditIds[0])];
    CEditNumber32 m_FCSREdit;

    CWindow m_COP0Tab;
    CEditNumber32 m_COP0Edits[sizeof(COP0EditIds) / sizeof(COP0EditIds[0])];
    CStatic m_CauseTip;

    CWindow m_RDRAMTab;
    CEditNumber32 m_RDRAMEdits[sizeof(RDRAMEditIds) / sizeof(RDRAMEditIds[0])];

    CWindow m_SPTab;
    CEditNumber32 m_SPEdits[sizeof(SPEditIds) / sizeof(SPEditIds[0])];
    CEditNumber32 m_SPPCEdit;

    CWindow m_DPCTab;
    CEditNumber32 m_DPCEdits[sizeof(DPCEditIds) / sizeof(DPCEditIds[0])];

    CWindow m_MITab;
    CEditNumber32 m_MIEdits[sizeof(MIEditIds) / sizeof(MIEditIds[0])];

    CWindow m_VITab;
    CEditNumber32 m_VIEdits[sizeof(VIEditIds) / sizeof(VIEditIds[0])];

    CWindow m_AITab;
    CEditNumber32 m_AIEdits[sizeof(AIEditIds) / sizeof(AIEditIds[0])];

    CWindow m_PITab;
    CEditNumber32 m_PIEdits[sizeof(PIEditIds) / sizeof(PIEditIds[0])];

    CWindow m_RITab;
    CEditNumber32 m_RIEdits[sizeof(RIEditIds) / sizeof(RIEditIds[0])];

    CWindow m_SITab;
    CEditNumber32 m_SIEdits[sizeof(SIEditIds) / sizeof(SIEditIds[0])];

    CWindow m_DDTab;
    CEditNumber32 m_DDEdits[sizeof(DDEditIds) / sizeof(DDEditIds[0])];

};