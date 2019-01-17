#pragma once

#include <stdafx.h>
#include "DebuggerUI.h"


class CDebugExcBreakpoints :
    public CDebugDialog<CDebugExcBreakpoints>
{
    typedef struct
    {
        WORD ctrlId;
        int exc;
    } ExcCheckboxMeta;

public:
    enum { IDD = IDD_Debugger_ExceptionBP };

    CDebugExcBreakpoints(CDebuggerUI * debugger);
    virtual ~CDebugExcBreakpoints(void);

private:
    static ExcCheckboxMeta ExcCheckboxMap[];

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
    
    void    OnExitSizeMove(void);
    LRESULT OnDestroy(void);

    BEGIN_MSG_MAP_EX(CDebugExcBreakpoints)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        MSG_WM_DESTROY(OnDestroy)
    END_MSG_MAP()
};
