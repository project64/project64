#include "wtl-ScanButton.h"
#include "CProject64Input.h"
#include <Common/StdString.h>
#include <time.h>

CScanButton::CScanButton(BUTTON & Button, int DisplayCtrlId, int ScanBtnId) :
    m_Button(Button),
    m_DisplayCtrlId(DisplayCtrlId),
    m_ScanBtnId(ScanBtnId),
    m_ScanBtnProc(nullptr),
    m_ScanCount(0),
    m_ScanStart(0),
    m_ChangeCallback(nullptr),
    m_ChangeCallbackData(0)
{
}

void CScanButton::SubclassWindow(CWindow Wnd)
{
    m_DisplayCtrl = Wnd.GetDlgItem(m_DisplayCtrlId);
    m_ScanBtn = Wnd.GetDlgItem(m_ScanBtnId);
    m_ScanBtnThunk.Init((WNDPROC)ScanButtonProc, this);
    m_ScanBtnProc = (WNDPROC)m_ScanBtn.SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)m_ScanBtnThunk.GetWNDPROC());
    DisplayButton();
}

void CScanButton::SetChangeCallback(ChangeCallback callback, size_t callbackdata)
{
    m_ChangeCallback = callback;
    m_ChangeCallbackData = callbackdata;
}

void CScanButton::DisplayButton(void)
{
    m_DisplayCtrl.SetWindowText(g_InputPlugin->ButtonAssignment(m_Button).c_str());
}

void CScanButton::DetectKey(void)
{
    enum
    {
        SACN_INTERVAL = 20
    };
    m_ScanCount = 0;
    time(&m_ScanStart);
    g_InputPlugin->StartScanDevices(m_DisplayCtrlId);
    m_DisplayCtrl.Invalidate();
    m_ScanBtn.SetTimer(DETECT_KEY_TIMER, SACN_INTERVAL, NULL);
    MakeOverlay();
}

void CScanButton::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == DETECT_KEY_TIMER)
    {
        BUTTON EmptyButton = { 0 };
        bool Stop = false, ScanSuccess = false;
        if (g_InputPlugin)
        {
            BUTTON Button = m_Button;
            CDirectInput::ScanResult Result = g_InputPlugin->ScanDevices(Button);
            if (Result == CDirectInput::SCAN_SUCCEED && (Button.Offset != m_Button.Offset || Button.AxisID != m_Button.AxisID || Button.BtnType != m_Button.BtnType))
            {
                m_ScanBtn.KillTimer(DETECT_KEY_TIMER);
                if (m_ChangeCallback != nullptr)
                {
                    m_ChangeCallback(m_ChangeCallbackData, Button);
                }
                m_Button = Button;
            }
            if (Result == CDirectInput::SCAN_ESCAPE && (EmptyButton.Offset != m_Button.Offset || EmptyButton.AxisID != m_Button.AxisID || EmptyButton.BtnType != m_Button.BtnType))
            {
                m_ScanBtn.KillTimer(DETECT_KEY_TIMER);
                if (m_ChangeCallback != nullptr)
                {
                    m_ChangeCallback(m_ChangeCallbackData, EmptyButton);
                }
                m_Button = EmptyButton;
            }
            if (Result == CDirectInput::SCAN_SUCCEED || Result == CDirectInput::SCAN_ESCAPE)
            {
                ScanSuccess = Result == CDirectInput::SCAN_SUCCEED;
                Stop = true;
                DisplayButton();
            }
        }
        if ((m_ScanCount % 30) == 0)
        {
            CWindow Dialog = m_ScanBtn.GetParent().GetParent();
            time_t Now = time(nullptr);
            if (10 - (Now - m_ScanStart) > 0)
            {
                Dialog.SetWindowText(stdstr_f("Configure input: Press key... (%d seconds)", 10 - (Now - m_ScanStart)).ToUTF16().c_str());
            }
            else
            {
                if (m_ChangeCallback != nullptr)
                {
                    m_ChangeCallback(m_ChangeCallbackData, EmptyButton);
                }
                m_Button = EmptyButton;
                DisplayButton();
                Stop = true;
            }
        }
        if (m_ScanCount > 500)
        {
            Stop = true;
        }
        else
        {
            m_ScanCount += 1;
        }

        if (Stop)
        {
            m_ScanBtn.KillTimer(DETECT_KEY_TIMER);
            CWindow Dialog = m_ScanBtn.GetParent().GetParent();
            Dialog.SetWindowText(L"Configure input");

            if (m_Overlay.m_hWnd != NULL)
            {
                m_Overlay.DestroyWindow();
                m_Overlay = NULL;
            }
 
            g_InputPlugin->EndScanDevices();
            m_DisplayCtrl.Invalidate();
            m_DisplayCtrl.GetParent().SendMessage(ScanSuccess ? WM_SCAN_SUCCESS : WM_SCAN_CANCELED);
        }
    }
}

void CScanButton::MakeOverlay(void)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = (WNDPROC)BlockerProc;
    wc.hInstance = g_InputPlugin->hInst();
    wc.lpszClassName = L"BlockerClass";
    RegisterClass(&wc);

    CWindow ControllerDlg = m_ScanBtn.GetParent().GetParent();
    CRect size;
    ControllerDlg.GetWindowRect(&size);
#ifndef _DEBUG
    m_Overlay = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT, L"BlockerClass", L"Blocker", WS_POPUP, size.left, size.top, size.Width(), size.Height(), ControllerDlg, nullptr, g_InputPlugin->hInst(), NULL);
    if (m_Overlay == NULL)
    {
        return;
    }
    m_Overlay.SetFocus();
    m_Overlay.ShowWindow(SW_SHOWNOACTIVATE);
#endif
}

UINT_PTR CALLBACK CScanButton::ScanButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CScanButton * _this = (CScanButton*)hWnd;
    UINT_PTR uRet = 0;
    if (uMsg == WM_LBUTTONUP)
    {
        POINT ptCursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        _this->m_ScanBtn.ClientToScreen(&ptCursor);
        
        RECT rect = { 0 };
        _this->m_ScanBtn.GetWindowRect(&rect);
        if (PtInRect(&rect, ptCursor))
        {
            _this->DetectKey();
        }
    }
    else if (uMsg == WM_TIMER)
    {
        _this->OnTimer((UINT_PTR)wParam);
    }
    if (_this->m_ScanBtnProc != nullptr)
    {
        uRet = _this->m_ScanBtnProc(_this->m_ScanBtn, uMsg, wParam, lParam);
    }
    return uRet;
}


UINT_PTR CALLBACK  CScanButton::BlockerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        return 0;
    }
    if (msg == WM_KEYDOWN || msg == WM_KEYUP)
    {
        return 0;
    }
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
