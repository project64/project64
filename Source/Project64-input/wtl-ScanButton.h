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
    CScanButton(BUTTON & Button, int DisplayCtrlId, int ScanBtnId);

    void SubclassWindow(CWindow Wnd);

private:
    CScanButton(void);
    CScanButton(const CScanButton&);
    CScanButton& operator=(const CScanButton&);

    void DisplayButton(void);
    void OnScan(void);
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
};