#pragma once

class CDumpMemory :
    public CDebugDialog<CDumpMemory>
{
public:
    enum
    {
        IDD = IDD_Cheats_DumpMemory
    };

    CDumpMemory(CDebuggerUI * debugger);
    virtual ~CDumpMemory(void);

private:
    CDumpMemory(void);
    CDumpMemory(const CDumpMemory &);
    CDumpMemory & operator=(const CDumpMemory &);

    enum DumpFormat
    {
        DisassemblyWithPC,
        RawBigEndian
    };

    BEGIN_MSG_MAP_EX(CDumpMemory)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
    }
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL & bHandled);
    void OnExitSizeMove(void);

    bool DumpMemory(LPCTSTR FileName, DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC);

    CComboBox m_FormatList;
    CEdit m_FileName;
    CEditNumber32 m_StartAddress, m_EndAddress, m_PC;
};
