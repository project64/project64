#pragma once

#include "ListTypes.h"

class CListEdit : public CWindowImpl<CListEdit, CEdit>
{
public:
    CListEdit()
    {
        m_nItem = NULL_ITEM;
        m_nSubItem = NULL_SUBITEM;
        m_nFlags = ITEM_FLAGS_NONE;
        m_nExitChar = 0;
    }

    ~CListEdit()
    {
    }

protected:
    int m_nItem;
    int m_nSubItem;
    UINT m_nFlags;
    int m_nMaxLen;
    TCHAR m_nExitChar;
    CFont m_fntEditFont;

public:
    BOOL Create(HWND hWndParent, int nItem, int nSubItem, CRect & rcRect, UINT nFlags, LPCTSTR lpszItemText, UINT nMaxLen)
    {
        m_nItem = nItem;
        m_nSubItem = nSubItem;
        m_nFlags = nFlags;
        m_nExitChar = 0;
        m_nMaxLen = nMaxLen;

        // Destroy old edit control
        if (IsWindow())
            DestroyWindow();

        DWORD dwStyle = WS_CHILD | ES_AUTOHSCROLL | WS_BORDER;

        // Right-justify numbers
        if (nFlags & (ITEM_FLAGS_EDIT_NUMBER | ITEM_FLAGS_EDIT_FLOAT))
            dwStyle |= ES_RIGHT;

        if (nFlags & ITEM_FLAGS_EDIT_UPPER)
            dwStyle |= ES_UPPERCASE;

        // Create edit control
        CRect Area(rcRect.left - 2, rcRect.top - 3, rcRect.right + 3, rcRect.bottom + 2);
        if (CWindowImpl<CListEdit, CEdit>::Create(hWndParent, Area, nullptr, dwStyle) == nullptr)
            return FALSE;

        // Get system message font
        CLogFont logFont;
        logFont.SetMessageBoxFont();
        if (!m_fntEditFont.IsNull())
            m_fntEditFont.DeleteObject();
        if (m_fntEditFont.CreateFontIndirect(&logFont) == nullptr)
            return FALSE;

        SetFont(m_fntEditFont);
        SetMargins(ITEM_EDIT_MARGIN, ITEM_EDIT_MARGIN);
        SetWindowText(lpszItemText);

        // Show edit control
        ShowWindow(SW_SHOW);

        SetSelAll();
        SetFocus();

        return TRUE;
    }

    BOOL IsValid(TCHAR nChar)
    {
        // Validate number and float input
        if (!(m_nFlags & (ITEM_FLAGS_EDIT_HEX | ITEM_FLAGS_EDIT_NUMBER | ITEM_FLAGS_EDIT_FLOAT)) || nChar == VK_BACK)
            return TRUE;

        std::wstring WindowText;
        int nValueLength = GetWindowTextLengthW();
        if (nValueLength > 0)
        {
            WindowText.reserve(nValueLength + 1);
            WindowText.resize(nValueLength);
            GetWindowTextW((wchar_t *)WindowText.c_str(), nValueLength + 1);
        }
        // Get selected positions
        int nStartChar;
        int nEndChar;
        GetSel(nStartChar, nEndChar);

        if (m_nMaxLen != -1)
        {
            if (nValueLength >= m_nMaxLen && nStartChar == nEndChar)
            {
                return FALSE;
            }
        }
        // Are we changing the sign?
        if ((m_nFlags & ITEM_FLAGS_EDIT_NEGATIVE) && nChar == _T('-'))
        {
            BOOL bNegative = FALSE;
            if (m_nFlags & ITEM_FLAGS_EDIT_FLOAT)
            {
                double dblValue = _wtof(WindowText.c_str());
                bNegative = (dblValue < 0);
                WindowText = stdstr_f("%lf", -dblValue).ToUTF16();
            }
            else
            {
                long lValue = _wtol(WindowText.c_str());
                bNegative = (lValue < 0);
                WindowText = stdstr_f("%ld", -lValue).ToUTF16();
            }

            SetWindowText(WindowText.c_str());

            // Restore select position
            SetSel(bNegative ? nStartChar - 1 : nStartChar + 1, bNegative ? nEndChar - 1 : nEndChar + 1);
            return FALSE;
        }

        // Construct new value string using entered character
        std::wstring strNewValue = WindowText.substr(0, nStartChar) + nChar + WindowText.substr(WindowText.length() - nEndChar);

        int nGreaterThan = 0;
        int nLessThan = 0;
        int nEquals = 0;
        int nDecimalPoint = 0;

        int nNegativeIndex = -1;
        int nGreaterIndex = -1;
        int nLessIndex = -1;
        int nEqualIndex = -1;
        int nDecimalIndex = -1;
        int nDigitIndex = -1;

        for (size_t nCharIndex = 0; nCharIndex < strNewValue.length(); nCharIndex++)
        {
            TCHAR nCharValue = strNewValue[nCharIndex];
            if ((m_nFlags & ITEM_FLAGS_EDIT_HEX) && (nCharValue >= 'A' && nCharValue <= 'F' || nCharValue >= 'a' && nCharValue <= 'f'))
            {
                if (nDigitIndex < 0)
                {
                    nDigitIndex = (int)nCharIndex;
                }
                break;
            }

            switch (nCharValue)
            {
            case _T('-'):
                nNegativeIndex = (int)nCharIndex;
                break;
            case _T('>'):
                if (!(m_nFlags & ITEM_FLAGS_EDIT_OPERATOR))
                    return FALSE;
                nGreaterIndex = (int)nCharIndex;
                nGreaterThan++;
                break;
            case _T('<'):
                if (!(m_nFlags & ITEM_FLAGS_EDIT_OPERATOR))
                    return FALSE;
                nLessIndex = (int)nCharIndex;
                nLessThan++;
                break;
            case _T('='):
                if (!(m_nFlags & ITEM_FLAGS_EDIT_OPERATOR))
                    return FALSE;
                nEqualIndex = (int)nCharIndex;
                nEquals++;
                break;
            case _T('.'):
                if (!(m_nFlags & ITEM_FLAGS_EDIT_FLOAT))
                    return FALSE;
                nDecimalIndex = (int)nCharIndex;
                nDecimalPoint++;
                break;
            default:
                if (!_istdigit(nCharValue))
                    return FALSE;
                if (nDigitIndex < 0)
                    nDigitIndex = (int)nCharIndex;
                break;
            }

            // Invalid if text contains more than one '>', '<', '=' or '.'
            if (nGreaterThan > 1 || nLessThan > 1 || nEquals > 1 || nDecimalPoint > 1)
                return FALSE;
        }

        // Invalid if text contains '=>' or '=<'
        if (nGreaterIndex != -1 && nEqualIndex != -1 && nGreaterIndex > nEqualIndex)
            return FALSE;
        if (nLessIndex != -1 && nEqualIndex != -1 && nLessIndex > nEqualIndex)
            return FALSE;

        // Invalid if digits exist before operator
        if (nDigitIndex != -1 && nGreaterIndex != -1 && nGreaterIndex > nDigitIndex)
            return FALSE;
        if (nDigitIndex != -1 && nLessIndex != -1 && nLessIndex > nDigitIndex)
            return FALSE;
        if (nDigitIndex != -1 && nEqualIndex != -1 && nEqualIndex > nDigitIndex)
            return FALSE;
        if (nDigitIndex != -1 && nNegativeIndex != -1 && nNegativeIndex > nDigitIndex)
            return FALSE;

        return TRUE;
    }

    BEGIN_MSG_MAP_EX(CListEdit)
    MSG_WM_KILLFOCUS(OnKillFocus)
    MSG_WM_GETDLGCODE(OnGetDlgCode)
    MSG_WM_CHAR(OnChar)
    END_MSG_MAP()

    void OnKillFocus(HWND /*hNewWnd*/)
    {
        CWindow wndParent(GetParent());
        if (wndParent.IsWindow())
        {
            std::wstring strValue;
            int nValueLength = GetWindowTextLength();
            strValue.reserve(nValueLength + 1);
            strValue.resize(nValueLength);
            GetWindowText((LPTSTR)strValue.c_str(), nValueLength + 1);

            CListNotify listNotify;
            listNotify.m_hdrNotify.hwndFrom = m_hWnd;
            listNotify.m_hdrNotify.idFrom = GetDlgCtrlID();
            listNotify.m_hdrNotify.code = LCN_ENDEDIT;
            listNotify.m_nItem = m_nItem;
            listNotify.m_nSubItem = m_nSubItem;
            listNotify.m_nExitChar = m_nExitChar;
            listNotify.m_lpszItemText = strValue.c_str();
            listNotify.m_lpItemDate = nullptr;

            // Forward notification to parent
            FORWARD_WM_NOTIFY(wndParent, listNotify.m_hdrNotify.idFrom, &listNotify.m_hdrNotify, ::SendMessage);
        }

        ShowWindow(SW_HIDE);
    }

    UINT OnGetDlgCode(LPMSG /*lpMessage*/)
    {
        return DLGC_WANTALLKEYS;
    }

    void OnChar(TCHAR nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
    {
        switch (nChar)
        {
        case VK_TAB:
        case VK_RETURN:
        case VK_ESCAPE:
        {
            m_nExitChar = nChar;
            CWindow wndParent(GetParent());
            if (wndParent.IsWindow())
                wndParent.SetFocus();
        }
        break;
        default:
            SetMsgHandled(!IsValid(nChar));
            break;
        }
    }
};
