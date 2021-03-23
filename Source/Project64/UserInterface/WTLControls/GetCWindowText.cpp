#include "stdafx.h"
#include "GetCWindowText.h"
#include <Common/StdString.h>

std::string GetCWindowText(const CWindow & window)
{
    int nLen = window.GetWindowTextLength();
    if (nLen == 0)
    {
        return "";
    }
    std::wstring WindowText;
    WindowText.resize(nLen + 1);
    window.GetWindowText((wchar_t *)WindowText.c_str(), nLen + 1);
    return stdstr().FromUTF16(WindowText.c_str());
}
