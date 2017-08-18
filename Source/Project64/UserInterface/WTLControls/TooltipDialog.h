#pragma once

#include "stdafx.h"


typedef struct {
	WORD ctrlId;
	char* text;
} _ToolTipMap;

template <class T>
class CToolTipDialog
{
private:
	HWND m_hWndTooltip;

public:
	CToolTipDialog()
	{
	}

	void AddToolTip(WORD ctrlId, char* lpszText)
	{
		T* pT = static_cast<T*>(this);

		TOOLINFO toolInfo = { 0 };
		toolInfo.cbSize = sizeof(toolInfo);
		toolInfo.hwnd = pT->m_hWnd;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR) ::GetDlgItem(pT->m_hWnd, ctrlId);
		toolInfo.lpszText = lpszText;
		SendMessage(m_hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	}

	void DlgToolTip_Init()
	{
		T* pT = static_cast<T*>(this);

		m_hWndTooltip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			pT->m_hWnd, NULL,
			NULL, NULL);

		const _ToolTipMap* map = pT->_GetToolTipMap();

		for (int i = 0; map[i].ctrlId != 0; i++)
		{
			AddToolTip(map[i].ctrlId, map[i].text);
		}
	}
};

#define BEGIN_TOOLTIP_MAP() static const _ToolTipMap* _GetToolTipMap() { static const _ToolTipMap map[] = {
#define TOOLTIP(ctrlId, text) { ctrlId, text },
#define END_TOOLTIP_MAP() { 0 } }; return map; }