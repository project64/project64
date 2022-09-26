#pragma once
#include "resource.h"
#include <Project64\UserInterface\WTLControls\wtl-BitmapPicture.h>

class CSupportWindow :
    public CDialogImpl<CSupportWindow>
{
public:
    BEGIN_MSG_MAP_EX(CSettingConfig)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic);
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground);
        MESSAGE_HANDLER(WM_TIMER, OnTimer);
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnCloseCmd);
        COMMAND_ID_HANDLER(ID_SUPPORT_PJ64, OnSupportProject64);
        COMMAND_ID_HANDLER(IDC_ENTER_CODE, OnEnterCode);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Support_Project64
    };

    CSupportWindow(CProjectSupport & Support);
    ~CSupportWindow(void);

    void Show(HWND hParent, bool Delay);

private:
    CSupportWindow();
    CSupportWindow(const CSupportWindow &);
    CSupportWindow & operator=(const CSupportWindow &);

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & bHandled);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnSupportProject64(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnEnterCode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/);

    void EnableContinue();

    CHyperLink m_EnterLink;

    static void CALLBACK TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD);

    CProjectSupport & m_Support;
    CBitmapPicture m_Logo;
    bool m_Delay;
    uint32_t m_TimeOutTime;
    HWND m_hParent;
    static CSupportWindow * m_this;
};
