#include "..\\User Interface.h"
#include <windows.h>

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

VIRTUAL_KEY MENU_SHORT_CUT_KEY::m_VirtualKeyList[] = {
	{ "VK_LBUTTON",        0x01, "VK_LBUTTON" },
	{ "VK_RBUTTON",        0x02, "VK_RBUTTON" },
	{ "VK_CANCEL",         0x03, "VK_CANCEL" },
	{ "VK_MBUTTON",        0x04, "VK_MBUTTON" },
	{ "VK_XBUTTON1",       0x05, "VK_XBUTTON1" },
	{ "VK_XBUTTON2",       0x06, "VK_XBUTTON2" },
	{ "VK_BACK",           0x08, "VK_BACK" },
	{ "VK_TAB",            0x09, "VK_TAB" },
	{ "VK_CLEAR",          0x0C, "VK_CLEAR" },
	{ "VK_RETURN",         0x0D, "Return" },
	{ "VK_SHIFT",          0x10, "VK_SHIFT" },
	{ "VK_CONTROL",        0x11, "VK_CONTROL" },
	{ "VK_MENU",           0x12, "VK_MENU" },
	{ "VK_PAUSE",          0x13, "Pause" },
	{ "VK_CAPITAL",        0x14, "VK_CAPITAL" },
	{ "VK_KANA",           0x15, "VK_KANA" },
	{ "VK_HANGUL",         0x15, "VK_HANGUL" },
	{ "VK_JUNJA",          0x17, "VK_JUNJA" },
	{ "VK_FINAL",          0x18, "VK_FINAL" },
	{ "VK_HANJA",          0x19, "VK_HANJA" },
	{ "VK_KANJI",          0x19, "VK_KANJI" },
	{ "VK_ESCAPE",         0x1B, "Esc" },
	{ "VK_CONVERT",        0x1C, "VK_CONVERT" },
	{ "VK_NONCONVERT",     0x1D, "VK_NONCONVERT" },
	{ "VK_ACCEPT",         0x1E, "VK_ACCEPT" },
	{ "VK_MODECHANGE",     0x1F, "VK_MODECHANGE" },
	{ "VK_SPACE",          0x20, "Space" },
	{ "VK_PRIOR",          0x21, "Page Up" },
	{ "VK_NEXT",           0x22, "Page Down" },
	{ "VK_END",            0x23, "End" },
	{ "VK_HOME",           0x24, "Home" },
	{ "VK_LEFT",           0x25, "Left" },
	{ "VK_UP",             0x26, "Up" },
	{ "VK_RIGHT",          0x27, "Right" },
	{ "VK_DOWN",           0x28, "Down" },
	{ "VK_SELECT",         0x29, "VK_SELECT" },
	{ "VK_PRINT",          0x2A, "VK_PRINT" },
	{ "VK_EXECUTE",        0x2B, "VK_EXECUTE" },
	{ "VK_SNAPSHOT",       0x2C, "VK_SNAPSHOT" },
	{ "VK_INSERT",         0x2D, "Insert" },
	{ "VK_DELETE",         0x2E, "Delete" },
	{ "VK_HELP",           0x2F, "Help" },
	{ "VK_0",              0x30, "0" },
	{ "VK_1",              0x31, "1" },
	{ "VK_2",              0x32, "2" },
	{ "VK_3",              0x33, "3" },
	{ "VK_4",              0x34, "4" },
	{ "VK_5",              0x35, "5" },
	{ "VK_6",              0x36, "6" },
	{ "VK_7",              0x37, "7" },
	{ "VK_8",              0x38, "8" },
	{ "VK_9",              0x39, "9" },
	{ "VK_A",              0x41, "A" },
	{ "VK_B",              0x42, "B" },
	{ "VK_C",              0x43, "C" },
	{ "VK_D",              0x44, "D" },
	{ "VK_E",              0x45, "E" },
	{ "VK_F",              0x46, "F" },
	{ "VK_G",              0x47, "G" },
	{ "VK_H",              0x48, "H" },
	{ "VK_I",              0x49, "I" },
	{ "VK_J",              0x4A, "J" },
	{ "VK_K",              0x4B, "K" },
	{ "VK_L",              0x4C, "L" },
	{ "VK_M",              0x4D, "M" },
	{ "VK_N",              0x4E, "N" },
	{ "VK_O",              0x4F, "O" },
	{ "VK_P",              0x50, "P" },
	{ "VK_Q",              0x51, "Q" },
	{ "VK_R",              0x52, "R" },
	{ "VK_S",              0x53, "S" },
	{ "VK_T",              0x54, "T" },
	{ "VK_U",              0x55, "U" },
	{ "VK_V",              0x56, "V" },
	{ "VK_W",              0x57, "W" },
	{ "VK_X",              0x58, "X" },
	{ "VK_Y",              0x59, "Y" },
	{ "VK_Z",              0x5A, "Z" },
	{ "VK_LWIN",           0x5B, "VK_LWIN" },
	{ "VK_RWIN",           0x5C, "VK_RWIN" }, 
	{ "VK_APPS",           0x5D, "VK_APPS" },
	{ "VK_SLEEP",          0x5D, "VK_SLEEP" },
	{ "VK_NUMPAD0",        0x60, "Numpad0" },
	{ "VK_NUMPAD1",        0x61, "Numpad1" },
	{ "VK_NUMPAD2",        0x62, "Numpad2" },
	{ "VK_NUMPAD3",        0x63, "Numpad3" },
	{ "VK_NUMPAD4",        0x64, "Numpad4" },
	{ "VK_NUMPAD5",        0x65, "Numpad5" },
	{ "VK_NUMPAD6",        0x66, "Numpad6" },
	{ "VK_NUMPAD7",        0x67, "Numpad7" },
	{ "VK_NUMPAD8",        0x68, "Numpad8" },
	{ "VK_NUMPAD9",        0x69, "Numpad9" },
	{ "VK_MULTIPLY",       0x6A, "*" },
	{ "VK_ADD",            0x6B, "+" },
	{ "VK_SEPARATOR",      0x6C, "" },
	{ "VK_SUBTRACT",       0x6D, "-" },
	{ "VK_DECIMAL",        0x6E, "." },
	{ "VK_DIVIDE",         0x6F, "/" },
	{ "VK_F1",             0x70, "F1" },
	{ "VK_F2",             0x71, "F2" },
	{ "VK_F3",             0x72, "F3" },
	{ "VK_F4",             0x73, "F4" },
	{ "VK_F5",             0x74, "F5" },
	{ "VK_F6",             0x75, "F6" },
	{ "VK_F7",             0x76, "F7" },
	{ "VK_F8",             0x77, "F8" },
	{ "VK_F9",             0x78, "F9" },
	{ "VK_F10",            0x79, "F10" },
	{ "VK_F11",            0x7A, "F11" },
	{ "VK_F12",            0x7B, "F12" },
	{ "VK_F13",            0x7C, "F13" },
	{ "VK_F14",            0x7D, "F14" },
	{ "VK_F15",            0x7E, "F15" },
	{ "VK_F16",            0x7F, "F16" },
	{ "VK_F17",            0x80, "F17" },
	{ "VK_F18",            0x81, "F18" },
	{ "VK_F19",            0x82, "F19" },
	{ "VK_F20",            0x83, "F20" },
	{ "VK_F21",            0x84, "F21" },
	{ "VK_F22",            0x85, "F22" },
	{ "VK_F23",            0x86, "F23" },
	{ "VK_F24",            0x87, "F24" },
	{ "VK_NUMLOCK",        0x90, "Numlock" },
	{ "VK_SCROLL",         0x91, "VK_SCROLL" },
	{ "VK_LSHIFT",         0xA0, "VK_LSHIFT" },
	{ "VK_RSHIFT",         0xA1, "VK_RSHIFT" },
	{ "VK_LCONTROL",       0xA2, "VK_LCONTROL" },
	{ "VK_RCONTROL",       0xA3, "VK_RCONTROL" },
	{ "VK_LMENU",          0xA4, "VK_LMENU" },
	{ "VK_RMENU",          0xA5, "VK_RMENU" },
	{ "VK_BROWSER_BACK",   0xA6, "" },
	{ "VK_BROWSER_FORWARD",0xA7, "" },
	{ "VK_BROWSER_REFRESH",0xA8, "" },
	{ "VK_BROWSER_STOP",   0xA9, "" },
	{ "VK_BROWSER_SEARCH", 0xAA, "" },
	{ "VK_BROWSER_FAVORITES",0xAB, "" },
	{ "VK_BROWSER_HOME",   0xAC, "" },
	{ "VK_VOLUME_MUTE",    0xAD, "" },
	{ "VK_VOLUME_DOWN",    0xAE, "" },
	{ "VK_VOLUME_UP",      0xAF, "" },
	{ "VK_MEDIA_NEXT_TRACK",0xB0, "" },
	{ "VK_MEDIA_PREV_TRACK",0xB1, "" },
	{ "VK_MEDIA_STOP",      0xB2, "" },
	{ "VK_MEDIA_PLAY_PAUSE",0xB3, "" },
	{ "VK_LAUNCH_MAIL",     0xB4, "" },
	{ "VK_LAUNCH_MEDIA_SELECT",0xB5, "" },
	{ "VK_LAUNCH_APP1",     0xB6, "" },
	{ "VK_LAUNCH_APP2",     0xB7, "" },
	{ "VK_OEM_1 (;:)",      0xBA, "" },
	{ "VK_OEM_PLUS",        0xBB, "+" },
	{ "VK_OEM_COMMA",       0xBC, "" },
	{ "VK_OEM_MINUS",       0xBD, "-" },
	{ "VK_OEM_PERIOD",      0xBE, "." },
	{ "VK_OEM_2 (/?)",      0xBF, "" },
	{ "VK_OEM_3 (`~)",      0xC0, "~" },	
	{ "VK_ATTN",           0xF6, "" },
	{ "VK_CRSEL",          0xF7, "" },
	{ "VK_EXSEL",          0xF8, "" }, 
	{ "VK_EREOF",          0xF9, "" },
	{ "VK_PLAY",           0xFA, "" },
	{ "VK_ZOOM",           0xFB, "" },
	{ "VK_NONAME",         0xFC, "" }, 
	{ "VK_PA1",            0xFD, "" },
	{ "VK_OEM_CLEAR",      0xFE }
};

MENU_SHORT_CUT_KEY::MENU_SHORT_CUT_KEY(void) :
	m_key(0),m_bCtrl(false),m_bAlt(false),m_bShift(false),m_AccessMode(NONE)
{
}
		
MENU_SHORT_CUT_KEY::MENU_SHORT_CUT_KEY(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode ) {
	m_key        = key;
	m_bCtrl      = bCtrl;
	m_bAlt       = bAlt;
	m_bShift     = bShift;
	m_AccessMode = AccessMode;

	m_ShortCutName = "";
	for (int count = 0; count < sizeof(m_VirtualKeyList)/sizeof(m_VirtualKeyList[0]);count++){
		if (key == m_VirtualKeyList[count].Key) {
			m_ShortCutName = m_VirtualKeyList[count].KeyName;
			break;
		}
	}
	if (m_bShift) { m_ShortCutName.Format("Shift+%s",m_ShortCutName.c_str()); }
	if (m_bCtrl)  { m_ShortCutName.Format("Ctrl+%s",m_ShortCutName.c_str()); }
	if (m_bAlt)   { m_ShortCutName.Format("Alt+%s",m_ShortCutName.c_str()); }
}

bool MENU_SHORT_CUT_KEY::Same(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode) const
{
	if (key != m_key) { return false; }
	if (bShift != m_bShift) { return false; }
	if (bCtrl != m_bCtrl) { return false; }
	if (bAlt != m_bAlt) { return false; }
	if ((m_AccessMode & AccessMode) != AccessMode ) { return false; }
	return true;
}

VIRTUAL_KEY * MENU_SHORT_CUT_KEY::VirtualKeyList(int &Size) {
	Size = sizeof(m_VirtualKeyList) / sizeof(m_VirtualKeyList[0]); 
	return (VIRTUAL_KEY *)m_VirtualKeyList; 
}

void MENU_SHORT_CUT::AddShortCut(WORD key, bool bCtrl, bool bAlt, bool bShift, MENU_SHORT_CUT_KEY::ACCESS_MODE AccessMode) {
	m_AccelList.push_back(MENU_SHORT_CUT_KEY(key,bCtrl,bAlt,bShift,AccessMode));
}

void MENU_SHORT_CUT::RemoveItem  ( MENU_SHORT_CUT_KEY * ShortCut )
{
	for (SHORTCUT_KEY_LIST::iterator item = m_AccelList.begin(); item != m_AccelList.end(); item++) {
		if (ShortCut == &*item) {
			m_AccelList.erase(item);
			return;
		}
	}
	
}

