#include "../../User Interface.h"
#include "Settings Page.h"
#include "Settings Page - Game - Recompiler.h"

CGameRecompilePage::CGameRecompilePage (HWND hParent, const RECT & rcDispay )
{
	Create(hParent);
	if (m_hWnd == NULL)
	{
		return;
	}
	SetWindowPos(HWND_TOP,&rcDispay,SWP_HIDEWINDOW);
}

void CGameRecompilePage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGameRecompilePage::HidePage()
{
	ShowWindow(SW_HIDE);
}