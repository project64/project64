#pragma once
#include "wtl.h"
#include "Button.h"

class CScanButton
{
    enum
    {
        DETECT_KEY_TIMER = 1
    }; 

public:
    enum
    {
        WM_SCAN_SUCCESS = WM_USER + 0x140,
        WM_SCAN_CANCELED = WM_USER + 0x141,
    };
    typedef void(*ChangeCallback)(size_t Data, const BUTTON & Button);

    CScanButton(BUTTON & Button, int DisplayCtrlId, int ScanBtnId);

    void SubclassWindow(CWindow Wnd);
    void SetChangeCallback(ChangeCallback callback, size_t callbackdata);
    void DetectKey(void);
    void DisplayButton(void);

private:
    CScanButton(void);
    CScanButton(const CScanButton&);
    CScanButton& operator=(const CScanButton&);

    void OnTimer(UINT_PTR nIDEvent);
    void MakeOverlay(void);
    static UINT_PTR CALLBACK ScanButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT_PTR CALLBACK BlockerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    BUTTON & m_Button;
    int32_t m_DisplayCtrlId, m_ScanBtnId;
    CWindow m_DisplayCtrl, m_ScanBtn;
    CWndProcThunk m_ScanBtnThunk;
    WNDPROC m_ScanBtnProc;
    uint32_t m_ScanCount;
    time_t m_ScanStart;
    CWindow m_Overlay;
    ChangeCallback m_ChangeCallback;
    size_t m_ChangeCallbackData;
};