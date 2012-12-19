/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <list>

enum Menu_ID {
	//ControlID
	SPLITER, SUB_MENU, NO_ID, ID_PLUGIN_MENU,
};

const stdstr EMPTY_STDSTR = "";

class MENU_ITEM {
public:
	MENU_ITEM (void) {
		Reset(NO_ID);
	}
	MENU_ITEM ( int ID, LanguageStringID Title = EMPTY_STRING, const stdstr & ShotCut = EMPTY_STDSTR,
		void * SubMenu = NULL, const stdstr & ManualString = EMPTY_STDSTR) 
	{
		Reset(ID,Title,ShotCut,SubMenu,ManualString);
	}
	void Reset ( int ID, LanguageStringID Title = EMPTY_STRING, const stdstr & ShotCut2 = EMPTY_STDSTR,
		void * SubMenu = NULL, const stdstr & ManualString = EMPTY_STDSTR) 
	{
		this->ID           = ID;
		this->Title        = Title;
		this->ShotCut      = ShotCut2;
		this->SubMenu      = SubMenu;
		this->ManualString = ManualString;
		this->ItemTicked   = false;
		this->ItemEnabled  = true;
	}
	int                ID; 
	LanguageStringID   Title; 
	stdstr             ShotCut;
	void *             SubMenu;
	stdstr             ManualString;
	bool               ItemTicked;
	bool               ItemEnabled;
};

typedef std::list<MENU_ITEM>   MenuItemList;

class CBaseMenu  {
protected:
	MENU_HANDLE m_MenuHandle;
	
	bool AddMenu    ( MENU_HANDLE hMenu, MenuItemList Items );

public:
	    CBaseMenu ();


    virtual int  ProcessAccelerator(WND_HANDLE hWnd, void * lpMsg ) = 0; // pure virtual draw() function
    virtual bool ProcessMessage(WND_HANDLE hWnd, DWORD wNotifyCode, DWORD wID) = 0; // pure virtual draw() function
    virtual void ResetMenu(void) = 0; // pure virtual draw() function
	MENU_HANDLE GetHandle (void) { return m_MenuHandle; }
};
