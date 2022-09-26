#pragma once

class CModifiedEditBox :
    public CEdit
{
    bool m_Changed;
    bool m_Reset;
    HFONT m_BoldFont;
    HFONT m_OriginalFont;
    HWND m_TextField;
    bool m_bString;

public:
    // Constructors
    CModifiedEditBox(bool bString = true, HWND hWnd = nullptr);
    ~CModifiedEditBox();

    void SetReset(bool Reset);
    void SetChanged(bool Changed);
    void SetTextField(HWND hWnd);

    inline bool IsChanged(void) const
    {
        return m_Changed;
    }
    inline bool IsReset(void) const
    {
        return m_Reset;
    }
    inline bool IsbString(void) const
    {
        return m_bString;
    }
};
