#pragma once
#include "Breakpoints.h"
#include "Debugger-RegisterTabData.h"

class CEditReg64 : 
    public CWindowImpl<CEditReg64, CEdit>,
    private CDebugSettings
{
public:
    static uint64_t ParseValue(const char* wordPair);
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
    void CopyTabRegisters();
    void CopyAllRegisters();

private:
    CRegisterTabs(const CRegisterTabs&);
    CRegisterTabs& operator=(const CRegisterTabs&);

    stdstr CopyTabRegisters(int id);

    static void RegisterChanged(HWND hDlg, TAB_ID srcTabId, WPARAM wParam);

    static INT_PTR CALLBACK TabProcDefault(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);


    static void InitRegisterEdit(CWindow& tab, CEditNumber32& edit, FieldPair ctrl);
    static void InitRegisterEdits(CWindow& tab, CEditNumber32* edits, const TabRecord* ctrlIds);
    static void InitRegisterEdit64(CWindow& tab, CEditReg64& edit, FieldPair ctrl);
    static void InitRegisterEdits64(CWindow& tab, CEditReg64* edits, const TabRecord* ctrlIds);

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

    static constexpr FieldPair GPRHi = {IDC_HI_LBL, IDC_HI_EDIT};
    static constexpr FieldPair GPRLo = {IDC_LO_LBL, IDC_LO_EDIT};

    static constexpr FieldPair FPRFCSR = {IDC_FCSR_LBL, IDC_FCSR_EDIT};

    static constexpr FieldPair SPPC = {IDC_SP_PC_LBL, IDC_SP_PC_EDIT};

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

    // For static dlgprocs, assumes single instance
    static bool m_bColorsEnabled;
    static CDebuggerUI* m_Debugger;

    vector<CWindow> m_TabWindows;
    bool m_attached;

    CWindow m_GPRTab;
    CEditReg64 m_GPREdits[TabData::GPR.FieldCount];
    CEditReg64 m_HIEdit;
    CEditReg64 m_LOEdit;

    CWindow m_FPRTab;
    CEditNumber32 m_FPREdits[TabData::FPR.FieldCount];
    CEditNumber32 m_FCSREdit;

    CWindow m_COP0Tab;
    CEditNumber32 m_COP0Edits[TabData::COP0.FieldCount];
    CStatic m_CauseTip;

    CWindow m_RDRAMTab;
    CEditNumber32 m_RDRAMEdits[TabData::RDRAM.FieldCount];

    CWindow m_SPTab;
    CEditNumber32 m_SPEdits[TabData::SP.FieldCount];
    CEditNumber32 m_SPPCEdit;

    CWindow m_DPCTab;
    CEditNumber32 m_DPCEdits[TabData::DPC.FieldCount];

    CWindow m_MITab;
    CEditNumber32 m_MIEdits[TabData::MI.FieldCount];

    CWindow m_VITab;
    CEditNumber32 m_VIEdits[TabData::VI.FieldCount];

    CWindow m_AITab;
    CEditNumber32 m_AIEdits[TabData::AI.FieldCount];

    CWindow m_PITab;
    CEditNumber32 m_PIEdits[TabData::PI.FieldCount];

    CWindow m_RITab;
    CEditNumber32 m_RIEdits[TabData::RI.FieldCount];

    CWindow m_SITab;
    CEditNumber32 m_SIEdits[TabData::SI.FieldCount];

    CWindow m_DDTab;
    CEditNumber32 m_DDEdits[TabData::DD.FieldCount];
};
