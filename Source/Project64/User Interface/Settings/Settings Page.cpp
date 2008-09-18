#include "../../User Interface.h"
#include "Settings Page.h"

CConfigSettingSection::CConfigSettingSection( LPCSTR PageTitle ) :
	m_PageTitle(PageTitle)
{
}

CConfigSettingSection::~CConfigSettingSection ()
{
	for (int i = 0; i < m_Pages.size(); i++)
	{
		delete m_Pages[i];
	}
	m_Pages.clear();
}

void CConfigSettingSection::AddPage(CSettingsPage * Page )
{
	m_Pages.push_back(Page);
}

CSettingsPage * CConfigSettingSection::GetPage ( int PageNo )
{
	if (PageNo < 0 || PageNo >= m_Pages.size())
	{
		return NULL;
	}
	return m_Pages[PageNo];
}
