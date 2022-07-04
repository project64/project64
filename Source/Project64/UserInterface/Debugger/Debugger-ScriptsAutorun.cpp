#include <stdafx.h>
#include "DebuggerUI.h"
#include <sstream>

CScriptsAutorunDlg::CScriptsAutorunDlg() :
    CDialogImpl<CScriptsAutorunDlg>(),
    m_hQuitScriptDirWatchEvent(nullptr),
    m_hScriptDirWatchThread(nullptr),
    m_bScriptListNeedsRefocus(false),
    m_bAutorunListNeedsRefocus(false),
    m_Debugger(nullptr),
    m_ScriptSystem(nullptr)
{
}

CScriptsAutorunDlg::~CScriptsAutorunDlg()
{
}

INT_PTR CScriptsAutorunDlg::DoModal(CDebuggerUI* debugger, stdstr selectedScriptName)
{
    m_Debugger = debugger;
    m_ScriptSystem = debugger->ScriptSystem();
    m_InitSelectedScriptName = std::move(selectedScriptName);
    return CDialogImpl<CScriptsAutorunDlg>::DoModal();
}

LRESULT CScriptsAutorunDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow();

    m_ScriptListView.Attach(GetDlgItem(IDC_SCRIPT_LIST));
    m_AutorunListView.Attach(GetDlgItem(IDC_AUTORUN_LIST));

    m_ScriptListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_ScriptListView.AddColumn(L"Script", 0);
    m_ScriptListView.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    
    m_AutorunListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_AutorunListView.AddColumn(L"Script", 0);
    m_AutorunListView.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

    m_hQuitScriptDirWatchEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_hScriptDirWatchThread = CreateThread(nullptr, 0, ScriptDirWatchProc, (void*)this, 0, nullptr);

    m_ScriptSystem->LoadAutorunList();

    RefreshAutorunList();
    RefreshScriptList();
    return 0;
}

LRESULT CScriptsAutorunDlg::OnDestroy(void)
{
    SetEvent(m_hQuitScriptDirWatchEvent);
    WaitForSingleObject(m_hScriptDirWatchThread, INFINITE);
    CloseHandle(m_hQuitScriptDirWatchEvent);
    CloseHandle(m_hScriptDirWatchThread);

    m_ScriptListView.Detach();
    m_AutorunListView.Detach();

    return 0;
}

LRESULT CScriptsAutorunDlg::OnOKCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    EndDialog(0);
    return 0;
}


LRESULT CScriptsAutorunDlg::OnAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_bScriptListNeedsRefocus = true;
    AddSelected();
    return 0;
}

LRESULT CScriptsAutorunDlg::OnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_bAutorunListNeedsRefocus = true;
    RemoveSelected();
    return 0;
}

LRESULT CScriptsAutorunDlg::OnScriptListDblClicked(NMHDR* /*pNMHDR*/)
{
    AddSelected();
    return 0;
}

LRESULT CScriptsAutorunDlg::OnCtrlSetFocus(NMHDR* pNMHDR)
{
    bool bEnableScriptButtons = false;
    bool bEnableAutorunButtons = false;
    
    switch (pNMHDR->idFrom)
    {
    case IDC_SCRIPT_LIST:
    case IDC_ADD_BTN:
        bEnableScriptButtons = true;
        bEnableAutorunButtons = false;
        break;
    case IDC_AUTORUN_LIST:
    case IDC_REMOVE_BTN:
        bEnableAutorunButtons = true;
        bEnableScriptButtons = false;
        break;
    }
    
    ::EnableWindow(GetDlgItem(IDC_ADD_BTN), bEnableScriptButtons);
    ::EnableWindow(GetDlgItem(IDC_REMOVE_BTN), bEnableAutorunButtons);

    return 0;
}

LRESULT CScriptsAutorunDlg::OnAutorunListDblClicked(NMHDR* /*pNMHDR*/)
{
    RemoveSelected();
    return 0;
}

LRESULT CScriptsAutorunDlg::OnRefreshScriptList(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    int nSelectedItem = m_ScriptListView.GetSelectedIndex();

    CPath searchPath(m_ScriptSystem->ScriptsDirPath(), "*");

    if (!searchPath.FindFirst(CPath::FIND_ATTRIBUTE_FILES))
    {
        return 0;
    }

    strlist fileNames;

    do
    {
        stdstr fileName = searchPath.GetNameExtension();
        if (searchPath.GetExtension() == "js" &&
            m_ScriptSystem->AutorunList().count(fileName) == 0)
        {
            fileNames.push_back(searchPath.GetNameExtension());
        }
    } while (searchPath.FindNext());

    fileNames.sort([](stdstr a, stdstr b) {
        return a.ToLower() < b.ToLower();
    });

    m_ScriptListView.SetRedraw(false);
    m_ScriptListView.DeleteAllItems();

    int nItem = 0;
    for (const stdstr& fileName : fileNames)
    {
        m_ScriptListView.AddItem(nItem, 0, fileName.ToUTF16().c_str());
        if (fileName == m_InitSelectedScriptName)
        {
            nSelectedItem = nItem;
            m_bScriptListNeedsRefocus = true;
            m_InitSelectedScriptName = "";
        }
        nItem++;
    }

    m_ScriptListView.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

    int itemCount = m_ScriptListView.GetItemCount();
    if (itemCount != 0 && nSelectedItem != -1)
    {
        m_ScriptListView.SelectItem(nSelectedItem >= itemCount ? itemCount - 1 : nSelectedItem);
    }

    if (m_bScriptListNeedsRefocus)
    {
        m_ScriptListView.SetFocus();
        m_bScriptListNeedsRefocus = false;
    }

    m_ScriptListView.SetRedraw(true);

    return 0;
}

LRESULT CScriptsAutorunDlg::OnRefreshAutorunList(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    int nSelectedItem = m_AutorunListView.GetSelectedIndex();

    strlist fileNames(m_ScriptSystem->AutorunList().begin(), m_ScriptSystem->AutorunList().end());
    fileNames.sort([](stdstr a, stdstr b) {
        return a.ToLower() < b.ToLower();
    });

    m_AutorunListView.SetRedraw(FALSE);
    m_AutorunListView.DeleteAllItems();

    int nItem = 0;
    for (const stdstr& fileName : fileNames)
    {
        m_AutorunListView.AddItem(nItem, 0, fileName.ToUTF16().c_str());
        if (fileName == m_InitSelectedScriptName)
        {
            nSelectedItem = nItem;
            m_bAutorunListNeedsRefocus = true;
            m_InitSelectedScriptName = "";
        }
        nItem++;
    }

    int itemCount = m_AutorunListView.GetItemCount();
    if (itemCount != 0 && nSelectedItem != -1)
    {
        m_AutorunListView.SelectItem(nSelectedItem >= itemCount ? itemCount - 1 : nSelectedItem);
    }

    if (m_bAutorunListNeedsRefocus)
    {
        m_AutorunListView.SetFocus();
        m_bAutorunListNeedsRefocus = false;
    }

    m_AutorunListView.SetRedraw(TRUE);

    return 0;
}

DWORD WINAPI CScriptsAutorunDlg::ScriptDirWatchProc(void* ctx)
{
    CScriptsAutorunDlg* _this = (CScriptsAutorunDlg*)ctx;

    stdstr scriptsDir = _this->m_ScriptSystem->ScriptsDirPath();

    HANDLE hEvents[2];
    hEvents[0] = FindFirstChangeNotification(scriptsDir.ToUTF16().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);

    if (hEvents[0] == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    hEvents[1] = _this->m_hQuitScriptDirWatchEvent;

    while (true)
    {
        DWORD status = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

        switch (status)
        {
        case WAIT_OBJECT_0:
            if (FindNextChangeNotification(hEvents[0]) == FALSE)
            {
                return 0;
            }
            _this->RefreshScriptList();
            break;
        case WAIT_OBJECT_0 + 1:
            return 0;
        default:
            return 0;
        }
    }
}

void CScriptsAutorunDlg::AddSelected()
{
    int nItem = m_ScriptListView.GetSelectedIndex();
    if (nItem == -1)
    {
        return;
    }

    wchar_t scriptName[MAX_PATH];
    m_ScriptListView.GetItemText(nItem, 0, scriptName, MAX_PATH);
    m_ScriptSystem->AutorunList().insert(stdstr().FromUTF16(scriptName));
    m_ScriptSystem->SaveAutorunList();

    RefreshAutorunList();
    RefreshScriptList();
}

void CScriptsAutorunDlg::RemoveSelected()
{
    int nItem = m_AutorunListView.GetSelectedIndex();
    if (nItem == -1)
    {
        return;
    }

    wchar_t scriptName[MAX_PATH];
    m_AutorunListView.GetItemText(nItem, 0, scriptName, MAX_PATH);
    m_ScriptSystem->AutorunList().erase(stdstr().FromUTF16(scriptName));
    m_ScriptSystem->SaveAutorunList();

    RefreshAutorunList();
    RefreshScriptList();
}

void CScriptsAutorunDlg::RefreshScriptList()
{
    if (m_hWnd != nullptr)
    {
        PostMessage(WM_REFRESH_LIST);
    }
}

void CScriptsAutorunDlg::RefreshAutorunList()
{
    if (m_hWnd != nullptr)
    {
        PostMessage(WM_REFRESH_AUTORUN_LIST);
    }
}
