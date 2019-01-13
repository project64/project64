#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
template <class T>
class CDebugDialog :
    public CDialogImpl < T >
{
protected:
    CDebuggerUI * m_Debugger;
    HANDLE        m_CreatedEvent;
    HANDLE        m_DialogThread;

    static DWORD CreateDebuggerWindow(CDebugDialog<T> * pThis)
    {
        pThis->DoModal(NULL);
        pThis->WindowCreated();
        return 0;
    }

    void WindowCreated(void)
    {
        SetEvent(m_CreatedEvent);
    }

public:
    CDebugDialog(CDebuggerUI * debugger) :
        m_Debugger(debugger),
        m_CreatedEvent(CreateEvent(NULL, true, false, NULL)),
        m_DialogThread(NULL)
    {
    }
    virtual ~CDebugDialog(void)
    {
        HideWindow();
        CloseHandle(m_CreatedEvent);
        if (m_DialogThread)
        {
            CloseHandle(m_DialogThread);
            m_DialogThread = NULL;
        }
    }

    enum { Timer_SetWindowPos = 1 };

    //Get Information about the window
    int GetHeight(void) {
        if (!m_hWnd) { return 0; }

        RECT rect;
        GetWindowRect(m_hWnd, &rect);
        return rect.bottom - rect.top;
    }

    int GetWidth(void) {
        if (!m_hWnd) { return 0; }

        RECT rect;
        GetWindowRect(m_hWnd, &rect);
        return rect.right - rect.left;
    }

    int GetX(CRect WinRect) {
        return (GetSystemMetrics(SM_CXSCREEN) - (WinRect.right - WinRect.left)) / 2;
    }

    int GetY(CRect WinRect) {
        return (GetSystemMetrics(SM_CYSCREEN) - (WinRect.bottom - WinRect.top)) / 2;
    }

    //Manipulate the state of the window
    void SetPos(int X, int Y) { //Move the window to this screen location
        ::SetWindowPos(m_hWnd, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    void SaveWindowLoc(UISettingID SettingID_Top, UISettingID SettingID_Left) {
        RECT WinRect;
        ::GetWindowRect(m_hWnd, &WinRect);

        //save the location of the window
        if (m_hWnd)
        {
            m_SaveWnd = true;
            m_SaveWndTop = WinRect.top;
            m_SaveWndLeft = WinRect.left;
        }

        ::KillTimer(m_hWnd, Timer_SetWindowPos);
        ::SetTimer(m_hWnd, Timer_SetWindowPos, 1000, NULL);

        bool flush = false;
        if (m_SaveWnd)
        {
            m_SaveWnd = false;
            UISettingsSaveDword(SettingID_Top, m_SaveWndTop);
            UISettingsSaveDword(SettingID_Left, m_SaveWndLeft);
            flush = true;
        }

        if (flush)
        {
            CSettingTypeApplication::Flush();
        }
    }

    void SetSize(int Width, int Height) { //Set window Height and Width
        RECT rcClient;
        rcClient.top = 0;
        rcClient.bottom = Height;
        rcClient.left = 0;
        rcClient.right = Width;
        ::AdjustWindowRect(&rcClient, ::GetWindowLong(m_hWnd, GWL_STYLE), true);

        int32_t WindowHeight = rcClient.bottom - rcClient.top;
        int32_t WindowWidth = rcClient.right - rcClient.left;

        ::SetWindowPos(m_hWnd, NULL, 0, 0, WindowWidth, WindowHeight, SWP_NOMOVE | SWP_NOZORDER);
    }

    void SaveSize(UISettingID SettingID_X, UISettingID SettingID_Y) {
        //Get the current window size
        RECT rect;
        GetWindowRect(m_hWnd, &rect);

        int32_t WindowHeight = rect.bottom - rect.top;
        int32_t WindowWidth = rect.right - rect.left;

        if (UISettingsLoadDword(SettingID_X) != WindowWidth)
        {
            UISettingsSaveDword(SettingID_X, WindowWidth);
        }
        if (UISettingsLoadDword(SettingID_Y) != WindowHeight)
        {
            UISettingsSaveDword(SettingID_Y, WindowHeight);
        }
    }

    void HideWindow(void)
    {
        if (m_hWnd && ::IsWindow(m_hWnd))
        {
            ::EndDialog(m_hWnd, 0);
        }
        if (m_DialogThread)
        {
            if (WaitForSingleObject(m_DialogThread, 5000) == WAIT_TIMEOUT)
            {
                WriteTrace(TraceUserInterface, TraceError, "CDebugDialog - time out on close");

                TerminateThread(m_DialogThread, 1);
            }
            CloseHandle(m_DialogThread);
            m_DialogThread = NULL;
        }
    }

    void ShowWindow(void)
    {
        if (m_hWnd == NULL)
        {
            DWORD ThreadID;
            ResetEvent(m_CreatedEvent);
            m_DialogThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateDebuggerWindow, (LPVOID)this, 0, &ThreadID);
            if (WaitForSingleObject(m_CreatedEvent, 20000) == WAIT_TIMEOUT)
            {
                WriteTrace(TraceUserInterface, TraceError, "Failed to get window create notification");
            }
        }
        if (m_hWnd)
        {
            if (::IsIconic((HWND)m_hWnd)) {
                SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, NULL);
            }
            SetForegroundWindow((HWND)m_hWnd);
        }
    }
private:
    bool        m_SaveWnd;
    LONG        m_SaveWndTop;
    LONG        m_SaveWndLeft;
};
