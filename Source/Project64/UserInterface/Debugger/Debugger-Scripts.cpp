#include "stdafx.h"

#include "DebuggerUI.h"

CDebugScripts::CDebugScripts(CDebuggerUI* debugger) :
    CDebugDialog<CDebugScripts>(debugger),
    CToolTipDialog<CDebugScripts>(),
    m_hQuitScriptDirWatchEvent(nullptr),
    m_hScriptDirWatchThread(nullptr)
{
}

CDebugScripts::~CDebugScripts(void)
{
}

LRESULT CDebugScripts::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_ScriptsPos);
    DlgToolTip_Init();

    HFONT monoFont = CreateFont(-11, 0, 0, 0,
        FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas"
    );

    m_ScriptList.Attach(GetDlgItem(IDC_SCRIPT_LIST));
    m_ScriptList.AddColumn(L"Status", 0);
    m_ScriptList.AddColumn(L"Script", 1);
    m_ScriptList.SetColumnWidth(0, 16);
    m_ScriptList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
    m_ScriptList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_ScriptList.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);

    m_EvalEdit.Attach(GetDlgItem(IDC_EVAL_EDIT));
    m_EvalEdit.SetScriptWindow(this);
    m_EvalEdit.SetFont(monoFont);
    m_EvalEdit.EnableWindow(FALSE);

    m_ConsoleEdit.Attach(GetDlgItem(IDC_CONSOLE_EDIT));
    m_ConsoleEdit.SetLimitText(0);
    m_ConsoleEdit.SetFont(monoFont);

    int statusPaneWidths[] = { -1 };
    m_StatusBar.Attach(GetDlgItem(IDC_STATUSBAR));
    m_StatusBar.SetParts(1, statusPaneWidths);

    RefreshList();

    m_InstallDir = (std::string)CPath(CPath::MODULE_DIRECTORY);
    m_ScriptsDir = m_InstallDir + "Scripts\\";

    if (!PathFileExistsA(m_ScriptsDir.c_str()))
    {
        CreateDirectoryA(m_ScriptsDir.c_str(), nullptr);
    }

    m_hQuitScriptDirWatchEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_hScriptDirWatchThread = CreateThread(nullptr, 0, ScriptDirWatchProc, (void*)this, 0, nullptr);

    LoadWindowPos();
    WindowCreated();

    return 0;
}

LRESULT CDebugScripts::OnDestroy(void)
{
    SetEvent(m_hQuitScriptDirWatchEvent);
    WaitForSingleObject(m_hScriptDirWatchThread, INFINITE);
    CloseHandle(m_hQuitScriptDirWatchEvent);
    CloseHandle(m_hScriptDirWatchThread);
    return 0;
}

LRESULT CDebugScripts::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    HDC hDC = (HDC)wParam;
    HWND hCtrl = (HWND)lParam;
    WORD ctrlId = (WORD) ::GetWindowLong(hCtrl, GWL_ID);

    if (ctrlId == IDC_CONSOLE_EDIT)
    {
        SetBkColor(hDC, RGB(255, 255, 255));
        SetDCBrushColor(hDC, RGB(255, 255, 255));
        return (LRESULT)GetStockObject(DC_BRUSH);
    }

    return FALSE;
}

DWORD WINAPI CDebugScripts::ScriptDirWatchProc(void* ctx)
{
    CDebugScripts* _this = (CDebugScripts*)ctx;

    HANDLE hEvents[2];

    hEvents[0] = FindFirstChangeNotification(_this->m_ScriptsDir.ToUTF16().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);

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
            _this->PostMessage(WM_REFRESH_LIST, 0, 0);
            break;
        case WAIT_OBJECT_0 + 1:
            return 0;
        default:
            return 0;
        }
    }
}

void CDebugScripts::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

void CDebugScripts::ConsoleCopy()
{
    if (!OpenClipboard())
    {
        return;
    }

    EmptyClipboard();

    size_t nChars = m_ConsoleEdit.GetWindowTextLength() + 1;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, nChars * sizeof(wchar_t));

    wchar_t* memBuf = (wchar_t*)GlobalLock(hMem);
    m_ConsoleEdit.GetWindowText(memBuf, nChars);

    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);

    GlobalFree(hMem);
    CloseClipboard();
}

void CDebugScripts::ConsolePrint(const char* text)
{
    if (m_hWnd != nullptr)
    {
        SendMessage(WM_CONSOLE_PRINT, (WPARAM)text);
    }
}

void CDebugScripts::ConsoleClear()
{
    if (m_hWnd != nullptr)
    {
        SendMessage(WM_CONSOLE_CLEAR);
    }
}

void CDebugScripts::RefreshList()
{
    if (m_hWnd != nullptr)
    {
        PostMessage(WM_REFRESH_LIST);
    }
}

LRESULT CDebugScripts::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case ID_POPUP_RUN:
    case IDC_RUN_BTN:
        RunSelected();
        break;
    case ID_POPUP_STOP:
    case IDC_STOP_BTN:
        StopSelected();
        break;
    case ID_POPUP_SCRIPT_EDIT:
        EditSelected();
        break;
    case IDC_CLEAR_BTN:
        ConsoleClear();
        break;
    case IDC_COPY_BTN:
        ConsoleCopy();
        break;
    case IDC_SCRIPTDIR_BTN:
        ShellExecuteA(nullptr, "open", m_ScriptsDir.c_str(), nullptr, m_InstallDir.c_str(), SW_SHOW);
        break;
    }
    return FALSE;
}

LRESULT CDebugScripts::OnScriptListDblClicked(NMHDR* pNMHDR)
{
    // Run script on double click
    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    if (nItem == -1)
    {
        return 0;
    }

    ToggleSelected();

    return 0;
}

void CDebugScripts::RefreshStatus()
{
    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

    stdstr statusText = m_ScriptsDir + m_SelectedScriptName;

    if (state == STATE_RUNNING)
    {
        statusText += " (Running)";
        m_EvalEdit.EnableWindow(TRUE);
    }
    else
    {
        if (state == STATE_STARTED)
        {
            statusText += " (Started)";
        }
        m_EvalEdit.EnableWindow(FALSE);
    }

    m_StatusBar.SetText(0, statusText.ToUTF16().c_str());
}

LRESULT CDebugScripts::OnScriptListRClicked(NMHDR* pNMHDR)
{
    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    if (nItem == -1)
    {
        return 0;
    }

    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_SCRIPT_POPUP));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    if (state == STATE_STARTED || state == STATE_RUNNING)
    {
        EnableMenuItem(hPopupMenu, ID_POPUP_RUN, MF_DISABLED | MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hPopupMenu, ID_POPUP_STOP, MF_DISABLED | MF_GRAYED);
    }
    
    POINT mouse;
    GetCursorPos(&mouse);
    TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);

    return 0;
}

LRESULT CDebugScripts::OnScriptListCustomDraw(NMHDR* pNMHDR)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    DWORD drawStage = pLVCD->nmcd.dwDrawStage;

    switch (drawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        return CDRF_NOTIFYSUBITEMDRAW;
    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
        break;
    default:
        return CDRF_DODEFAULT;
    }

    DWORD nItem = pLVCD->nmcd.dwItemSpec;

    wchar_t scriptName[MAX_PATH];
    m_ScriptList.GetItemText(nItem, 1, scriptName, MAX_PATH);

    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(stdstr("").FromUTF16(scriptName).c_str());

    if (state == STATE_STARTED)
    {
        pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xAA);
    }
    else if (state == STATE_RUNNING)
    {
        pLVCD->clrTextBk = RGB(0xAA, 0xFF, 0xAA);
    }

    return CDRF_DODEFAULT;
}

LRESULT CDebugScripts::OnScriptListItemChanged(NMHDR* pNMHDR)
{
    NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    if ((lpStateChange->uNewState ^  lpStateChange->uOldState) & LVIS_SELECTED)
    {
        if (lpStateChange->iItem == -1)
        {
            return FALSE;
        }

        wchar_t ScriptName[MAX_PATH];

        m_ScriptList.GetItemText(lpStateChange->iItem, 1, ScriptName, MAX_PATH);
        m_SelectedScriptName = stdstr().FromUTF16(ScriptName).c_str();

        INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

        ::EnableWindow(GetDlgItem(IDC_STOP_BTN), state == STATE_RUNNING || state == STATE_STARTED);
        ::EnableWindow(GetDlgItem(IDC_RUN_BTN), state == STATE_STOPPED || state == STATE_INVALID);

        RefreshStatus();
    }
    return FALSE;
}

LRESULT CDebugScripts::OnConsoleLog(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    const char *text = (const char*)wParam;

    ::ShowWindow(*this, SW_SHOWNOACTIVATE);

    SCROLLINFO scroll;
    scroll.cbSize = sizeof(SCROLLINFO);
    scroll.fMask = SIF_ALL;
    m_ConsoleEdit.GetScrollInfo(SB_VERT, &scroll);

    m_ConsoleEdit.SetRedraw(FALSE);
    m_ConsoleEdit.AppendText(stdstr(text).ToUTF16().c_str());
    m_ConsoleEdit.SetRedraw(TRUE);

    if ((scroll.nPage + scroll.nPos) - 1 == (uint32_t)scroll.nMax)
    {
        m_ConsoleEdit.ScrollCaret();
    }
    return FALSE;
}

LRESULT CDebugScripts::OnConsoleClear(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_ConsoleEdit.SetWindowText(L"");
    return FALSE;
}

LRESULT CDebugScripts::OnRefreshList(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    int nIndex = m_ScriptList.GetSelectedIndex();

    CPath SearchPath(m_ScriptsDir, "*");

    if (!SearchPath.FindFirst(CPath::FIND_ATTRIBUTE_ALLFILES))
    {
        return FALSE;
    }

    m_ScriptList.SetRedraw(false);
    m_ScriptList.DeleteAllItems();

    size_t nItem = 0;

    do
    {
        stdstr scriptFileName = SearchPath.GetNameExtension();
        INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(scriptFileName.c_str());
        const wchar_t *statusIcon = L"";

        switch (state)
        {
        case STATE_STARTED:
            statusIcon = L"*";
            break;
        case STATE_RUNNING:
            statusIcon = L">";
            break;
        default:
            statusIcon = L"-";
            break;
        }

        m_ScriptList.AddItem(nItem, 0, statusIcon);
        m_ScriptList.SetItemText(nItem, 1, scriptFileName.ToUTF16().c_str());
        nItem++;
    } while (SearchPath.FindNext());

    m_ScriptList.SetRedraw(true);
    m_ScriptList.Invalidate();

    if (nIndex >= 0)
    {
        m_ScriptList.SelectItem(nIndex);
        RefreshStatus();
    }
    return FALSE;
}

void CDebugScripts::EvaluateInSelectedInstance(const char* code)
{
    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

    if (state == STATE_RUNNING || state == STATE_STARTED)
    {
        CScriptInstance* instance = m_Debugger->ScriptSystem()->GetInstance(m_SelectedScriptName.c_str());
        instance->Eval(code);
    }
}

void CDebugScripts::RunSelected()
{
    if (m_SelectedScriptName.empty())
    {
        return;
    }

    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

    if (state == STATE_INVALID || state == STATE_STOPPED)
    {
        m_Debugger->ScriptSystem()->RunScript(m_SelectedScriptName.c_str());
    }
    else
    {
        m_Debugger->Debug_LogScriptsWindow("[Error: script is already running]\n");
    }
}

void CDebugScripts::StopSelected()
{
    m_Debugger->ScriptSystem()->StopScript(m_SelectedScriptName.c_str());
}

void CDebugScripts::ToggleSelected()
{
    INSTANCE_STATE state = m_Debugger->ScriptSystem()->GetInstanceState(m_SelectedScriptName.c_str());

    if (state == STATE_INVALID || state == STATE_STOPPED)
    {
        RunSelected();
    }
    else
    {
        StopSelected();
    }
}

void CDebugScripts::EditSelected()
{
    ShellExecuteA(nullptr, "edit", m_SelectedScriptName.c_str(), nullptr, m_ScriptsDir.c_str(), SW_SHOWNORMAL);
}

// Console input
LRESULT CEditEval::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (wParam == VK_UP)
    {
        if (m_HistoryIdx > 0)
        {
            const std::string & Code = m_History[--m_HistoryIdx];
            SetWindowText(stdstr(Code).ToUTF16().c_str());
            SetSel((int)Code.length(), (int)Code.length());
        }
    }
    else if (wParam == VK_DOWN)
    {
        int size = m_History.size();
        if (m_HistoryIdx < size - 1)
        {
            const std::string & Code = m_History[++m_HistoryIdx];
            SetWindowText(stdstr(Code).ToUTF16().c_str());
            SetSel((int)Code.length(), (int)Code.length());
        }
        else if (m_HistoryIdx < size)
        {
            SetWindowText(L"");
            m_HistoryIdx++;
        }
    }
    else if (wParam == VK_RETURN)
    {
        if (m_ScriptWindow == nullptr)
        {
            bHandled = FALSE;
            return 0;
        }

        std::string Code = GetCWindowText(*this);
        m_ScriptWindow->EvaluateInSelectedInstance(Code.c_str());

        SetWindowText(L"");
        int historySize = m_History.size();

        // Remove duplicate
        for (int i = 0; i < historySize; i++)
        {
            if (strcmp(Code.c_str(), m_History[i].c_str()) == 0)
            {
                m_History.erase(m_History.begin() + i);
                historySize--;
                break;
            }
        }

        // Remove oldest if maxed
        if (historySize >= HISTORY_MAX_ENTRIES)
        {
            m_History.erase(m_History.begin() + 0);
            historySize--;
        }

        m_History.push_back(Code);
        m_HistoryIdx = historySize++;
    }
    bHandled = FALSE;
    return 0;
}
