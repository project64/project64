#include "stdafx.h"

CBaseMenu::CBaseMenu () :
	m_MenuHandle((MENU_HANDLE)CreateMenu())
{
}

bool CBaseMenu::AddMenu(MENU_HANDLE hMenu, MenuItemList Items ) {
	if (Items.begin() == Items.end()) { return false; }

	UINT ItemID, uFlags;
	stdstr Text;
	stdstr String;
	for (MenuItemList::iterator MenuItem = Items.begin(); MenuItem != Items.end(); MenuItem++) {
		ItemID = MenuItem->ID;
		uFlags = MF_STRING;
		Text = _Lang->GetString(MenuItem->Title).c_str();

		if (MenuItem->Title == EMPTY_STRING && MenuItem->ManualString.length() > 0) {
			Text = MenuItem->ManualString;
		}
		if (ItemID == SPLITER) {
			uFlags |= MF_SEPARATOR;
		}
		if (MenuItem->ItemTicked) {
			uFlags |= MFS_CHECKED;
		}
		if (MenuItem->ItemEnabled) {
			uFlags |= MFS_ENABLED;
		} else {
			uFlags |= MFS_DISABLED;
		}
			
		MenuItemList * SubMenu = (MenuItemList *)MenuItem->SubMenu;
		if (ItemID == SUB_MENU && HIWORD(SubMenu) != 0 && (SubMenu->begin() != SubMenu->end())) {
			ItemID = (UINT)CreatePopupMenu();
			uFlags |= MF_POPUP;

			AddMenu((MENU_HANDLE)ItemID,*SubMenu);
		}
		
		if (ItemID == ID_PLUGIN_MENU)
		{
			ItemID = (UINT)MenuItem->SubMenu;
			uFlags |= MF_POPUP;
			MENUITEMINFO lpmii;
	
			lpmii.cbSize = sizeof(MENUITEMINFO);
			lpmii.fMask = MIIM_STATE;
			lpmii.fState = 0;
			SetMenuItemInfo((HMENU)ItemID, (DWORD)MenuItem->SubMenu, FALSE,&lpmii);
		}
		
		if (MenuItem->ShotCut.empty() == false) {
			String = Text;
			String += "\t";
			String += MenuItem->ShotCut;
			Text = String;
		}
		AppendMenu((HMENU)hMenu,uFlags,ItemID,Text.c_str());
	}
		
	return true;
}


/*void MENU_SHORT_CUT::RemoveItem  ( MENU_SHORT_CUT_KEY * ShortCut )
{
	for (SHORTCUT_KEY_LIST::iterator item = m_AccelList.begin(); item != m_AccelList.end(); item++) {
		if (ShortCut == &*item) {
			m_AccelList.erase(item);
			return;
		}
	}
	
}*/

