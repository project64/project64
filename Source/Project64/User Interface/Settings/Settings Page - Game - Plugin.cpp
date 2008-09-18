#include "../../User Interface.h"
#include "Settings Page.h"
#include "Settings Page - Game - Plugin.h"

CGamePluginPage::CGamePluginPage (HWND hParent, const RECT & rcDispay )
{
	Create(hParent);
	if (m_hWnd == NULL)
	{
		return;
	}
	SetWindowPos(HWND_TOP,&rcDispay,SWP_HIDEWINDOW);
}

void CGamePluginPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGamePluginPage::HidePage()
{
	ShowWindow(SW_HIDE);
}
