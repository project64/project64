#include "stdafx.h"
#include <Project64-core/Settings/SettingType/SettingsType-Enhancements.h>

class CEditEnhancement :
	public CDialogImpl < CEditEnhancement >
{
public:
	BEGIN_MSG_MAP_EX(CEditEnhancement)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BTN_GAMESHARK, OnEditGameshark)
		COMMAND_ID_HANDLER(IDOK, OnOkCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	enum { IDD = IDD_Enhancement_Edit };

	CEditEnhancement(int EditItem); 

	void Display(HWND ParentWindow);
	LRESULT	OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditGameshark(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CEditEnhancement(void);                               // Disable default constructor
	CEditEnhancement(const CEditEnhancement&);            // Disable copy constructor
	CEditEnhancement& operator=(const CEditEnhancement&); // Disable assignment

	std::string GetDlgItemStr(int nIDDlgItem);

	int m_EditItem;
};

class CEditGS :
	public CDialogImpl < CEditGS >
{
public:
	BEGIN_MSG_MAP_EX(CEditEnhancement)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_CHEAT_CODES, EN_CHANGE, OnCheatChanged)
		COMMAND_ID_HANDLER(IDOK, OnOkCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	enum { IDD = IDD_Enhancement_GS };

	CEditGS(int EditItem);

	void Display(HWND ParentWindow);

	LRESULT	OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheatChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
	CEditGS(void);                               // Disable default constructor
	CEditGS(const CEditGS&);            // Disable copy constructor
	CEditGS& operator=(const CEditGS&); // Disable assignment

	bool CheatCode(std::string & Code);

	int m_EditItem;
};

CEnhancementConfig::CEnhancementConfig(void) :
	m_hSelectedItem(NULL)
{
}

CEnhancementConfig::~CEnhancementConfig()
{
}

void CEnhancementConfig::Display(void * ParentWindow)
{
    BOOL result = m_thunk.Init(NULL, NULL);
    if (result)
    {
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
#ifdef _DEBUG
        m_bModal = true;
#endif //_DEBUG
        ::DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD), (HWND)ParentWindow, StartDialogProc, NULL);
    }
}

LRESULT	CEnhancementConfig::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_TreeList.Attach(GetDlgItem(IDC_ENHANCEMENTLIST));
	LONG Style = m_TreeList.GetWindowLong(GWL_STYLE);
	m_TreeList.SetWindowLong(GWL_STYLE, TVS_CHECKBOXES | TVS_SHOWSELALWAYS | Style);

	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 40, 40);
	HBITMAP hBmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_TRI_STATE));
	ImageList_AddMasked(hImageList, hBmp, RGB(255, 0, 255));
	DeleteObject(hBmp);
	m_TreeList.SetImageList(hImageList, TVSIL_STATE);

	RefreshList();
	return TRUE;
}

LRESULT CEnhancementConfig::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return TRUE;
}

LRESULT CEnhancementConfig::OnAddEnhancement(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CEditEnhancement(-1).Display(m_hWnd);
	RefreshList();
	return TRUE;
}

LRESULT CEnhancementConfig::OnEnhancementListClicked(NMHDR* pNMHDR)
{
	SetMsgHandled(false);

	TVHITTESTINFO ht = { 0 };
	uint32_t dwpos = GetMessagePos();

	// include <windowsx.h> and <windows.h> header files
	ht.pt.x = GET_X_LPARAM(dwpos);
	ht.pt.y = GET_Y_LPARAM(dwpos);
	::MapWindowPoints(HWND_DESKTOP, pNMHDR->hwndFrom, &ht.pt, 1);

	TreeView_HitTest(pNMHDR->hwndFrom, &ht);

	if (TVHT_ONITEMSTATEICON & ht.flags)
	{
		switch (TV_GetCheckState(ht.hItem))
		{
		case TV_STATE_CLEAR:
		case TV_STATE_INDETERMINATE:
			TV_SetCheckState(ht.hItem, TV_STATE_CHECKED);
			ChangeChildrenStatus(ht.hItem, true);
			CheckParentStatus(m_TreeList.GetParentItem(ht.hItem));
			break;
		case TV_STATE_CHECKED:
			TV_SetCheckState(ht.hItem, TV_STATE_CLEAR);
			ChangeChildrenStatus(ht.hItem, false);
			CheckParentStatus(m_TreeList.GetParentItem(ht.hItem));
			break;
		}
		switch (TV_GetCheckState(ht.hItem))
		{
		case TV_STATE_CHECKED: TV_SetCheckState(ht.hItem, TV_STATE_INDETERMINATE); break;
		case TV_STATE_CLEAR: TV_SetCheckState(ht.hItem, TV_STATE_CHECKED); break;
		case TV_STATE_INDETERMINATE: TV_SetCheckState(ht.hItem, TV_STATE_CLEAR); break;
		}
	}
	return TRUE;
}

void CEnhancementConfig::CheckParentStatus(HTREEITEM hParent)
{
	TV_CHECK_STATE CurrentState, InitialState;
	
	if (!hParent) { return; }
	HTREEITEM hItem = m_TreeList.GetChildItem(hParent);
	InitialState = (TV_CHECK_STATE)TV_GetCheckState(hParent);
	CurrentState = (TV_CHECK_STATE)TV_GetCheckState(hItem);

	while (hItem != NULL)
	{
		if (TV_GetCheckState(hItem) != CurrentState)
		{
			CurrentState = TV_STATE_INDETERMINATE;
			break;
		}
		hItem = m_TreeList.GetNextSiblingItem(hItem);
	}
	TV_SetCheckState(hParent, CurrentState);
	if (InitialState != CurrentState)
	{
		CheckParentStatus(m_TreeList.GetParentItem(hParent));
	}
}

LRESULT CEnhancementConfig::OnEnhancementListRClicked(NMHDR* pNMHDR)
{
	TVHITTESTINFO ht = { 0 };
	uint32_t dwpos = GetMessagePos();
	ht.pt.x = GET_X_LPARAM(dwpos);
	ht.pt.y = GET_Y_LPARAM(dwpos);
	::MapWindowPoints(HWND_DESKTOP, pNMHDR->hwndFrom, &ht.pt, 1);

	TreeView_HitTest(pNMHDR->hwndFrom, &ht);
	m_hSelectedItem = ht.hItem;

	POINT Mouse;
	GetCursorPos(&Mouse);
	HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ENHANCEMENT_MENU));
	HMENU hPopupMenu = GetSubMenu(hMenu, 0);

	if (m_hSelectedItem == NULL)
	{
		RemoveMenu(hPopupMenu, 3, MF_BYPOSITION);
		RemoveMenu(hPopupMenu, 2, MF_BYPOSITION);
		RemoveMenu(hPopupMenu, 1, MF_BYPOSITION);
	}

	TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_hWnd, NULL);
	DestroyMenu(hMenu);
	return TRUE;
}

LRESULT CEnhancementConfig::OnEnhancementListDblClicked(NMHDR * pNMHDR)
{
	TVHITTESTINFO ht = { 0 };
	uint32_t dwpos = GetMessagePos();

	ht.pt.x = GET_X_LPARAM(dwpos);
	ht.pt.y = GET_Y_LPARAM(dwpos);
	::MapWindowPoints(HWND_DESKTOP, pNMHDR->hwndFrom, &ht.pt, 1);

	TreeView_HitTest(pNMHDR->hwndFrom, &ht);

	if (TVHT_ONITEMLABEL & ht.flags)
	{
		TVITEM item;

		item.mask = TVIF_PARAM;
		item.hItem = ht.hItem;
		m_TreeList.GetItem(&item);

		CEditEnhancement(item.lParam).Display(m_hWnd);
		RefreshList();
	}
	return TRUE;
}

LRESULT CEnhancementConfig::OnEnhancementListSelChanged(NMHDR * /*pNMHDR*/)
{
	HTREEITEM hItem = m_TreeList.GetSelectedItem();
	if (m_TreeList.GetChildItem(hItem) == NULL)
	{
		TVITEM item;

		item.mask = TVIF_PARAM;
		item.hItem = hItem;
		m_TreeList.GetItem(&item);

		std::string Notes(g_Settings->LoadStringIndex(Enhancement_Notes, item.lParam));
		SetDlgItemText(IDC_NOTES, Notes.c_str());
	}
	else
	{
		SetDlgItemText(IDC_NOTES, "");
	}
	return TRUE;
}

LRESULT CEnhancementConfig::OnEditItem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_hSelectedItem != NULL)
	{
		TVITEM item;

		item.mask = TVIF_PARAM;
		item.hItem = m_hSelectedItem;
		m_TreeList.GetItem(&item);

		CEditEnhancement(item.lParam).Display(m_hWnd);
		RefreshList();
	}
	return TRUE;
}

LRESULT CEnhancementConfig::OnDeleteItem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_hSelectedItem == NULL)
	{
		return TRUE;
	}
	int Response = MessageBoxW(m_hWnd, wGS(MSG_DEL_SURE).c_str(), wGS(MSG_DEL_TITLE).c_str(), MB_YESNO | MB_ICONQUESTION);
	if (Response != IDYES) { return TRUE; }

	TVITEM item;
	item.hItem = m_hSelectedItem;
	item.mask = TVIF_PARAM;
	m_TreeList.GetItem(&item);

	for (int i = item.lParam; i < CCheats::MaxCheats; i++)
	{
		stdstr Name = g_Settings->LoadStringIndex(Enhancement_Name, i + 1);
		if (Name.empty())
		{
			g_Settings->DeleteSettingIndex(Enhancement_Name, i);
			g_Settings->DeleteSettingIndex(Enhancement_Active, i);
			g_Settings->DeleteSettingIndex(Enhancement_OnByDefault, i);
			g_Settings->DeleteSettingIndex(Enhancement_Notes, i);
			g_Settings->DeleteSettingIndex(Enhancement_Overclock, i);
			g_Settings->DeleteSettingIndex(Enhancement_OverclockValue, i);
			g_Settings->DeleteSettingIndex(Enhancement_Gameshark, i);
			g_Settings->DeleteSettingIndex(Enhancement_GamesharkCode, i);
			break;
		}
		stdstr Value;
		if (g_Settings->LoadStringIndex(Enhancement_Gameshark, i + 1, Value))
		{
			g_Settings->SaveStringIndex(Enhancement_Gameshark, i, Value);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_Gameshark, i);
		}

		if (g_Settings->LoadStringIndex(Enhancement_Notes, i + 1, Value))
		{
			g_Settings->SaveStringIndex(Enhancement_Notes, i, Value);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_Notes, i);
		}

		bool bValue;
		if (g_Settings->LoadBoolIndex(Enhancement_Active, i + 1, bValue))
		{
			g_Settings->SaveBoolIndex(Enhancement_Active, i, bValue);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_Active, i);
		}

		if (g_Settings->LoadBoolIndex(Enhancement_OnByDefault, i + 1, bValue))
		{
			g_Settings->SaveBoolIndex(Enhancement_OnByDefault, i, bValue);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_OnByDefault, i);
		}

		if (g_Settings->LoadBoolIndex(Enhancement_Overclock, i + 1, bValue))
		{
			g_Settings->SaveBoolIndex(Enhancement_Overclock, i, bValue);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_Overclock, i);
		}

		if (g_Settings->LoadBoolIndex(Enhancement_Gameshark, i + 1, bValue))
		{
			g_Settings->SaveBoolIndex(Enhancement_Gameshark, i, bValue);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_Gameshark, i);
		}

		uint32_t dwValue;
		if (g_Settings->LoadDwordIndex(Enhancement_OverclockValue, i + 1, dwValue))
		{
			g_Settings->SaveDwordIndex(Enhancement_OverclockValue, i, dwValue);
		}
		else
		{
			g_Settings->DeleteSettingIndex(Enhancement_OverclockValue, i);
		}
		g_Settings->SaveStringIndex(Enhancement_Name, i, Name);
	}
	CSettingTypeEnhancements::FlushChanges();
	RefreshList();
	return TRUE;
}


void CEnhancementConfig::RefreshList()
{
	m_TreeList.DeleteAllItems();
 	for (int i = 0; i < CCheats::MaxCheats; i++)
	{
		std::string Name = g_Settings->LoadStringIndex(Enhancement_Name, i);
		if (Name.length() == 0) { break; }

		AddCodeLayers(i, Name, TVI_ROOT, g_Settings->LoadBoolIndex(Enhancement_Active, i) != 0);
	}
}

void CEnhancementConfig::AddCodeLayers(int index, const std::string & Name, HTREEITEM hParent, bool Active)
{
	TV_INSERTSTRUCT tv;

	//Work out text to add
	char Text[500], Item[500];
	if (Name.length() > (sizeof(Text) - 5)) { g_Notify->BreakPoint(__FILE__, __LINE__); }
	strcpy(Text, Name.c_str());
	if (strchr(Text, '\\') > 0) { *strchr(Text, '\\') = 0; }

	//See if text is already added
	tv.item.mask = TVIF_TEXT;
	tv.item.pszText = Item;
	tv.item.cchTextMax = sizeof(Item);
	tv.item.hItem = m_TreeList.GetChildItem(hParent);
	while (tv.item.hItem)
	{
		m_TreeList.GetItem(&tv.item);
		if (strcmp(Text, Item) == 0)
		{
			//If already exists then just use existing one
			int State = TV_GetCheckState(tv.item.hItem);
			if ((Active && State == TV_STATE_CLEAR) || (!Active && State == TV_STATE_CHECKED))
			{
				TV_SetCheckState(tv.item.hItem, TV_STATE_INDETERMINATE);
			}
			size_t StartPos = strlen(Text) + 1;
			stdstr TempCheatName;
			if (StartPos < Name.length())
			{
				TempCheatName = Name.substr(StartPos);
			}
			AddCodeLayers(index, TempCheatName, tv.item.hItem, Active);
			return;
		}
		tv.item.hItem = TreeView_GetNextSibling(m_TreeList, tv.item.hItem);
	}

	//Add to dialog
	tv.hInsertAfter = TVI_SORT;
	tv.item.mask = TVIF_TEXT | TVIF_PARAM;
	tv.item.pszText = Text;
	tv.item.lParam = index;
	tv.hParent = (HTREEITEM)hParent;
	hParent = m_TreeList.InsertItem(&tv);
	TV_SetCheckState(hParent, Active ? TV_STATE_CHECKED : TV_STATE_CLEAR);

	if (strcmp(Text, Name.c_str()) == 0) { return; }
	AddCodeLayers(index, (stdstr)(Name.substr(strlen(Text) + 1)), hParent, Active);
}

bool CEnhancementConfig::TV_SetCheckState( HTREEITEM hItem, TV_CHECK_STATE state)
{
	TVITEM tvItem;

	tvItem.mask = TVIF_HANDLE | TVIF_STATE;
	tvItem.hItem = hItem;
	tvItem.stateMask = TVIS_STATEIMAGEMASK;

	/*Image 1 in the tree-view check box image list is the
	unchecked box. Image 2 is the checked box.*/

	switch (state)
	{
	case TV_STATE_CHECKED: tvItem.state = INDEXTOSTATEIMAGEMASK(1); break;
	case TV_STATE_CLEAR: tvItem.state = INDEXTOSTATEIMAGEMASK(2); break;
	case TV_STATE_INDETERMINATE: tvItem.state = INDEXTOSTATEIMAGEMASK(3); break;
	default: tvItem.state = INDEXTOSTATEIMAGEMASK(0); break;
	}
	return m_TreeList.SetItem(&tvItem) != 0;
}

int CEnhancementConfig::TV_GetCheckState(HTREEITEM hItem)
{
	TVITEM tvItem;

	// Prepare to receive the desired information.
	tvItem.mask = TVIF_HANDLE | TVIF_STATE;
	tvItem.hItem = hItem;
	tvItem.stateMask = TVIS_STATEIMAGEMASK;

	// Request the information.
	m_TreeList.GetItem(&tvItem);

	// Return zero if it's not checked, or nonzero otherwise.
	switch (tvItem.state >> 12) {
	case 1: return TV_STATE_CHECKED;
	case 2: return TV_STATE_CLEAR;
	case 3: return TV_STATE_INDETERMINATE;
	}
	return ((int)(tvItem.state >> 12) - 1);
}

void CEnhancementConfig::ChangeChildrenStatus(HTREEITEM hParent, bool Checked)
{
	HTREEITEM hItem = m_TreeList.GetChildItem(hParent);
	if (hItem == NULL)
	{
		if (hParent == TVI_ROOT) { return; }

		TVITEM item;
		item.mask = TVIF_PARAM;
		item.hItem = (HTREEITEM)hParent;
		m_TreeList.GetItem(&item);

		TV_SetCheckState(hParent, Checked ? TV_STATE_CHECKED : TV_STATE_CLEAR);
		g_Settings->SaveBoolIndex(Enhancement_Active, item.lParam, Checked);
	}
	else
	{
		TV_CHECK_STATE state = TV_STATE_UNKNOWN;
		while (hItem != NULL)
		{
			TV_CHECK_STATE ChildState = (TV_CHECK_STATE)TV_GetCheckState(hItem);
			if ((ChildState != TV_STATE_CHECKED || !Checked) &&
				(ChildState != TV_STATE_CLEAR || Checked))
			{
				ChangeChildrenStatus(hItem, Checked);
			}
			ChildState = (TV_CHECK_STATE)TV_GetCheckState(hItem);
			if (state == TV_STATE_UNKNOWN) { state = ChildState; }
			if (state != ChildState) { state = TV_STATE_INDETERMINATE; }
			hItem = m_TreeList.GetNextSiblingItem(hItem);
		}
		if (state != TV_STATE_UNKNOWN)
		{
			TV_SetCheckState(hParent, state);
		}
	}
}

CEditEnhancement::CEditEnhancement(int EditItem) :
	m_EditItem(EditItem)
{
}

void CEditEnhancement::Display(HWND ParentWindow)
{
	BOOL result = m_thunk.Init(NULL, NULL);
	if (result)
	{
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
#ifdef _DEBUG
		m_bModal = true;
#endif //_DEBUG
		::DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD), ParentWindow, StartDialogProc, NULL);
	}
}

LRESULT	CEditEnhancement::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	GetDlgItem(IDC_CODE_NAME).SetWindowTextA(m_EditItem >= 0 ? g_Settings->LoadStringIndex(Enhancement_Name, m_EditItem).c_str() : "");
	GetDlgItem(IDC_NOTES).SetWindowTextA(m_EditItem >= 0 ? g_Settings->LoadStringIndex(Enhancement_Notes, m_EditItem).c_str() : "");
	CButton(GetDlgItem(IDC_AUTOON)).SetCheck(g_Settings->LoadBoolIndex(Enhancement_OnByDefault, m_EditItem) ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_OVERCLOCK)).SetCheck(g_Settings->LoadBoolIndex(Enhancement_Overclock, m_EditItem) ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_GAMESHARK)).SetCheck(g_Settings->LoadBoolIndex(Enhancement_Gameshark, m_EditItem) ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_OVER_CLOCK_MODIFIER).SetWindowTextA(m_EditItem >= 0 ? stdstr_f("%d", g_Settings->LoadDwordIndex(Enhancement_OverclockValue, m_EditItem)).c_str() : "");
	return TRUE;
}

LRESULT CEditEnhancement::OnEditGameshark(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CEditGS(m_EditItem).Display(m_hWnd);
	return TRUE;
}

LRESULT CEditEnhancement::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return TRUE;
}

LRESULT CEditEnhancement::OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	std::string NewName = GetDlgItemStr(IDC_CODE_NAME);

	for (int i = 0; i < CCheats::MaxCheats; i++)
	{
		if (m_EditItem == i)
		{
			continue;
		}
		std::string Name = g_Settings->LoadStringIndex(Enhancement_Name, i);
		if (Name.length() == 0)
		{
			if (m_EditItem < 0)
			{
				m_EditItem = i;
			}
			break;
		}
		if (_stricmp(Name.c_str(), NewName.c_str()) == 0)
		{
			g_Notify->DisplayWarning(GS(MSG_CHEAT_NAME_IN_USE));
			GetDlgItem(IDC_CODE_NAME).SetFocus();
			return true;
		}
	}
	if (m_EditItem < 0)
	{
		g_Notify->DisplayError(GS(MSG_MAX_CHEATS));
		return true;
	}
	g_Settings->SaveStringIndex(Enhancement_Name, m_EditItem, NewName);
	g_Settings->SaveStringIndex(Enhancement_Notes, m_EditItem, GetDlgItemStr(IDC_NOTES));
	g_Settings->SaveBoolIndex(Enhancement_OnByDefault, m_EditItem, CButton(GetDlgItem(IDC_AUTOON)).GetCheck() == 1);
	g_Settings->SaveBoolIndex(Enhancement_Overclock, m_EditItem, CButton(GetDlgItem(IDC_OVERCLOCK)).GetCheck() == 1);
	g_Settings->SaveDwordIndex(Enhancement_OverclockValue, m_EditItem, atoi(GetDlgItemStr(IDC_OVER_CLOCK_MODIFIER).c_str()));
	g_Settings->SaveBoolIndex(Enhancement_Gameshark, m_EditItem, CButton(GetDlgItem(IDC_GAMESHARK)).GetCheck() == 1);
	CSettingTypeEnhancements::FlushChanges();
	EndDialog(wID);
	return TRUE;
}

std::string CEditEnhancement::GetDlgItemStr(int nIDDlgItem)
{
	CWindow DlgItem = GetDlgItem(nIDDlgItem);
	int length = DlgItem.SendMessage(WM_GETTEXTLENGTH, 0, 0);
	if (length == 0)
	{
		return "";
	}

	stdstr Result;
	Result.resize(length + 1);

	DlgItem.GetWindowText((char *)Result.c_str(), Result.length());
	return Result;
}

CEditGS::CEditGS(int EditItem) :
	m_EditItem(EditItem)
{
}

void CEditGS::Display(HWND ParentWindow)
{
	BOOL result = m_thunk.Init(NULL, NULL);
	if (result)
	{
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
#ifdef _DEBUG
		m_bModal = true;
#endif //_DEBUG
		::DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD), (HWND)ParentWindow, StartDialogProc, NULL);
	}
}

LRESULT CEditGS::OnCheatChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	std::string code;
	GetDlgItem(IDOK).EnableWindow(CheatCode(code));
	return true;
}

LRESULT CEditGS::OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	std::string code;
	if (CheatCode(code))
	{
		if (code.empty())
		{
			g_Settings->DeleteSettingIndex(Enhancement_GamesharkCode, m_EditItem);
		}
		else
		{
			g_Settings->SaveStringIndex(Enhancement_GamesharkCode, m_EditItem, code.c_str());
		}
	}
	EndDialog(wID);
	return TRUE;
}

LRESULT	CEditGS::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	std::string entry = g_Settings->LoadStringIndex(Enhancement_GamesharkCode, m_EditItem);
	std::string Buffer;
	const char * ReadPos = entry.c_str();
	do
	{
		const char * End = strchr(ReadPos, ',');
		if (End)
		{
			Buffer.append(ReadPos, End - ReadPos);
		}
		else
		{
			Buffer.append(ReadPos);
		}

		ReadPos = strchr(ReadPos, ',');
		if (ReadPos != NULL)
		{
			Buffer.append("\r\n");
			ReadPos += 1;
		}
	} while (ReadPos);
	CWindow Code = GetDlgItem(IDC_CHEAT_CODES);
	Code.SetWindowText(Buffer.c_str());
	Code.SetFocus();
	Code.PostMessage(EM_SETSEL, (WPARAM)-1, 0);
	return TRUE;
}


LRESULT CEditGS::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return TRUE;
}

bool CEditGS::CheatCode(std::string & Code)
{
	Code.clear();

 	bool ValidCode = true;
	uint32_t numlines = SendDlgItemMessage(IDC_CHEAT_CODES, EM_GETLINECOUNT, 0, 0);

	for (uint32_t line = 0; line < numlines; line++) //read line after line (bypassing limitation GetDlgItemText)
	{
		char tempformat[128] = { 0 }, str[128] = { 0 };
		const char * formatnormal = "XXXXXXXX XXXX";

		*(LPWORD)str = sizeof(str);
		uint32_t len = SendDlgItemMessage(IDC_CHEAT_CODES, EM_GETLINE, (WPARAM)line, (LPARAM)(const char *)str);
		str[len] = 0;

		if (len <= 0) { continue; }

		for (uint32_t i = 0; i < sizeof(tempformat); i++)
		{
			if (isxdigit(str[i]))
			{
				tempformat[i] = 'X';
			}
			if (str[i] == ' ')
			{
				tempformat[i] = str[i];
			}
			if (str[i] == 0) { break; }
		}

		if (strcmp(tempformat, formatnormal) == 0)
		{
			if (!Code.empty())
			{
				Code += ",";
			}
			Code += str;
		}
		else
		{
			ValidCode = false;
			break;
		}
 	}
	return ValidCode;
}

