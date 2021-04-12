#pragma once

#include <list>

enum Menu_ID 
{
	// ControlID
	SPLITER, SUB_MENU, NO_ID, ID_PLUGIN_MENU,
};

const std::wstring EMPTY_STDSTR = L"";

class MENU_ITEM 
{
public:
	MENU_ITEM (void) 
    {
		Reset(NO_ID);
	}
	MENU_ITEM ( int ID, LanguageStringID Title = EMPTY_STRING, const std::wstring & ShortCut = EMPTY_STDSTR,
		void * SubMenu = nullptr, const std::wstring & ManualString = EMPTY_STDSTR) 
	{
		Reset(ID,Title,ShortCut,SubMenu,ManualString);
	}
	void Reset ( int ID, LanguageStringID Title = EMPTY_STRING, const std::wstring & ShortCut2 = EMPTY_STDSTR,
		void * SubMenu = nullptr, const std::wstring & ManualString = EMPTY_STDSTR) 
	{
		this->m_ID           = ID;
		this->m_Title        = Title;
		this->m_ShortCut     = ShortCut2;
		this->m_SubMenu      = SubMenu;
		this->m_ManualString = ManualString;
		this->m_ItemTicked   = false;
		this->m_ItemEnabled  = true;
	}

    int ID() const { return m_ID;  }
	LanguageStringID Title() const { return m_Title;  }
	const std::wstring & ShortCut() const { return m_ShortCut;  }
	void * SubMenu() const { return m_SubMenu;  }
	const std::wstring & ManualString() const { return m_ManualString;  }
	bool ItemTicked() const { return m_ItemTicked;  }
	bool ItemEnabled() const { return m_ItemEnabled;  }
	
    void SetItemTicked(bool ItemTicked) { m_ItemTicked = ItemTicked;  }
    void SetItemEnabled(bool ItemEnabled) { m_ItemEnabled = ItemEnabled;  }


private:
    int                m_ID; 
	LanguageStringID   m_Title; 
	std::wstring       m_ShortCut;
	void *             m_SubMenu;
	std::wstring       m_ManualString;
	bool               m_ItemTicked;
	bool               m_ItemEnabled;
};

typedef std::list<MENU_ITEM>   MenuItemList;

class CBaseMenu  {
protected:
	HMENU m_MenuHandle;
	
	bool AddMenu    ( HMENU hMenu, MenuItemList Items );

public:
	    CBaseMenu ();


    virtual int  ProcessAccelerator(HWND hWnd, void * lpMsg ) = 0; // Pure virtual draw() function
    virtual bool ProcessMessage(HWND hWnd, DWORD wNotifyCode, DWORD wID) = 0; // Pure virtual draw() function
    virtual void ResetMenu(void) = 0; // Pure virtual draw() function
	HMENU GetHandle (void) { return m_MenuHandle; }
};
