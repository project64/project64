#pragma once

#include "stdafx.h"

inline static stdstr GetCWindowText(const CWindow *window)
{
    stdstr Result;
    int nLen = ::GetWindowTextLength(window->m_hWnd);
    if (nLen == 0)
    {
        return Result;
    }
    Result.resize(nLen + 1);
    ::GetWindowText(window->m_hWnd, (char *)Result.c_str(), nLen + 1);
    return Result;
}