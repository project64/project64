#include "stdafx.h"

#include "SettingsPage.h"

CConfigSettingSection::CConfigSettingSection(LPCWSTR PageTitle) :
m_PageTitle(PageTitle)
{
}

CConfigSettingSection::~CConfigSettingSection()
{
	for (size_t i = 0; i < m_Pages.size(); i++)
	{
		CSettingsPage * Page = m_Pages[i];
		delete Page;
	}
	m_Pages.clear();
}

void CConfigSettingSection::AddPage(CSettingsPage * Page)
{
	m_Pages.push_back(Page);
}

CSettingsPage * CConfigSettingSection::GetPage(int PageNo)
{
	if (PageNo < 0 || PageNo >= (int)m_Pages.size())
	{
		return NULL;
	}
	return m_Pages[PageNo];
}
