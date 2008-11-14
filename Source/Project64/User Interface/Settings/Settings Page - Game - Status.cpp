#include "../../User Interface.h"
#include "Settings Page.h"
#include "Settings Page - Game - Status.h"

CGameStatusPage::CGameStatusPage (HWND hParent, const RECT & rcDispay )
{
	Create(hParent);
	if (m_hWnd == NULL)
	{
		return;
	}
	SetWindowPos(HWND_TOP,&rcDispay,SWP_HIDEWINDOW);
}

void CGameStatusPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGameStatusPage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void CGameStatusPage::ApplySettings( bool UpdateScreen )
{
}

bool CGameStatusPage::EnableReset ( void )
{
	return false;
}

void CGameStatusPage::ResetPage()
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}