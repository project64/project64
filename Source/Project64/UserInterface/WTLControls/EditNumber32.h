#pragma once

#include <wtl/atlctrls.h>
#include "DisplayMode.h"

class CEditNumber32 :
    public CWindowImpl<CEditNumber32, CEdit>
{
public:
    enum DisplayType
    {
        DisplayHex,
        DisplayDec,
    };

    CEditNumber32(void);
    virtual ~CEditNumber32(void);

    BOOL Attach(HWND hWndNew);
    BOOL AttachToDlgItem(HWND parent, UINT dlgID);
    void SetDisplayType(DisplayType Type);
    uint32_t GetValue(void);
    stdstr GetValueText(void);
    void SetValue(uint32_t Value, DisplayMode Display = DisplayMode::ShowHexIdent);

protected:
    enum
    {
        WM_VALIDATE_VALUE = WM_USER + 0x97
    };

    DisplayType  m_DisplayType;

    BEGIN_MSG_MAP(CEditNumber32)
        MESSAGE_HANDLER(WM_CHAR, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_PASTE, OnPaste)
        MESSAGE_HANDLER(WM_VALIDATE_VALUE, OnValidateValue);
    END_MSG_MAP()

    bool IsHexConvertableText(LPTSTR _text);
    void FormatClipboard();

    LRESULT OnValidateValue(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaste(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
