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

typedef struct { 
	LPCSTR    Name; 
	int       Key;  
	LPCSTR    KeyName; 
} VIRTUAL_KEY;

class CMenuShortCutKey {
public:
	enum ACCESS_MODE {
		NONE                    = 0,
		GAME_NOT_RUNNING        = 1,
		GAME_RUNNING_WINDOW     = 2,
		NOT_IN_FULLSCREEN       = 3,
		GAME_RUNNING_FULLSCREEN = 4,
		GAME_RUNNING            = 6,
		ANYTIME                 = 7,
	} ;

private:
	static VIRTUAL_KEY m_VirtualKeyList[];

	stdstr        m_ShortCutName;
	WORD          m_key; 
	bool          m_bCtrl;
	bool          m_bAlt;
	bool          m_bShift;
	ACCESS_MODE   m_AccessMode;
	bool          m_bUserAdded;
	bool          m_bInactive;

public:
	CMenuShortCutKey(void);
	CMenuShortCutKey(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode, bool bUserAdded, bool bInactive );
	bool   Same     (WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode ) const;

	static VIRTUAL_KEY * VirtualKeyList(int &Size);

	inline stdstr      Name       ( void ) const { return m_ShortCutName; }
	inline WORD        Key        ( void ) const { return m_key; }
	inline bool        Ctrl       ( void ) const { return m_bCtrl; }
	inline bool        Alt        ( void ) const { return m_bAlt; }
	inline bool        Shift      ( void ) const { return m_bShift; }
	inline bool        UserAdded  ( void ) const { return m_bUserAdded; }
	inline bool        Inactive   ( void ) const { return m_bInactive; }
	inline ACCESS_MODE AccessMode ( void ) const { return m_AccessMode; }

	inline void        SetInactive ( bool Inactive ) { m_bInactive = Inactive; }
};

class CShortCutItem 
{
public:
	typedef std::list<CMenuShortCutKey>   SHORTCUT_KEY_LIST;

private:
	typedef CMenuShortCutKey::ACCESS_MODE ACCESS_MODE;

	ACCESS_MODE        m_Access;
	LanguageStringID   m_Section;
	LanguageStringID   m_Title;
	SHORTCUT_KEY_LIST  m_AccelList;

public:
	CShortCutItem(LanguageStringID Section, LanguageStringID Title, ACCESS_MODE Access);
	void Reset ( LanguageStringID Section, LanguageStringID Title, ACCESS_MODE Access);
	void AddShortCut ( WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode, bool bUserAdded = false, bool bInactive = false );
	void RemoveItem  ( CMenuShortCutKey * ShortCut );

	inline const SHORTCUT_KEY_LIST  & GetAccelItems ( void ) const { return m_AccelList; }
	inline LanguageStringID Section    ( void ) const { return m_Section; }
	inline LanguageStringID Title      ( void ) const { return m_Title; }
	inline ACCESS_MODE      AccessMode ( void ) const { return m_Access; }
};

typedef std::map<int,CShortCutItem>   MSC_MAP;

class CShortCuts 
{
	typedef CShortCutItem::SHORTCUT_KEY_LIST SHORTCUT_KEY_LIST;
	typedef CMenuShortCutKey::ACCESS_MODE    ACCESS_MODE;
	typedef LanguageStringID LangStr;

	MSC_MAP m_ShortCuts;
	CriticalSection m_CS;

	void AddShortCut   ( WORD ID, LangStr Section, LangStr LangID, CMenuShortCutKey::ACCESS_MODE AccessMode);

public:
	 CShortCuts ( void );
	~CShortCuts ( void );

	std::wstring ShortCutString       ( int  MenuID, ACCESS_MODE AccessLevel );
    LangStr      GetMenuItemName      ( WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE Access );
	HACCEL       GetAcceleratorTable  ( void );
	MSC_MAP    & GetShortCuts         ( void ) { return m_ShortCuts; }

	void Load ( bool InitialValues = false );
	void Save ( void );
};
