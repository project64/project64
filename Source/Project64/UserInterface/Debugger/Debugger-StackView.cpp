#include "stdafx.h"
#include "DebuggerUI.h"
#include "Symbols.h"

CDebugStackView::CDebugStackView(CDebuggerUI * debugger) :
    CDebugDialog<CDebugStackView>(debugger)
{
}

CDebugStackView::~CDebugStackView(void)
{
}

LRESULT CDebugStackView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_StackPos);

    m_StackList.Attach(GetDlgItem(IDC_STACK_LIST));
    m_StackList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_StackList.AddColumn(L"#", 0);
    m_StackList.AddColumn(L"00", 1);
    m_StackList.AddColumn(L"04", 2);
    m_StackList.AddColumn(L"08", 3);
    m_StackList.AddColumn(L"0C", 4);

    m_StackList.SetColumnWidth(0, 22);
    m_StackList.SetColumnWidth(1, 64);
    m_StackList.SetColumnWidth(2, 64);
    m_StackList.SetColumnWidth(3, 64);
    m_StackList.SetColumnWidth(4, 64);

    m_SPStatic.Attach(GetDlgItem(IDC_SP_STATIC));

	LoadWindowPos();
	WindowCreated();

    return 0;
}

void CDebugStackView::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugStackView::OnDestroy(void)
{
    m_StackList.Detach();
    m_SPStatic.Detach();
    return 0;
}

LRESULT CDebugStackView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDC_MEM_BTN:
        if (g_Reg != nullptr)
        {
            m_Debugger->Debug_ShowMemoryLocation(g_Reg->m_GPR[29].UW[0], true);
        }
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    }
    return 0;
}

void CDebugStackView::Refresh()
{
    if (g_Reg == nullptr)
    {
        return;
    }

    m_StackList.SetRedraw(FALSE);
    m_StackList.DeleteAllItems();

    uint32_t spBase = g_Reg->m_GPR[29].UW[0];
    m_SPStatic.SetWindowText(stdstr_f("SP: %08X", spBase).ToUTF16().c_str());

    for (int i = 0; i < 0x10; i++)
    {
        wchar_t t[4];
        swprintf(t, sizeof(t) / sizeof(t[0]), L"%02X", i * 0x10);
        m_StackList.AddItem(i, 0, t);

        for (int j = 0; j < 4; j++)
        {
            uint32_t vaddr = spBase + (i * 0x10) + (j * 4);
            uint32_t val;

            if (!m_Debugger->DebugLoad_VAddr(vaddr, val))
            {
                m_StackList.AddItem(i, j + 1, L"????????");
                continue;
            }

            wchar_t valStr[9];
            wsprintf(valStr, L"%08X", val);
            m_StackList.AddItem(i, j + 1, valStr);
        }
    }

    m_StackList.SetRedraw(TRUE);
}
