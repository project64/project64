#ifndef __MENU_CLASS__H__
#define __MENU_CLASS__H__

typedef struct { 
	LPCSTR    Name; 
	int       Key;  
	LPCSTR    KeyName; 
} VIRTUAL_KEY;

#include <list>
class MENU_SHORT_CUT_KEY {
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

public:
	MENU_SHORT_CUT_KEY(void);
	MENU_SHORT_CUT_KEY(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode );
	bool   Same       (WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode ) const;

	static VIRTUAL_KEY * VirtualKeyList(int &Size);

	inline stdstr      Name       ( void ) const { return m_ShortCutName; }
	inline WORD        Key        ( void ) const { return m_key; }
	inline bool        Ctrl       ( void ) const { return m_bCtrl; }
	inline bool        Alt        ( void ) const { return m_bAlt; }
	inline bool        Shift      ( void ) const { return m_bShift; }
	inline ACCESS_MODE AccessMode ( void ) const { return m_AccessMode; }
};

typedef std::list<MENU_SHORT_CUT_KEY>   SHORTCUT_KEY_LIST;

class MENU_SHORT_CUT {
	MENU_SHORT_CUT_KEY::ACCESS_MODE        m_Access;
	LanguageStringID   m_Section;
	LanguageStringID   m_Title;
	SHORTCUT_KEY_LIST  m_AccelList;

public:
	MENU_SHORT_CUT(LanguageStringID Section, LanguageStringID Title, MENU_SHORT_CUT_KEY::ACCESS_MODE Access) 
	{
		Reset(Section, Title,Access);
	}
	void Reset ( LanguageStringID Section, LanguageStringID Title, MENU_SHORT_CUT_KEY::ACCESS_MODE Access) 
	{
		this->m_Section = Section;
		this->m_Title   = Title;
		this->m_Access  = Access;
	}
	void AddShortCut ( WORD key, bool bCtrl, bool bAlt, bool bShift, MENU_SHORT_CUT_KEY::ACCESS_MODE AccessMode );
	void RemoveItem  ( MENU_SHORT_CUT_KEY * ShortCut );

	inline const SHORTCUT_KEY_LIST  & GetAccelItems ( void ) const { return m_AccelList; }
	inline LanguageStringID Section    ( void ) const { return m_Section; }
	inline LanguageStringID Title      ( void ) const { return m_Title; }
	inline MENU_SHORT_CUT_KEY::ACCESS_MODE      AccessMode ( void ) const { return m_Access; }
};

typedef std::map<int,MENU_SHORT_CUT>   MENU_SHORT_CUT_MAP;
typedef MENU_SHORT_CUT_MAP MSC_MAP;

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
    virtual MSC_MAP GetShortCutInfo(bool InitialSettings) = 0; // pure virtual draw() function
    virtual void      SaveShortCuts   ( MSC_MAP * ShortCuts ) = 0;
    virtual LanguageStringID GetShortCutMenuItemName(MSC_MAP * ShortCuts, WORD key, bool bCtrl, bool bAlt, bool bShift, MENU_SHORT_CUT_KEY::ACCESS_MODE Access ) = 0; // pure virtual draw() function
};

#endif