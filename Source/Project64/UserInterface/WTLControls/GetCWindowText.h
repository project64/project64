#pragma once

#include "stdafx.h"

inline static stdstr GetCWindowText(const CWindow *window)
{
    stdstr Result;
    int nLen = ::GetWindowTextLengthW(window->m_hWnd);
    if (nLen == 0)
    {
        return Result;
    }
    std::wstring WindowText;
    WindowText.resize(nLen + 1);
    ::GetWindowTextW(window->m_hWnd, (wchar_t *)WindowText.c_str(), nLen + 1);
    return Result.FromUTF16(WindowText.c_str());
}