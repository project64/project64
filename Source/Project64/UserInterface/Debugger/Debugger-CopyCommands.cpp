#include "stdafx.h"

#include "DebuggerUI.h"
#include "Symbols.h"

LRESULT CCopyCommandsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    CenterWindow();

    m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_AddressEdit.SetDisplayType(CEditNumber32::DisplayHex);
    m_CountEdit.Attach(GetDlgItem(IDC_COUNT_EDIT));
    m_CountEdit.SetDisplayType(CEditNumber32::DisplayDec);

    m_AddressEdit.SetWindowText(L"");
    m_AddressEdit.SetFocus();

    if (m_bHaveAddress)
    {
        m_AddressEdit.SetValue(m_InitAddress, DisplayMode::ZeroExtend);
        m_CountEdit.SetFocus();
    }

    m_CountEdit.SetValue(m_bHaveCount ? m_InitCount : 1, DisplayMode::None);

    return FALSE;
}

LRESULT CCopyCommandsDlg::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDOK:
        int addrLen = m_AddressEdit.GetWindowTextLength();
        if (!addrLen)
        {
            MessageBox(L"Address required", L"Error", MB_OK);
            return 0;
        }

        int countLen = m_CountEdit.GetWindowTextLength();
        if (!countLen)
        {
            MessageBox(L"Count required", L"Error", MB_OK);
            return 0;
        }

        uint32_t address = m_AddressEdit.GetValue();
        uint32_t count = m_CountEdit.GetValue();

        m_Debugger->CopyCommands(address, count);

        EndDialog(0);
        break;
    }
    return 0;
}

INT_PTR CCopyCommandsDlg::DoModal(CDebuggerUI * debugger)
{
    m_Debugger = debugger;
    m_bHaveAddress = false;
    m_bHaveCount = false;
    return CDialogImpl<CCopyCommandsDlg>::DoModal();
}

INT_PTR CCopyCommandsDlg::DoModal(CDebuggerUI * debugger, uint32_t initAddress)
{
    m_Debugger = debugger;
    m_bHaveAddress = true;
    m_bHaveCount = false;
    m_InitAddress = initAddress;
    return CDialogImpl<CCopyCommandsDlg>::DoModal();
}

INT_PTR CCopyCommandsDlg::DoModal(CDebuggerUI * debugger, uint32_t initAddress, uint32_t initCount)
{
    m_Debugger = debugger;
    m_bHaveAddress = true;
    m_bHaveCount = true;
    m_InitAddress = initAddress;
    m_InitCount = initCount;
    return CDialogImpl<CCopyCommandsDlg>::DoModal();
}
