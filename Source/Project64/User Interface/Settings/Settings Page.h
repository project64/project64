#pragma once

class CSettingsPage 
{
public:
	virtual ~CSettingsPage ( void ) = 0 {};
	
	virtual LanguageStringID PageTitle ( void ) = 0;
	virtual void             HidePage  ( void ) = 0;
	virtual void             ShowPage  ( void ) = 0;
};

typedef std::vector<CSettingsPage *> SETTING_PAGES;

class CConfigSettingSection
{
	SETTING_PAGES m_Pages;
	stdstr        m_PageTitle;

public:
	CConfigSettingSection ( LPCSTR PageTitle );
	~CConfigSettingSection();

	LPCTSTR GetPageTitle    ( void ) const { return m_PageTitle.c_str(); }
	void    AddPage         ( CSettingsPage * Page );
	int     GetPageCount    ( void ) const { return m_Pages.size(); }
	CSettingsPage * GetPage ( int PageNo );
};

#include "Settings Page - Game - General.h"
#include "Settings Page - Game - Plugin.h"
#include "Settings Page - Game - Recompiler.h"
#include "Settings Page - Game - Status.h"
