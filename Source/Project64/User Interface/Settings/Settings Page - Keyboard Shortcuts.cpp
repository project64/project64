#include "stdafx.h"
#include "Settings Page.h"

COptionsShortCutsPage::COptionsShortCutsPage (HWND hParent, const RECT & rcDispay ) :
	m_EnableReset(false)
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}

	SetDlgItemText(IDC_S_CPU_STATE,GS(ACCEL_CPUSTATE_TITLE));	
	SetDlgItemText(IDC_MENU_ITEM_TEXT,GS(ACCEL_MENUITEM_TITLE));	
	SetDlgItemText(IDC_S_CURRENT_KEYS,GS(ACCEL_CURRENTKEYS_TITLE));	
	SetDlgItemText(IDC_S_SELECT_SHORT,GS(ACCEL_SELKEY_TITLE));	
	SetDlgItemText(IDC_S_CURRENT_ASSIGN,GS(ACCEL_ASSIGNEDTO_TITLE));	
	SetDlgItemText(IDC_ASSIGN,GS(ACCEL_ASSIGN_BTN));	
	SetDlgItemText(IDC_REMOVE,GS(ACCEL_REMOVE_BTN));	

	m_CreateNewShortCut.AttachToDlgItem(m_hWnd,IDC_S_SELECT_SHORT);
	m_CpuState.Attach(GetDlgItem(IDC_C_CPU_STATE));
	m_MenuItems.Attach(GetDlgItem(IDC_MENU_ITEMS));
	m_CurrentKeys.Attach(GetDlgItem(IDC_CURRENT_KEYS));
	m_VirtualKeyList.Attach(GetDlgItem(IDC_VIRTUALKEY));

	m_MenuItems.ModifyStyle(0,TVS_SHOWSELALWAYS);

	m_CpuState.SetItemData(m_CpuState.AddString(GS(ACCEL_CPUSTATE_1)),CMenuShortCutKey::GAME_NOT_RUNNING);
	m_CpuState.SetItemData(m_CpuState.AddString(GS(ACCEL_CPUSTATE_3)),CMenuShortCutKey::GAME_RUNNING_WINDOW);
	m_CpuState.SetItemData(m_CpuState.AddString(GS(ACCEL_CPUSTATE_4)),CMenuShortCutKey::GAME_RUNNING_FULLSCREEN);
	m_CpuState.SetCurSel(0);
	
	int VirtualKeyListSize;
	VIRTUAL_KEY * VirtualKeyList = CMenuShortCutKey::VirtualKeyList(VirtualKeyListSize);
	for (int count = 0; count < VirtualKeyListSize; count++) 
	{
		m_VirtualKeyList.SetItemData(m_VirtualKeyList.AddString(VirtualKeyList[count].Name),VirtualKeyList[count].Key);
	}

	OnCpuStateChanged(LBN_SELCHANGE,IDC_C_CPU_STATE,GetDlgItem(IDC_C_CPU_STATE));
	CheckResetEnable();

}

void COptionsShortCutsPage::CheckResetEnable ( void )
{
	MSC_MAP & ShortCuts = m_ShortCuts.GetShortCuts();
	for (MSC_MAP::iterator Item = ShortCuts.begin(); Item != ShortCuts.end(); Item++) 
	{
		const SHORTCUT_KEY_LIST & ShortCutList = Item->second.GetAccelItems();
		for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item ++)
		{
			if (!ShortCut_item->Inactive() && !ShortCut_item->UserAdded())
			{
				continue;
			}
			m_EnableReset = true;
			return;
		}

	}
	m_EnableReset = false;
}

void COptionsShortCutsPage::OnCpuStateChanged(UINT Code, int id, HWND ctl)
{
	ACCESS_MODE AccessLevel = (ACCESS_MODE)m_CpuState.GetItemData(m_CpuState.GetCurSel());

	MSC_MAP & ShortCuts = m_ShortCuts.GetShortCuts();
	m_MenuItems.DeleteAllItems();

	for (MSC_MAP::iterator Item = ShortCuts.begin(); Item != ShortCuts.end(); Item++) 
	{
		ACCESS_MODE ItemMode = Item->second.AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}
		//find Parent
		HTREEITEM hParent = m_MenuItems.GetChildItem(TVI_ROOT);
		while (hParent) 
		{
			if (m_MenuItems.GetItemData(hParent) == Item->second.Section())
			{
				break;
			}
			hParent = m_MenuItems.GetNextSiblingItem(hParent);
		}

		if (hParent == NULL)
		{
			hParent = m_MenuItems.InsertItem(TVIF_TEXT | TVIF_PARAM,GS(Item->second.Section()),0,0,0,0,
				Item->second.Section(),TVI_ROOT,TVI_LAST);
		}

		stdstr str = GS(Item->second.Title());
		str.replace("&","");
		str.replace("...","");

		HTREEITEM hItem = m_MenuItems.InsertItem(TVIF_TEXT | TVIF_PARAM,str.c_str(),0,0,0,0,
			(DWORD_PTR)&Item->second,hParent,TVI_LAST);

		const SHORTCUT_KEY_LIST & ShortCutList = Item->second.GetAccelItems();
		for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item ++)
		{
			if (!ShortCut_item->Inactive() && !ShortCut_item->UserAdded())
			{
				continue;
			}
			m_MenuItems.SetItemState(hItem,TVIS_BOLD,TVIS_BOLD);
			m_MenuItems.SetItemState(hParent,TVIS_BOLD,TVIS_BOLD);
			break;
		}

	}
}

void COptionsShortCutsPage::OnRemoveClicked ( UINT Code, int id, HWND ctl )
{
	HTREEITEM hSelectedItem = m_MenuItems.GetSelectedItem();
	if (hSelectedItem == NULL) 
	{ 
		_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT));
		return; 
	}
	HTREEITEM hParent = m_MenuItems.GetParentItem(hSelectedItem);
	if (hParent == NULL)
	{
		_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT)); 
		return; 
	}

	CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hSelectedItem);

	ACCESS_MODE AccessLevel = (ACCESS_MODE)m_CpuState.GetItemData(m_CpuState.GetCurSel());
	
	//Make sure an item is selected
	int index = m_CurrentKeys.GetCurSel();
	if (index < 0) 
	{
		_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT)); 
		return; 
	}
	ShortCut->RemoveItem((CMenuShortCutKey *)m_CurrentKeys.GetItemData(index));
	m_MenuItems.SetItemState(hSelectedItem,TVIS_BOLD,TVIS_BOLD);
	m_MenuItems.SetItemState(hParent,TVIS_BOLD,TVIS_BOLD);
	m_EnableReset = true;
	
	RefreshShortCutOptions(hSelectedItem);
 	SendMessage(GetParent(),PSM_CHANGED ,(WPARAM)m_hWnd,0);
}

void COptionsShortCutsPage::OnDetectKeyClicked ( UINT Code, int id, HWND ctl )
{
	CloseHandle(CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)stInputGetKeys,this,0,NULL));
}

void COptionsShortCutsPage::OnAssignClicked ( UINT Code, int id, HWND ctl )
{
	//Get the virtual key info
	int index = m_VirtualKeyList.GetCurSel();
	if (index < 0) 
	{ 
		_Notify->DisplayError(GS(MSG_NO_SHORTCUT_SEL));
		return;
	}

	WORD key     = (WORD)SendDlgItemMessage(IDC_VIRTUALKEY,CB_GETITEMDATA,index,0);
	bool bCtrl   = (SendDlgItemMessage(IDC_CTL,BM_GETCHECK, 0,0)   == BST_CHECKED);
	bool bAlt    = (SendDlgItemMessage(IDC_ALT,BM_GETCHECK, 0,0)   == BST_CHECKED);
	bool bShift  = (SendDlgItemMessage(IDC_SHIFT,BM_GETCHECK, 0,0) == BST_CHECKED);
	
	ACCESS_MODE AccessLevel = (ACCESS_MODE)m_CpuState.GetItemData(m_CpuState.GetCurSel());


	HTREEITEM hSelectedItem = m_MenuItems.GetSelectedItem();
	if (hSelectedItem == NULL) 
	{ 
		_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL)); 
		return; 
	}
	HTREEITEM hParent = m_MenuItems.GetParentItem(hSelectedItem);
	if (hParent == NULL)
	{
		_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL)); 
		return; 
	}

	CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hSelectedItem);

	LanguageStringID strid = m_ShortCuts.GetMenuItemName(key,bCtrl,bAlt,bShift,AccessLevel);
	if (strid != EMPTY_STRING) 
	{
		_Notify->DisplayError(GS(MSG_MENUITEM_ASSIGNED));
		return;
	}
	ShortCut->AddShortCut(key,bCtrl,bAlt,bShift,AccessLevel,true,false);
	m_MenuItems.SetItemState(hSelectedItem,TVIS_BOLD,TVIS_BOLD);
	m_MenuItems.SetItemState(hParent,TVIS_BOLD,TVIS_BOLD);
	m_EnableReset = true;
	
	RefreshShortCutOptions(hSelectedItem);
 	SendMessage(GetParent(),PSM_CHANGED ,(WPARAM)m_hWnd,0);
}

void COptionsShortCutsPage::OnShortCutChanged ( UINT Code, int id, HWND ctl )
{
	//Get the virtual key info
	int index = m_VirtualKeyList.GetCurSel();
	if (index < 0) { return; }
	WORD key    = (WORD)m_VirtualKeyList.GetItemData(index);
	bool bCtrl  = (SendDlgItemMessage(IDC_CTL,BM_GETCHECK, 0,0)   == BST_CHECKED);
	bool bAlt   = (SendDlgItemMessage(IDC_ALT,BM_GETCHECK, 0,0)   == BST_CHECKED);
	bool bShift = (SendDlgItemMessage(IDC_SHIFT,BM_GETCHECK, 0,0) == BST_CHECKED);

	ACCESS_MODE AccessLevel = (ACCESS_MODE)m_CpuState.GetItemData(m_CpuState.GetCurSel());

	stdstr str = GS(m_ShortCuts.GetMenuItemName(key,bCtrl,bAlt,bShift,AccessLevel));
	if (str.length() > 0)
	{
		str.resize(std::remove(str.begin(), str.end(), '&') - str.begin());
	} else {
		str = "None";
	}
	SetDlgItemText(IDC_ASSIGNED_MENU_ITEM,str.c_str());
}

LRESULT COptionsShortCutsPage::OnMenuItemChanged ( LPNMHDR lpnmh )
{
	RefreshShortCutOptions(((LPNMTREEVIEW)lpnmh)->itemNew.hItem);
	return true;
}

void COptionsShortCutsPage::RefreshShortCutOptions ( HTREEITEM hItem )
{
	HTREEITEM hParent = m_MenuItems.GetParentItem(hItem);
	if (hParent == NULL)
	{
		return;
	}


	ACCESS_MODE AccessLevel = (ACCESS_MODE)m_CpuState.GetItemData(m_CpuState.GetCurSel());
	CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hItem);

	m_CurrentKeys.ResetContent();

	const SHORTCUT_KEY_LIST & ShortCutList = ShortCut->GetAccelItems();
	for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item ++)
	{
		if (ShortCut_item->Inactive())
		{
			continue;
		}

		ACCESS_MODE ItemMode = ShortCut_item->AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}
		stdstr Name = ShortCut_item->Name();
		m_CurrentKeys.SetItemData(m_CurrentKeys.AddString(Name.c_str()),(DWORD_PTR)&*ShortCut_item);
	}
}


BOOL CALLBACK KeyPromptDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch (uMsg) 
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDCANCEL:
			SetForegroundWindow(GetParent(hDlg));
			DestroyWindow(hDlg);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void COptionsShortCutsPage::InputGetKeys (void) 
{
	HWND hKeyDlg = CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_Key_Prompt),m_hWnd,KeyPromptDlgProc,(LPARAM)::GetDlgItem(m_hWnd,IDC_VIRTUALKEY));
	::EnableWindow(GetParent(),false);
	MSG msg;

	for(bool fDone=false;!fDone;MsgWaitForMultipleObjects(0,NULL,false,45,QS_ALLINPUT)) {
		while(PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				fDone = true;
				::PostMessage(NULL,WM_QUIT,0,0);
				break;
			}
			if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ) {
				int nVirtKey = (int)msg.wParam;
				DWORD lKeyData = msg.lParam;
				if (nVirtKey == VK_SHIFT) { continue; }
				if (nVirtKey == VK_CONTROL) { continue; }
				if (nVirtKey == VK_MENU) { continue; }
				SendDlgItemMessage(IDC_VIRTUALKEY,CB_SETCURSEL,-1,0);
				for (int count = 0; count < SendDlgItemMessage(IDC_VIRTUALKEY,CB_GETCOUNT,0,0); count++) {
					int Data = (int)SendDlgItemMessage(IDC_VIRTUALKEY,CB_GETITEMDATA,count,0);
					if (Data != nVirtKey) { continue; }
					SendDlgItemMessage(IDC_VIRTUALKEY,CB_SETCURSEL,count,0);
					SendDlgItemMessage(IDC_CTL,BM_SETCHECK, (GetKeyState(VK_CONTROL) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendDlgItemMessage(IDC_ALT,BM_SETCHECK, (GetKeyState(VK_MENU) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendDlgItemMessage(IDC_SHIFT,BM_SETCHECK, (GetKeyState(VK_SHIFT) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendMessage(WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)::GetDlgItem(m_hWnd,IDC_VIRTUALKEY));
					SetForegroundWindow(GetParent());
					::DestroyWindow(hKeyDlg);
				}
				continue;
			}
			if(!::IsDialogMessage(hKeyDlg,&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		
		if(!::IsWindow(hKeyDlg)) { fDone = true; }

	}
	::SetFocus(GetParent());
	::EnableWindow(GetParent(),true);
}

void COptionsShortCutsPage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void COptionsShortCutsPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void COptionsShortCutsPage::ApplySettings( bool UpdateScreen )
{
	m_ShortCuts.Save();
	_Settings->SaveBool(Info_ShortCutsChanged,true);
}

bool COptionsShortCutsPage::EnableReset ( void )
{
	return m_EnableReset;
}

void COptionsShortCutsPage::ResetPage()
{
	m_EnableReset = false;
	m_ShortCuts.Load(true);
	OnCpuStateChanged(LBN_SELCHANGE,IDC_C_CPU_STATE,GetDlgItem(IDC_C_CPU_STATE));
	SendMessage(GetParent(),PSM_CHANGED ,(WPARAM)m_hWnd,0);
	m_CurrentKeys.ResetContent();
	CSettingsPageImpl<COptionsShortCutsPage>::ResetPage();
}
