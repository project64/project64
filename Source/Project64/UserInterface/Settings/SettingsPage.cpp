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

CSettingsPage * CConfigSettingSection::GetPage(size_t PageNo)
{
	if (PageNo >= m_Pages.size())
	{
		return nullptr;
	}
	return m_Pages[PageNo];
}
