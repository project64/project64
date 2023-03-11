#include "stdafx.h"

#include "DebuggerUI.h"

CDebugScripts::CDebugScripts(CDebuggerUI * debugger) :
    CDebugDialog<CDebugScripts>(debugger),
    CToolTipDialog<CDebugScripts>(),
    m_hQuitScriptDirWatchEvent(nullptr),
    m_hScriptDirWatchThread(nullptr),
    m_InputHistoryIndex(0),
    m_MonoFont(nullptr),
    m_MonoBoldFont(nullptr)
{
}

CDebugScripts::~CDebugScripts(void)
{
    for (size_t i = 0; i < m_InputHistory.size(); i++)
    {
        delete[] m_InputHistory[i];
    }
    m_InputHistory.clear();
}

LRESULT CDebugScripts::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_ScriptsPos);
    DlgToolTip_Init();

    CDC hDC = GetDC();
    float DPIScale = hDC.GetDeviceCaps(LOGPIXELSX) / 96.0f;

    m_MonoFont = CreateFont((int)(-12 * DPIScale), 0, 0, 0,
                            FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas");

    m_MonoBoldFont = CreateFont((int)(-13 * DPIScale), 0, 0, 0,
                                FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas");

    m_ScriptList.Attach(GetDlgItem(IDC_SCRIPT_LIST));
    m_ScriptList.AddColumn(L"Status", 0);
    m_ScriptList.AddColumn(L"Script", 1);
    m_ScriptList.SetColumnWidth(0, (int)(16 * DPIScale));
    m_ScriptList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
    m_ScriptList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_ScriptList.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);

    m_ConInputEdit.Attach(GetDlgItem(IDC_EVAL_EDIT));
    m_ConInputEdit.SetFont(m_MonoFont);
    m_ConInputEdit.EnableWindow(FALSE);

    m_ConOutputEdit.Attach(GetDlgItem(IDC_CONSOLE_EDIT));
    m_ConOutputEdit.SetLimitText(0);
    m_ConOutputEdit.SetFont(m_MonoFont);

    ::SendMessage(GetDlgItem(IDC_EVAL_LBL), WM_SETFONT, (WPARAM)m_MonoBoldFont, 0);

    int statusPaneWidths[] = {-1};
    m_StatusBar.Attach(GetDlgItem(IDC_STATUSBAR));
    m_StatusBar.SetParts(1, statusPaneWidths);

    m_Debugger->ScriptSystem()->LoadAutorunList();

    RefreshList();

    m_InstallDir = m_Debugger->ScriptSystem()->InstallDirPath();
    m_ScriptsDir = m_Debugger->ScriptSystem()->ScriptsDirPath();

    SetTimer(CONFLUSH_TIMER_ID, CONFLUSH_TIMER_INTERVAL, nullptr);

    m_hQuitScriptDirWatchEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_hScriptDirWatchThread = CreateThread(nullptr, 0, ScriptDirWatchProc, (void *)this, 0, nullptr);

    m_ConOutputEdit.SetWindowText(m_Debugger->ScriptSystem()->GetConsoleBuffer().ToUTF16().c_str());

    LoadWindowPos();
    WindowCreated();

    return 0;
}

LRESULT CDebugScripts::OnDestroy(void)
{
    KillTimer(CONFLUSH_TIMER_ID);

    SetEvent(m_hQuitScriptDirWatchEvent);
    WaitForSingleObject(m_hScriptDirWatchThread, INFINITE);
    CloseHandle(m_hQuitScriptDirWatchEvent);
    CloseHandle(m_hScriptDirWatchThread);

    DeleteObject(m_MonoFont);
    DeleteObject(m_MonoBoldFont);
    return 0;
}

LRESULT CDebugScripts::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/)
{
    HDC hDC = (HDC)wParam;
    HWND hCtrl = (HWND)lParam;
    WORD ctrlId = (WORD)::GetWindowLong(hCtrl, GWL_ID);

    if (ctrlId == IDC_CONSOLE_EDIT)
    {
        SetTextColor(hDC, RGB(0xEE, 0xEE, 0xEE));
        SetBkColor(hDC, RGB(0x22, 0x22, 0x22));
        SetDCBrushColor(hDC, RGB(0x22, 0x22, 0x22));
        return (LRESULT)GetStockObject(DC_BRUSH);
    }

    return FALSE;
}

LRESULT CDebugScripts::OnCtlColorEdit(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/)
{
    HDC hDC = (HDC)wParam;
    HWND hCtrl = (HWND)lParam;
    WORD ctrlId = (WORD)::GetWindowLong(hCtrl, GWL_ID);

    if (ctrlId == IDC_EVAL_EDIT)
    {
        SetTextColor(hDC, RGB(0xEE, 0xEE, 0xEE));
        SetBkColor(hDC, RGB(0x22, 0x22, 0x22));
        SetDCBrushColor(hDC, RGB(0x22, 0x22, 0x22));
        return (LRESULT)GetStockObject(DC_BRUSH);
    }

    return FALSE;
}

DWORD WINAPI CDebugScripts::ScriptDirWatchProc(void * ctx)
{
    CDebugScripts * _this = (CDebugScripts *)ctx;

    HANDLE hEvents[2];

    hEvents[0] = FindFirstChangeNotification(_this->m_ScriptsDir.ToUTF16().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);

    if (hEvents[0] == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    hEvents[1] = _this->m_hQuitScriptDirWatchEvent;

    while (true)
    {
        DWORD nHandle = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

        switch (nHandle)
        {
        case WAIT_OBJECT_0:
            if (FindNextChangeNotification(hEvents[0]) == FALSE)
            {
                goto done;
            }
            _this->PostMessage(WM_REFRESH_LIST, 0, 0);
            break;
        default:
        case WAIT_OBJECT_0 + 1:
            goto done;
        }
    }

done:
    FindCloseChangeNotification(hEvents[0]);
    return 0;
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

    size_t nChars = m_ConOutputEdit.GetWindowTextLength() + 1;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, nChars * sizeof(wchar_t));

    if (hMem == nullptr)
    {
        return;
    }

    wchar_t * memBuf = (wchar_t *)GlobalLock(hMem);

    if (memBuf == nullptr)
    {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
        return;
    }

    m_ConOutputEdit.GetWindowText(memBuf, (int)((INT_PTR)(nChars)));

    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);

    GlobalFree(hMem);
    CloseClipboard();
}

void CDebugScripts::ConsolePrint(const char * text)
{
    if (m_hWnd != nullptr)
    {
        // OnConsolePrint will free this
        char * textCopy = _strdup(text);
        PostMessage(WM_CONSOLE_PRINT, (WPARAM)textCopy);
    }
}

void CDebugScripts::ConsoleClear()
{
    if (m_hWnd != nullptr)
    {
        PostMessage(WM_CONSOLE_CLEAR);
    }
}

void CDebugScripts::RefreshList()
{
    if (m_hWnd != nullptr)
    {
        PostMessage(WM_REFRESH_LIST);
    }
}

LRESULT CDebugScripts::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case ID_POPUP_RUN:
        RunSelected();
        break;
    case IDC_RUN_BTN:
        ToggleSelected();
        break;
    case ID_POPUP_STOP:
        StopSelected();
        break;
    case ID_POPUP_SCRIPT_EDIT:
        EditSelected();
        break;
    case ID_POPUP_AUTORUN:
        m_AutorunDlg.DoModal(m_Debugger, m_SelectedScriptName);
        m_ScriptList.RedrawWindow();
        break;
    case IDC_CLEAR_BTN:
        m_Debugger->ScriptSystem()->ConsoleClear();
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

LRESULT CDebugScripts::OnScriptListDblClicked(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);
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
    JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(m_SelectedScriptName.c_str());

    stdstr statusText = m_ScriptsDir + m_SelectedScriptName;

    if (status == JS_STATUS_STARTED)
    {
        statusText += " (Started)";
        m_ConInputEdit.EnableWindow(TRUE);
        m_ConInputEdit.SetFocus();
    }
    else
    {
        if (status == JS_STATUS_STARTING)
        {
            statusText += " (Starting)";
        }
        m_ConInputEdit.EnableWindow(FALSE);
    }

    m_StatusBar.SetText(0, statusText.ToUTF16().c_str());
}

LRESULT CDebugScripts::OnScriptListRClicked(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);
    int nItem = pIA->iItem;

    if (nItem == -1)
    {
        return 0;
    }

    JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(m_SelectedScriptName.c_str());

    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_SCRIPT_POPUP));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    if (status == JS_STATUS_STARTING || status == JS_STATUS_STARTED)
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

LRESULT CDebugScripts::OnScriptListCustomDraw(NMHDR * pNMHDR)
{
    NMLVCUSTOMDRAW * pLVCD = reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);
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

    DWORD nItem = (DWORD)pLVCD->nmcd.dwItemSpec;

    wchar_t scriptName[MAX_PATH];
    m_ScriptList.GetItemText(nItem, 1, scriptName, MAX_PATH);

    if (m_Debugger->ScriptSystem()->AutorunList().count(stdstr().FromUTF16(scriptName)) > 0 && pLVCD->iSubItem == 1)
    {
        pLVCD->clrText = RGB(0x00, 0x80, 0x00);
    }

    JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(stdstr("").FromUTF16(scriptName).c_str());

    if (status == JS_STATUS_STARTING)
    {
        pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xAA);
    }
    else if (status == JS_STATUS_STARTED)
    {
        pLVCD->clrTextBk = RGB(0xAA, 0xFF, 0xAA);
    }

    return CDRF_DODEFAULT;
}

LRESULT CDebugScripts::OnScriptListItemChanged(NMHDR * pNMHDR)
{
    NMLISTVIEW * lpStateChange = reinterpret_cast<NMLISTVIEW *>(pNMHDR);
    if ((lpStateChange->uNewState ^ lpStateChange->uOldState) & LVIS_SELECTED)
    {
        if (lpStateChange->iItem == -1)
        {
            return FALSE;
        }

        wchar_t ScriptName[MAX_PATH];

        m_ScriptList.GetItemText(lpStateChange->iItem, 1, ScriptName, MAX_PATH);
        m_SelectedScriptName = stdstr().FromUTF16(ScriptName).c_str();

        JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(m_SelectedScriptName.c_str());

        ::SetWindowText(GetDlgItem(IDC_RUN_BTN), status == JS_STATUS_STOPPED ? L"Run" : L"Stop");

        RefreshStatus();
    }
    return FALSE;
}

LRESULT CDebugScripts::OnConsolePrint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    char * text = (char *)wParam;
    m_ConOutputBuffer += text;
    free(text);
    return FALSE;
}

LRESULT CDebugScripts::OnConsoleClear(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    m_ConOutputBuffer = "";
    m_ConOutputEdit.SetWindowText(L"");
    return FALSE;
}

LRESULT CDebugScripts::OnRefreshList(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    int nIndex = m_ScriptList.GetSelectedIndex();
    CPath searchPath(m_ScriptsDir.c_str(), "*");
    strlist fileNames;

    if (!searchPath.FindFirst(CPath::FIND_ATTRIBUTE_FILES))
    {
        m_ScriptList.DeleteAllItems();
        return FALSE;
    }

    do
    {
        if (searchPath.GetExtension() == "js")
        {
            fileNames.push_back(searchPath.GetNameExtension());
        }
    } while (searchPath.FindNext());

    fileNames.sort([](stdstr a, stdstr b) { return a.ToLower() < b.ToLower(); });

    m_ScriptList.SetRedraw(false);
    m_ScriptList.DeleteAllItems();

    int nItem = 0;
    for (const stdstr & fileName : fileNames)
    {
        JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(fileName.c_str());
        const wchar_t * statusIcon = L"";

        switch (status)
        {
        case JS_STATUS_STARTING:
            statusIcon = L"*";
            break;
        case JS_STATUS_STARTED:
            statusIcon = L">";
            break;
        default:
            statusIcon = L"-";
            break;
        }

        m_ScriptList.AddItem(nItem, 0, statusIcon);
        m_ScriptList.SetItemText(nItem, 1, fileName.ToUTF16().c_str());
        nItem++;
    }

    m_ScriptList.SetRedraw(true);
    m_ScriptList.Invalidate();

    if (nIndex >= 0)
    {
        m_ScriptList.SelectItem(nIndex);
        RefreshStatus();
    }
    return FALSE;
}

void CDebugScripts::SendInput(const char * name, const char * code)
{
    m_Debugger->ScriptSystem()->Input(name, code);
}

void CDebugScripts::RunSelected()
{
    if (m_SelectedScriptName.empty())
    {
        return;
    }

    stdstr path = m_ScriptsDir + m_SelectedScriptName;

    m_Debugger->ScriptSystem()->StartScript(m_SelectedScriptName.c_str(), path.c_str());
}

void CDebugScripts::StopSelected()
{
    m_Debugger->ScriptSystem()->StopScript(m_SelectedScriptName.c_str());
}

void CDebugScripts::ToggleSelected()
{
    JSInstanceStatus status = m_Debugger->ScriptSystem()->GetStatus(m_SelectedScriptName.c_str());

    if (status == JS_STATUS_STOPPED)
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
    stdstr scriptPath = m_ScriptsDir + m_SelectedScriptName;
    ShellExecuteA(nullptr, "edit", scriptPath.c_str(), nullptr, m_InstallDir.c_str(), SW_SHOWNORMAL);
}

LRESULT CDebugScripts::OnInputSpecialKey(NMHDR * pNMHDR)
{
    NMCISPECIALKEY * pnmsk = (NMCISPECIALKEY *)pNMHDR;

    if (pnmsk->vkey == VK_UP)
    {
        if (m_InputHistoryIndex > 0)
        {
            wchar_t * code = m_InputHistory[--m_InputHistoryIndex];
            m_ConInputEdit.SetWindowText(code);
            int selEnd = (int)((INT_PTR)wcslen(code));
            m_ConInputEdit.SetSel(selEnd, selEnd);
        }

        return 0;
    }

    if (pnmsk->vkey == VK_DOWN)
    {
        size_t size = m_InputHistory.size();

        if (m_InputHistoryIndex < size)
        {
            m_InputHistoryIndex++;
        }

        if (m_InputHistoryIndex < size)
        {
            wchar_t * code = m_InputHistory[m_InputHistoryIndex];
            m_ConInputEdit.SetWindowText(code);
            int selEnd = (int)((INT_PTR)(wcslen(code)));
            m_ConInputEdit.SetSel(selEnd, selEnd);
        }
        else
        {
            m_ConInputEdit.SetWindowText(L"");
        }

        return 0;
    }

    if (pnmsk->vkey == VK_RETURN)
    {
        size_t codeLength = m_ConInputEdit.GetWindowTextLength();

        if (codeLength == 0)
        {
            return 0;
        }

        wchar_t * code = new wchar_t[codeLength + 1];
        m_ConInputEdit.GetWindowText(code, (int)((INT_PTR)(codeLength + 1)));
        m_ConInputEdit.SetWindowText(L"");

        SendInput(m_SelectedScriptName.c_str(), stdstr().FromUTF16(code).c_str());

        // If there is a duplicate entry move it to the bottom
        for (size_t i = 0; i < m_InputHistory.size(); i++)
        {
            if (wcscmp(code, m_InputHistory[i]) == 0)
            {
                wchar_t * str = m_InputHistory[i];
                m_InputHistory.erase(m_InputHistory.begin() + i);
                m_InputHistory.push_back(str);
                m_InputHistoryIndex = m_InputHistory.size();
                delete[] code;
                return 0;
            }
        }

        m_InputHistory.push_back(code);
        m_InputHistoryIndex = m_InputHistory.size();
        return 0;
    }

    return 0;
}

void CDebugScripts::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == CONFLUSH_TIMER_ID)
    {
        if (m_ConOutputBuffer == "")
        {
            return;
        }

        SCROLLINFO scroll;
        scroll.cbSize = sizeof(SCROLLINFO);
        scroll.fMask = SIF_ALL;
        m_ConOutputEdit.GetScrollInfo(SB_VERT, &scroll);

        m_ConOutputEdit.SetRedraw(FALSE);
        m_ConOutputEdit.AppendText(m_ConOutputBuffer.ToUTF16().c_str());
        m_ConOutputEdit.SetRedraw(TRUE);

        if ((scroll.nPage + scroll.nPos) - 1 == (uint32_t)scroll.nMax)
        {
            m_ConOutputEdit.ScrollCaret();
        }

        m_ConOutputBuffer = "";
    }
}
