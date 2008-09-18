#include "../../User Interface.h"
#include "Settings Page.h"
#include "Settings Page - Game - General.h"

CGameGeneralPage::CGameGeneralPage (HWND hParent, const RECT & rcDispay )
{
	Create(hParent);
	if (m_hWnd == NULL)
	{
		return;
	}
	SetWindowPos(HWND_TOP,&rcDispay,SWP_HIDEWINDOW);
}

void CGameGeneralPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGameGeneralPage::HidePage()
{
	ShowWindow(SW_HIDE);
}