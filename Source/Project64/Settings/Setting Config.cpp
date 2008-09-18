#ifdef old
#include "..\\Multilanguage.h"
#include "..\\Settings.h"
#include "..\\Plugin.h"

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <algorithm>

#include "..\\User Interface\\resource.h"

BOOL CALLBACK AdvancedOptionsProc  ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK DirSelectProc        ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK PluginSelectProc     ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK GeneralOptionsProc   ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RomSettingsProc      ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RomStatusProc        ( HWND, UINT, WPARAM, LPARAM );
int  CALLBACK RomBrowserConfigProc ( DWORD, DWORD, DWORD, DWORD );
BOOL CALLBACK ShortCutConfigProc   ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK ShellIntegrationProc ( HWND, UINT, WPARAM, LPARAM );

class SETTINGS_TAB {
public:
	SETTINGS_TAB(LanguageStringID LanguageID, WORD TemplateID, DLGPROC pfnDlgProc) {
		this->LanguageID = LanguageID;
		this->TemplateID = TemplateID;
		this->pfnDlgProc = pfnDlgProc;
	}
	stdstr              Title;
	LanguageStringID	LanguageID;
	WORD				TemplateID;
	DLGPROC				pfnDlgProc;
} ;

typedef std::vector<SETTINGS_TAB> SETTINGS_TAB_VECTOR;

typedef struct {
	CN64System * System;
	CMainGui   * Gui;
} SETTING_CLASSES;

SETTINGS_TAB BasicSettingsTabs[] = {
	SETTINGS_TAB( TAB_PLUGIN,          IDD_Settings_PlugSel,   PluginSelectProc     ),
	SETTINGS_TAB( TAB_OPTIONS,         IDD_Settings_General,   GeneralOptionsProc   ),
};

SETTINGS_TAB AdvancedSettingsTabs[] = {
	SETTINGS_TAB( TAB_DIRECTORY,       IDD_Settings_Directory, DirSelectProc        ),
	SETTINGS_TAB( TAB_ADVANCED,        IDD_Settings_Advanced,  AdvancedOptionsProc  ),
	SETTINGS_TAB( TAB_ROMSELECTION,    IDD_Settings_RomBrowser,(DLGPROC)RomBrowserConfigProc ),
	SETTINGS_TAB( TAB_SHORTCUTS,       IDD_Settings_Accelerator,ShortCutConfigProc  ),
	SETTINGS_TAB( TAB_SHELLINTERGATION,IDD_Settings_ShellInt,  ShellIntegrationProc ),
};

SETTINGS_TAB RomSettingsTabs[] = {
	SETTINGS_TAB( TAB_ROMSETTINGS,     IDD_Settings_Rom,       RomSettingsProc      ),
	SETTINGS_TAB( TAB_ROMSTATUS,       IDD_Settings_RomStatus, RomStatusProc        ),
};

enum { MaxConfigPages = 40 };

/*void CSettings::Config (void * ParentWindow, CN64System * System, CMainGui * Gui) {
    if (_Lang == NULL) { return; }
	int count;
	
	SETTING_CLASSES Classes;
	Classes.System   = System;
	Classes.Gui      = Gui;
	
	SETTINGS_TAB_VECTOR Tabs;

	for (count = 0; count < (sizeof(BasicSettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
		Tabs.push_back(BasicSettingsTabs[count]);
	}
	if (LoadString(ROM_NAME).length() > 0) {
		if (LoadString(ROM_MD5).length() == 0) {
			CN64Rom * Rom = System->GetCurrentRom();
			if (Rom)
			{
				SaveDword(ROM_MD5,Rom->GetRomMD5().c_str());
				SaveDword(ROM_InternalName,Rom->GetRomName().c_str());
			}
		}
		for (count = 0; count < (sizeof(RomSettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
			Tabs.push_back(RomSettingsTabs[count]);
		}
	}
	if (!LoadDword(BasicMode)) {
		for (count = 0; count < (sizeof(AdvancedSettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
			Tabs.push_back(AdvancedSettingsTabs[count]);
		}
	}

	PROPSHEETPAGE psp[MaxConfigPages];

	for (count = 0; count < Tabs.size(); count ++) {
		Tabs[count].Title = _Lang->GetString(Tabs[count].LanguageID);
		psp[count].dwSize      = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags     = PSP_USETITLE;
		psp[count].hInstance   = GetModuleHandle(NULL);
		psp[count].pszTemplate = MAKEINTRESOURCE(Tabs[count].TemplateID);
		psp[count].pfnDlgProc  = Tabs[count].pfnDlgProc;
		psp[count].pszTitle    = Tabs[count].Title.c_str();
		psp[count].lParam      = (long)&Classes;
		psp[count].pfnCallback = NULL;
	}

    PROPSHEETHEADER psh;
	stdstr SettingTitle = _Lang->GetString(OPTIONS_TITLE);
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE;
    psh.hwndParent  = (HWND)ParentWindow;
    psh.hInstance   = GetModuleHandle(NULL);
    psh.pszCaption  = SettingTitle.c_str();
    psh.nPages      = Tabs.size();
    psh.nStartPage  = 0;
    psh.ppsp        = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

	PropertySheet(&psh);
}*/

/*void CSettings::ConfigRom (void * ParentWindow, CMainGui * Gui) 
{
    if (_Lang == NULL) { return; }
	int count;
	
	SETTING_CLASSES Classes;
	Classes.System   = NULL;
	Classes.Gui      = Gui;
	
	SETTINGS_TAB_VECTOR Tabs;

	for (count = 0; count < (sizeof(RomSettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
		Tabs.push_back(RomSettingsTabs[count]);
	}

	PROPSHEETPAGE psp[MaxConfigPages];

	for (count = 0; count < Tabs.size(); count ++) {
		Tabs[count].Title = _Lang->GetString(Tabs[count].LanguageID);
		psp[count].dwSize      = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags     = PSP_USETITLE;
		psp[count].hInstance   = GetModuleHandle(NULL);
		psp[count].pszTemplate = MAKEINTRESOURCE(Tabs[count].TemplateID);
		psp[count].pfnDlgProc  = Tabs[count].pfnDlgProc;
		psp[count].pszTitle    = _Lang->GetString(Tabs[count].LanguageID).c_str();
		psp[count].lParam      = (long)&Classes;
		psp[count].pfnCallback = NULL;
	}

    PROPSHEETHEADER psh;
	stdstr SettingTitle = _Lang->GetString(OPTIONS_TITLE);
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE;
    psh.hwndParent  = (HWND)ParentWindow;
    psh.hInstance   = GetModuleHandle(NULL);
    psh.pszCaption  = SettingTitle.c_str();
    psh.nPages      = Tabs.size();
    psh.nStartPage  = 0;
    psh.ppsp        = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

	PropertySheet(&psh);
}*/

typedef struct _PLUGIN_LIST{
	int       Type;
	WORD      ListID;
	WORD      AboutID;
	SettingID Setting_ID;
	SettingID ChangeSettingID;
	stdstr    CurrentPlugin;

	_PLUGIN_LIST (int _Type, WORD _ListID, WORD _AboutID, SettingID _Setting_ID, SettingID _ChangeSettingID, LPCSTR _CurrentPlugin) :
		Type(_Type),
		ListID(_ListID),
		AboutID(_AboutID),
		Setting_ID(_Setting_ID),
		ChangeSettingID(_ChangeSettingID),
		CurrentPlugin(_CurrentPlugin)
	{
	}
} PLUGIN_LIST;

PLUGIN_LIST Items[] = {
	PLUGIN_LIST(PLUGIN_TYPE_RSP,        RSP_LIST,   RSP_ABOUT,   CurrentRSP_Plugin,   RSP_PluginChanged,   "" ),
	PLUGIN_LIST(PLUGIN_TYPE_GFX,        GFX_LIST,   GFX_ABOUT,   CurrentGFX_Plugin,   GFX_PluginChanged,   "" ),
	PLUGIN_LIST(PLUGIN_TYPE_AUDIO,      AUDIO_LIST, AUDIO_ABOUT, CurrentAUDIO_Plugin, AUDIO_PluginChanged, "" ),
	PLUGIN_LIST(PLUGIN_TYPE_CONTROLLER, CONT_LIST,  CONT_ABOUT,  CurrentCONT_Plugin,  CONT_PluginChanged,  "" ),
};

int AddDropDownItem (HWND hDlg, WORD CtrlID, LPCSTR StringID, int ItemData, DWORD * Variable) {
	HWND hCtrl = GetDlgItem(hDlg,CtrlID);
	int indx;

	indx = SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM)StringID);
	SendMessage(hCtrl,CB_SETITEMDATA,indx,ItemData);
	if (*Variable == ItemData) { SendMessage(hCtrl,CB_SETCURSEL,indx,0); }
	if (SendMessage(hCtrl,CB_GETCOUNT,0,0) == 1) { SendMessage(hCtrl,CB_SETCURSEL,0,0); }
	return indx;
}

void SetFlagControl (HWND hDlg, bool Flag, WORD CtrlID, const char * Name) {
	SetDlgItemText(hDlg,CtrlID,Name);	
	if (Flag) { SendMessage(GetDlgItem(hDlg,CtrlID),BM_SETCHECK, BST_CHECKED,0); }
}

BOOL CALLBACK AdvancedOptionsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			
			SetDlgItemText(hDlg,IDC_CORE_DEFAULTS,GS(ADVANCE_DEFAULTS));	
			
			//Self mod method
			SetFlagControl(hDlg,_Settings->LoadDword(SYSTEM_SMM_Cache) != 0,     IDC_SMM_CACHE, GS(ADVANCE_SMM_CACHE));
			SetFlagControl(hDlg,_Settings->LoadDword(SYSTEM_SMM_PIDMA) != 0,     IDC_SMM_DMA, GS(ADVANCE_SMM_PIDMA));
			SetFlagControl(hDlg,_Settings->LoadDword(SYSTEM_SMM_ValidFunc) != 0, IDC_SMM_VALIDATE, GS(ADVANCE_SMM_VALIDATE));
			SetFlagControl(hDlg,_Settings->LoadDword(SYSTEM_SMM_Protect) != 0,   IDC_SMM_PROTECT, GS(ADVANCE_SMM_PROTECT));
			SetFlagControl(hDlg,_Settings->LoadDword(SYSTEM_SMM_TLB) != 0,       IDC_SMM_TLB, GS(ADVANCE_SMM_TLB));
			
			SetFlagControl(hDlg,_Settings->LoadDword(AutoStart) != 0, IDC_START_ON_ROM_OPEN, GS(ADVANCE_AUTO_START));
			SetFlagControl(hDlg,_Settings->LoadDword(AutoZip) != 0, IDC_ZIP, GS(ADVANCE_COMPRESS));
			SetFlagControl(hDlg,_Settings->LoadDword(Debugger) != 0, IDC_DEBUGGER, GS(ADVANCE_DEBUGGER));

			DWORD CPU_Type = _Settings->LoadDword(SYSTEM_CPUType);
			AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_INTERPTER),CPU_Interpreter,&CPU_Type);
			AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_RECOMPILER),CPU_Recompiler,&CPU_Type);
			if (_Settings->LoadDword(Debugger)) { 
				AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_SYNC),CPU_SyncCores,&CPU_Type);
			}

			DWORD FunctionLookup = _Settings->LoadDword(SYSTEM_FunctionLookup);
			AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_PLOOKUP),FuncFind_PhysicalLookup,&FunctionLookup);
			AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_VLOOKUP),FuncFind_VirtualLookup,&FunctionLookup);
			//AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_CHANGEMEM),FuncFind_ChangeMemory,&FunctionLookup);
			
			DWORD RDRamSize = _Settings->LoadDword(SYSTEM_RDRamSize);
			AddDropDownItem(hDlg,IDC_RDRAM_SIZE,GS(RDRAM_4MB),0x400000,&RDRamSize);
			AddDropDownItem(hDlg,IDC_RDRAM_SIZE,GS(RDRAM_8MB),0x800000,&RDRamSize);

			DWORD SystemABL = _Settings->LoadDword(SYSTEM_BlockLinking);
			AddDropDownItem(hDlg,IDC_ABL,GS(ABL_ON),TRUE,(DWORD *)&SystemABL);
			AddDropDownItem(hDlg,IDC_ABL,GS(ABL_OFF),FALSE,(DWORD *)&SystemABL);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {			
		case IDC_CPU_TYPE:
		case IDC_RDRAM_SIZE:
		case IDC_FUNCFIND:
		case IDC_ABL:
			if (HIWORD(wParam) == LBN_SELCHANGE) { 
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_SMM_CACHE:
		case IDC_SMM_DMA:
		case IDC_SMM_VALIDATE:
		case IDC_SMM_PROTECT:
		case IDC_SMM_TLB:
		case IDC_START_ON_ROM_OPEN:
		case IDC_ZIP:
		case IDC_DEBUGGER:
			SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			break;

		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 		
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
			int indx;

			indx = SendDlgItemMessage(hDlg,IDC_CPU_TYPE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(SYSTEM_CPUType,SendDlgItemMessage(hDlg,IDC_CPU_TYPE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_RDRAM_SIZE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(SYSTEM_RDRamSize,SendDlgItemMessage(hDlg,IDC_RDRAM_SIZE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_FUNCFIND,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(SYSTEM_FunctionLookup,SendDlgItemMessage(hDlg,IDC_FUNCFIND,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_ABL,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(SYSTEM_BlockLinking,SendDlgItemMessage(hDlg,IDC_ABL,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));


			_Settings->SaveDword(SYSTEM_SMM_Cache,SendMessage(GetDlgItem(hDlg,IDC_SMM_CACHE),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(SYSTEM_SMM_PIDMA,SendMessage(GetDlgItem(hDlg,IDC_SMM_DMA),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(SYSTEM_SMM_ValidFunc,SendMessage(GetDlgItem(hDlg,IDC_SMM_VALIDATE),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(SYSTEM_SMM_Protect,SendMessage(GetDlgItem(hDlg,IDC_SMM_PROTECT),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(SYSTEM_SMM_TLB,SendMessage(GetDlgItem(hDlg,IDC_SMM_TLB),BM_GETSTATE, 0,0) == BST_CHECKED);
			
			_Settings->SaveDword(AutoStart,SendMessage(GetDlgItem(hDlg,IDC_START_ON_ROM_OPEN),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(AutoZip,SendMessage(GetDlgItem(hDlg,IDC_ZIP),BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(Debugger,SendMessage(GetDlgItem(hDlg,IDC_DEBUGGER),BM_GETSTATE, 0,0) == BST_CHECKED);

			CBaseMenu * Menu = Classes->Gui->GetMenuClass();
			Menu->ResetMenu();
		}
		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int CALLBACK SelectDirCallBack (WND_HANDLE hwnd,DWORD uMsg,DWORD lp, DWORD lpData) {
  switch(uMsg)
  {
    case BFFM_INITIALIZED:
      // WParam is TRUE since you are passing a path.
      // It would be FALSE if you were passing a pidl.
      if (lpData)
      {
        SendMessage((HWND)hwnd,BFFM_SETSELECTION,TRUE,lpData);
      }
      break;
  } 
  return 0;
}
BOOL CALLBACK KeyPromptDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {			
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

void InputGetKeys (HWND hDlg ) {
	HWND hKeyDlg = CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_Key_Prompt),hDlg,KeyPromptDlgProc,(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
	EnableWindow(GetParent(hDlg),false);
	MSG msg;

	for(bool fDone=false;!fDone;MsgWaitForMultipleObjects(0,NULL,false,45,QS_ALLINPUT)) {
		while(PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				fDone = true;
				PostMessage(NULL,WM_QUIT,0,0);
				break;
			}
			if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ) {
				int nVirtKey = (int)msg.wParam;
				DWORD lKeyData = msg.lParam;
				if (nVirtKey == VK_SHIFT) { continue; }
				if (nVirtKey == VK_CONTROL) { continue; }
				if (nVirtKey == VK_MENU) { continue; }
				SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_SETCURSEL,-1,0);
				for (int count = 0; count < SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETCOUNT,0,0); count++) {
					int Data = (int)SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETITEMDATA,count,0);
					if (Data != nVirtKey) { continue; }
					SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_SETCURSEL,count,0);
					SendDlgItemMessage(hDlg,IDC_CTL,BM_SETCHECK, (GetKeyState(VK_CONTROL) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendDlgItemMessage(hDlg,IDC_ALT,BM_SETCHECK, (GetKeyState(VK_MENU) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendDlgItemMessage(hDlg,IDC_SHIFT,BM_SETCHECK, (GetKeyState(VK_SHIFT) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
					SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
					SetForegroundWindow(GetParent(hDlg));
					DestroyWindow(hKeyDlg);
				}
				continue;
			}
			if(!IsDialogMessage(hKeyDlg,&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		
		if(!IsWindow(hKeyDlg)) { fDone = true; }

	}
	SetFocus(GetParent(hDlg));
	EnableWindow(GetParent(hDlg),true);
}

void RefreshShortCutOptions ( HWND hDlg, HTREEITEM hItem )
{
	SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_RESETCONTENT,0,0);

	HWND hTree = GetDlgItem(hDlg,IDC_MENU_ITEMS);

	HTREEITEM hParent = TreeView_GetParent(hTree,hItem);
	if (hParent == NULL)
	{
		return;
	}

	TVITEM item;
	item.mask  = TVIF_PARAM;
	item.hItem = (HTREEITEM)hItem;
	TreeView_GetItem(hTree,&item);

	int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
	int AccessLevel = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);
	
	MENU_SHORT_CUT * ShortCut = (MENU_SHORT_CUT *)item.lParam;
	for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCut->GetAccelItems().begin(); ShortCut_item != ShortCut->GetAccelItems().end(); ShortCut_item++) 
	{
		MENU_SHORT_CUT_KEY::ACCESS_MODE ItemMode = ShortCut_item->AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}
		stdstr Name = ShortCut_item->Name();
		int index = SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_ADDSTRING,0,(LPARAM)Name.c_str());
		SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_SETITEMDATA,index,(LPARAM)&*ShortCut_item);
	}
}

BOOL CALLBACK ShortCutConfigProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			CBaseMenu *Menu = Classes->Gui->GetMenuClass();
			
			MSC_MAP * ShortCuts = new MSC_MAP(Menu->GetShortCutInfo(false));
			SetProp((HWND)hDlg,"ShortCuts",(SETTING_CLASSES *)ShortCuts);

			SetDlgItemText(hDlg,IDC_S_CPU_STATE,GS(ACCEL_CPUSTATE_TITLE));	
			SetDlgItemText(hDlg,IDC_MENU_ITEM_TEXT,GS(ACCEL_MENUITEM_TITLE));	
			SetDlgItemText(hDlg,IDC_S_CURRENT_KEYS,GS(ACCEL_CURRENTKEYS_TITLE));	
			SetDlgItemText(hDlg,IDC_S_SELECT_SHORT,GS(ACCEL_SELKEY_TITLE));	
			SetDlgItemText(hDlg,IDC_S_CURRENT_ASSIGN,GS(ACCEL_ASSIGNEDTO_TITLE));	
			SetDlgItemText(hDlg,IDC_ASSIGN,GS(ACCEL_ASSIGN_BTN));	
			SetDlgItemText(hDlg,IDC_REMOVE,GS(ACCEL_REMOVE_BTN));	
			SetDlgItemText(hDlg,IDC_RESET,GS(ACCEL_RESETALL_BTN));	

			int index = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_ADDSTRING,0,(LPARAM)GS(ACCEL_CPUSTATE_1));
			SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_SETITEMDATA,index,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
			index = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_ADDSTRING,0,(LPARAM)GS(ACCEL_CPUSTATE_3));
			SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_SETITEMDATA,index,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
			SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_SETCURSEL,index,0);
			index = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_ADDSTRING,0,(LPARAM)GS(ACCEL_CPUSTATE_4));
			SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_SETITEMDATA,index,MENU_SHORT_CUT_KEY::GAME_RUNNING_FULLSCREEN);
			
			SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_C_CPU_STATE,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_C_CPU_STATE));
			
			HWND hTree = GetDlgItem(hDlg,IDC_MENU_ITEMS);
			DWORD Style = GetWindowLong(hTree,GWL_STYLE);					
			SetWindowLong(hTree,GWL_STYLE,TVS_SHOWSELALWAYS| Style);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {			
		case IDC_C_CPU_STATE:
			if (HIWORD(wParam) == LBN_SELCHANGE) 
			{ 
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
				CBaseMenu *Menu = Classes->Gui->GetMenuClass();

				int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
				int AccessLevel = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);
				MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
				

				HWND hTree = GetDlgItem((HWND)hDlg, IDC_MENU_ITEMS);

				TreeView_DeleteAllItems(hTree);
				for (MSC_MAP::iterator Item = ShortCuts->begin(); Item != ShortCuts->end(); Item++) 
				{
					MENU_SHORT_CUT_KEY::ACCESS_MODE ItemMode = Item->second.AccessMode();
					if ((ItemMode & AccessLevel) != AccessLevel )
					{
						continue;
					}

					//find Parent
					TV_INSERTSTRUCT tv;
					tv.item.mask       = TVIF_PARAM;
					tv.item.hItem      = TreeView_GetChild((HWND)hTree,TVI_ROOT);
					while (tv.item.hItem) 
					{
						TreeView_GetItem((HWND)hTree,&tv.item);
						if (tv.item.lParam == Item->second.Section())
						{
							break;
						}
						tv.item.hItem = TreeView_GetNextSibling(hTree,tv.item.hItem);
					}

					if (tv.item.hItem == NULL)
					{
						char Text[500];

						strcpy(Text,GS(Item->second.Section()));

						tv.item.mask       = TVIF_TEXT | TVIF_PARAM;
						tv.item.pszText    = Text;
						tv.item.lParam     = Item->second.Section();
						tv.item.cchTextMax = sizeof(Text);

						tv.hInsertAfter = TVI_LAST;
						tv.hParent = TVI_ROOT;
						tv.item.hItem = TreeView_InsertItem((HWND)hTree,&tv);
					}

					char Text[500];

					stdstr str = GS(Item->second.Title());
					str.resize(std::remove(str.begin(), str.end(), '&') - str.begin());

					tv.hParent = tv.item.hItem;
					tv.hInsertAfter = TVI_LAST;
					strcpy(Text,str.c_str());

					tv.item.mask       = TVIF_TEXT | TVIF_PARAM;
					tv.item.pszText    = Text;
					tv.item.lParam     = (WPARAM)&Item->second;
					tv.item.cchTextMax = sizeof(Text);

					tv.item.hItem = TreeView_InsertItem((HWND)hTree,&tv);
				}

				int VirtualKeyListSize;
				VIRTUAL_KEY *VirtualKeyList = MENU_SHORT_CUT_KEY::VirtualKeyList(VirtualKeyListSize);
				for (int count = 0; count < VirtualKeyListSize; count++) {
					int index = SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_ADDSTRING,0,(LPARAM)VirtualKeyList[count].Name);
					SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_SETITEMDATA,index,VirtualKeyList[count].Key);
				}
				//SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_MENU_ITEM_LIST,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_MENU_ITEM_LIST));
				SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
			}
			break;
		case IDC_KEY_PROMPT:
			{
				CloseHandle(CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)InputGetKeys,hDlg,0,NULL));
			}
			break;
		case IDC_ASSIGN:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
				CNotification * _Notify = Classes->Gui->GetNotifyClass();

				//Get the virtual key info
				int index = SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETCURSEL,0,0);
				if (index < 0) { _Notify->DisplayError(GS(MSG_NO_SHORTCUT_SEL)); break; }
				WORD key     = SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETITEMDATA,index,0);
				bool bCtrl   = (SendDlgItemMessage(hDlg,IDC_CTL,BM_GETCHECK, 0,0)   == BST_CHECKED);
				bool bAlt    = (SendDlgItemMessage(hDlg,IDC_ALT,BM_GETCHECK, 0,0)   == BST_CHECKED);
				bool bShift  = (SendDlgItemMessage(hDlg,IDC_SHIFT,BM_GETCHECK, 0,0) == BST_CHECKED);
				
				int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
				MENU_SHORT_CUT_KEY::ACCESS_MODE AccessLevel = (MENU_SHORT_CUT_KEY::ACCESS_MODE)SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);

				HWND hTree = GetDlgItem(hDlg,IDC_MENU_ITEMS);

				HTREEITEM hSelectedItem = TreeView_GetSelection(hTree);
				if (hSelectedItem == NULL) 
				{ 
					_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL)); 
					break; 
				}
				HTREEITEM hParent = TreeView_GetParent(hTree,hSelectedItem);
				if (hParent == NULL)
				{
					_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL)); 
					break;
				}

				TVITEM item;
				item.mask  = TVIF_PARAM;
				item.hItem = (HTREEITEM)hSelectedItem;
				TreeView_GetItem(hTree,&item);
			
				MENU_SHORT_CUT * ShortCut = (MENU_SHORT_CUT *)item.lParam;

				/*index = SendDlgItemMessage(hDlg,IDC_MENU_ITEM_LIST,LB_GETCURSEL,0,0);
				if (index < 0) { _Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL)); break; }
				*/		
				CBaseMenu *Menu = Classes->Gui->GetMenuClass();
				MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
				LanguageStringID strid = Menu->GetShortCutMenuItemName(ShortCuts,key,bCtrl,bAlt,bShift,AccessLevel);
				if (strid != EMPTY_STRING) 
				{
					_Notify->DisplayError(GS(MSG_MENUITEM_ASSIGNED));
					break;
				}

				ShortCut->AddShortCut(key,bCtrl,bAlt,bShift,AccessLevel);
				RefreshShortCutOptions(hDlg,hSelectedItem);
				//SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_MENU_ITEM_LIST,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_MENU_ITEM_LIST));
				//SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		//Remove a short cut key
		case IDC_REMOVE:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
				CNotification * _Notify = Classes->Gui->GetNotifyClass();

				HWND hTree = GetDlgItem(hDlg,IDC_MENU_ITEMS);

				HTREEITEM hSelectedItem = TreeView_GetSelection(hTree);
				if (hSelectedItem == NULL) 
				{ 
					_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT)); 
					break; 
				}
				HTREEITEM hParent = TreeView_GetParent(hTree,hSelectedItem);
				if (hParent == NULL)
				{
					_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT)); 
					break;
				}

				TVITEM item;
				item.mask  = TVIF_PARAM;
				item.hItem = (HTREEITEM)hSelectedItem;
				TreeView_GetItem(hTree,&item);

				int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
				int AccessLevel = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);
				
				MENU_SHORT_CUT * ShortCut = (MENU_SHORT_CUT *)item.lParam;


				//Male sure an item is selected
				int index = SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_GETCURSEL,0,0);
				if (index < 0) { _Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT)); break; }
				MENU_SHORT_CUT_KEY * ShortCutKey = (MENU_SHORT_CUT_KEY *)SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_GETITEMDATA,index,0);
				ShortCut->RemoveItem(ShortCutKey);
				RefreshShortCutOptions(hDlg,hSelectedItem);
				SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
//		case IDC_MENU_ITEM_LIST:
//			{
//				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
//				if (Classes == NULL) { break; }
//
//				if (HIWORD(wParam) != LBN_SELCHANGE) { break; }
//				SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_RESETCONTENT,0,0);
//
//				int index = SendDlgItemMessage(hDlg,IDC_MENU_ITEM_LIST,LB_GETCURSEL,0,0);
//				if (index < 0) { break; }
//
//				int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
//				int AccessLevel = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);
//
//				
//				MENU_SHORT_CUT * ShortCut = (MENU_SHORT_CUT *)SendDlgItemMessage(hDlg,IDC_MENU_ITEM_LIST,LB_GETITEMDATA,index,0);
//				for (SHORTCUT_KEY_LIST::const_iterator item = ShortCut->GetAccelItems().begin(); item != ShortCut->GetAccelItems().end(); item++) 
//				{
//					MENU_SHORT_CUT_KEY::ACCESS_MODE ItemMode = item->AccessMode();
//					if ((ItemMode & AccessLevel) != AccessLevel )
//					{
//						continue;
//					}
//					stdstr Name = item->Name();
//					index = SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_ADDSTRING,0,(LPARAM)Name.c_str());
//					SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_SETITEMDATA,index,(LPARAM)&*item);
//				}
//			}
//			break;
		case IDC_VIRTUALKEY:
			if (HIWORD(wParam) != LBN_SELCHANGE) { break; }
		case IDC_CTL:
		case IDC_ALT:
		case IDC_SHIFT:
//		case IDC_RUNNING:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

				//Get the virtual key info
				int index = SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETCURSEL,0,0);
				if (index < 0) { break; }
				WORD key    = SendDlgItemMessage(hDlg,IDC_VIRTUALKEY,CB_GETITEMDATA,index,0);
				bool bCtrl  = (SendDlgItemMessage(hDlg,IDC_CTL,BM_GETCHECK, 0,0)   == BST_CHECKED);
				bool bAlt   = (SendDlgItemMessage(hDlg,IDC_ALT,BM_GETCHECK, 0,0)   == BST_CHECKED);
				bool bShift = (SendDlgItemMessage(hDlg,IDC_SHIFT,BM_GETCHECK, 0,0) == BST_CHECKED);

				int SelIndex = SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETCURSEL,0,0);
				MENU_SHORT_CUT_KEY::ACCESS_MODE AccessLevel = (MENU_SHORT_CUT_KEY::ACCESS_MODE)SendDlgItemMessage(hDlg,IDC_C_CPU_STATE,CB_GETITEMDATA,SelIndex,0);

				CNotification * _Notify = Classes->Gui->GetNotifyClass();
				CBaseMenu *Menu = Classes->Gui->GetMenuClass();
				MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
				stdstr str = GS(Menu->GetShortCutMenuItemName(ShortCuts,key,bCtrl,bAlt,bShift,AccessLevel));
				str.resize(std::remove(str.begin(), str.end(), '&') - str.begin());
				if (str.length() == 0)
				{
					str = "None";
				}
				SetDlgItemText(hDlg,IDC_ASSIGNED_MENU_ITEM,str.c_str());

			}
			break;
		case IDC_RESET:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

				int result = MessageBox(NULL,GS(STR_SHORTCUT_RESET_TEXT),GS(STR_SHORTCUT_RESET_TITLE),
					MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
				if (result == IDNO) 
				{ 
					break;
				}

				CBaseMenu *Menu = Classes->Gui->GetMenuClass();

				//Remove current short cuts
				MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
				RemoveProp((HWND)hDlg,"ShortCuts");
				delete ShortCuts;
	
				// Recreate short cuts
				ShortCuts = new MSC_MAP(Menu->GetShortCutInfo(true));
				SetProp((HWND)hDlg,"ShortCuts",(SETTING_CLASSES *)ShortCuts);

				SendDlgItemMessage(hDlg,IDC_CURRENT_KEYS,LB_RESETCONTENT,0,0);
				SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_C_CPU_STATE,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_C_CPU_STATE));
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
//				SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_MENU_ITEM_LIST,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_MENU_ITEM_LIST));
				SendMessage(hDlg,WM_COMMAND,MAKELPARAM(IDC_VIRTUALKEY,LBN_SELCHANGE),(LPARAM)GetDlgItem(hDlg,IDC_VIRTUALKEY));
			}
			break;
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 		
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
			MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
			CBaseMenu * Menu = Classes->Gui->GetMenuClass();
			Menu->SaveShortCuts(ShortCuts);
			Menu->ResetMenu();
		}
		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			MSC_MAP * ShortCuts = (MSC_MAP *)GetProp((HWND)hDlg,"ShortCuts");
			delete ShortCuts;
			RemoveProp((HWND)hDlg,"ShortCuts");
			RemoveProp((HWND)hDlg,"Classes");
		}                        

		{
			LPNMHDR lpnmh = (LPNMHDR) lParam;
			if ((lpnmh->code  == TVN_SELCHANGED) && (lpnmh->idFrom == IDC_MENU_ITEMS))
			{
				LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
				RefreshShortCutOptions(hDlg,pnmtv->itemNew.hItem);

			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK DirSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;

			stdstr &TexturePath = _Settings->LoadString(TextureDir);

			SendMessage(GetDlgItem(hDlg,_Settings->LoadDword(UsePluginDirSelected)   ? IDC_PLUGIN_OTHER : IDC_PLUGIN_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
			SendMessage(GetDlgItem(hDlg,_Settings->LoadDword(UseSaveDirSelected)     ? IDC_AUTO_OTHER : IDC_AUTO_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
			SendMessage(GetDlgItem(hDlg,_Settings->LoadDword(UseInstantDirSelected)  ? IDC_INSTANT_OTHER : IDC_INSTANT_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
			SendMessage(GetDlgItem(hDlg,_Settings->LoadDword(UseSnapShotDirSelected) ? IDC_SNAP_OTHER : IDC_SNAP_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
			SendMessage(GetDlgItem(hDlg,TexturePath.empty() ? IDC_TEXTURE_DEFAULT : IDC_TEXTURE_OTHER),BM_SETCHECK, BST_CHECKED,0);
			
			SetDlgItemText(hDlg,IDC_PLUGIN_DIR,_Settings->LoadString(SelectedPluginDirectory).c_str());
			SetDlgItemText(hDlg,IDC_INSTANT_DIR,_Settings->LoadString(SelectedInstantSaveDirectory).c_str());
			SetDlgItemText(hDlg,IDC_AUTO_DIR,_Settings->LoadString(SelectedSaveDirectory).c_str());
			SetDlgItemText(hDlg,IDC_SNAP_DIR,_Settings->LoadString(SelectedSnapShotDir).c_str());
			SetDlgItemText(hDlg,IDC_TEXTURE_DIR,TexturePath.empty() ? _Settings->LoadString(InitialTextureDir).c_str() : TexturePath.c_str());

			//Set Text language for the dialog box
			SetDlgItemText(hDlg,IDC_DIR_FRAME1,GS(DIR_PLUGIN));
			SetDlgItemText(hDlg,IDC_DIR_FRAME3,GS(DIR_AUTO_SAVE));
			SetDlgItemText(hDlg,IDC_DIR_FRAME4,GS(DIR_INSTANT_SAVE));
			SetDlgItemText(hDlg,IDC_DIR_FRAME5,GS(DIR_SCREEN_SHOT));
			SetDlgItemText(hDlg,IDC_DIR_TEXTURE_FRAME,GS(DIR_TEXTURE));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {			
		case IDC_SELECT_PLUGIN_DIR:
		case IDC_SELECT_INSTANT_DIR:
		case IDC_SELECT_AUTO_DIR:
		case IDC_SELECT_SNAP_DIR:
		case IDC_SELECT_TEXTURE_DIR:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

				char Buffer[MAX_PATH], Directory[255];
				LPITEMIDLIST pidl;
				BROWSEINFO bi;

				//get the title of the select box
				char Title[255];
				switch (LOWORD(wParam)) {
				case IDC_SELECT_PLUGIN_DIR: strcpy(Title,GS(DIR_SELECT_PLUGIN)); break;
				case IDC_SELECT_AUTO_DIR: strcpy(Title,GS(DIR_SELECT_AUTO)); break;
				case IDC_SELECT_INSTANT_DIR: strcpy(Title,GS(DIR_SELECT_INSTANT)); break;
				case IDC_SELECT_SNAP_DIR: strcpy(Title,GS(DIR_SELECT_SCREEN)); break;
				case IDC_SELECT_TEXTURE_DIR: strcpy(Title,GS(DIR_SELECT_TEXTURE)); break;
				}

				//Get the initial Dir
				char InitialDir[_MAX_PATH];
				switch (LOWORD(wParam)) {
				case IDC_SELECT_PLUGIN_DIR: GetDlgItemText(hDlg,IDC_PLUGIN_DIR,InitialDir,sizeof(InitialDir)); break;
				case IDC_SELECT_AUTO_DIR: GetDlgItemText(hDlg,IDC_AUTO_DIR,InitialDir,sizeof(InitialDir)); break;
				case IDC_SELECT_INSTANT_DIR: GetDlgItemText(hDlg,IDC_INSTANT_DIR,InitialDir,sizeof(InitialDir)); break;
				case IDC_SELECT_SNAP_DIR: GetDlgItemText(hDlg,IDC_SNAP_DIR,InitialDir,sizeof(InitialDir)); break;
				case IDC_SELECT_TEXTURE_DIR: GetDlgItemText(hDlg,IDC_TEXTURE_DIR,InitialDir,sizeof(InitialDir)); break;
				}


				bi.hwndOwner = hDlg;
				bi.pidlRoot = NULL;
				bi.pszDisplayName = Buffer;
				bi.lpszTitle = Title;
				bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
				bi.lParam = (DWORD)InitialDir;
				if ((pidl = SHBrowseForFolder(&bi)) != NULL) {
					if (SHGetPathFromIDList(pidl, Directory)) {
						int len = strlen(Directory);

						if (Directory[len - 1] != '\\') { strcat(Directory,"\\"); }
						switch (LOWORD(wParam)) {
						case IDC_SELECT_PLUGIN_DIR: 
							SetDlgItemText(hDlg,IDC_PLUGIN_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_INSTANT_DIR: 
							SetDlgItemText(hDlg,IDC_INSTANT_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_INSTANT_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_INSTANT_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_AUTO_DIR: 
							SetDlgItemText(hDlg,IDC_AUTO_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_AUTO_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_AUTO_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_SNAP_DIR: 
							SetDlgItemText(hDlg,IDC_SNAP_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_SNAP_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_SNAP_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_TEXTURE_DIR: 
							SetDlgItemText(hDlg,IDC_TEXTURE_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_TEXTURE_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_TEXTURE_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						}
						SendMessage(GetParent(hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
					}
				}
			}
			break;
		
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

			//Current Dir
			stdstr strRomDir = _Settings->LoadString(RomDirectory);
			
			//Save all selected dirs
			char String[300];

			GetDlgItemText(hDlg,IDC_PLUGIN_DIR,String,sizeof(String));
			_Settings->SaveString(SelectedPluginDirectory,String);
			GetDlgItemText(hDlg,IDC_AUTO_DIR,String,sizeof(String));
			_Settings->SaveString(SelectedSaveDirectory,String);
			GetDlgItemText(hDlg,IDC_INSTANT_DIR,String,sizeof(String));
			_Settings->SaveString(SelectedInstantSaveDirectory,String);
			GetDlgItemText(hDlg,IDC_SNAP_DIR,String,sizeof(String));
			_Settings->SaveString(SelectedSnapShotDir,String);
			if (SendMessage(GetDlgItem(hDlg,IDC_TEXTURE_DEFAULT),BM_GETSTATE, 0,0) != BST_CHECKED)
			{
				GetDlgItemText(hDlg,IDC_TEXTURE_DIR,String,sizeof(String));
				_Settings->SaveString(TextureDir,String);
			} else {
				_Settings->SaveString(TextureDir,"");
			}

			//Save if using selected
			_Settings->SaveDword(UsePluginDirSelected,SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_DEFAULT),BM_GETSTATE, 0,0) != BST_CHECKED);
			_Settings->SaveDword(UseSaveDirSelected,SendMessage(GetDlgItem(hDlg,IDC_AUTO_DEFAULT),BM_GETSTATE, 0,0) != BST_CHECKED);
			_Settings->SaveDword(UseInstantDirSelected,SendMessage(GetDlgItem(hDlg,IDC_INSTANT_DEFAULT),BM_GETSTATE, 0,0) != BST_CHECKED);
			_Settings->SaveDword(UseSnapShotDirSelected,SendMessage(GetDlgItem(hDlg,IDC_SNAP_DEFAULT),BM_GETSTATE, 0,0) != BST_CHECKED);

			SettingID Dir;
			Dir = _Settings->LoadDword(UsePluginDirSelected) ? SelectedPluginDirectory : InitialPluginDirectory ;
			_Settings->SaveString(PluginDirectory,_Settings->LoadString(Dir).c_str());
			Dir = _Settings->LoadDword(UseSaveDirSelected) ? SelectedSaveDirectory : InitialSaveDirectory ;
			_Settings->SaveString(SaveDirectory,_Settings->LoadString(Dir).c_str());
			Dir = _Settings->LoadDword(UseInstantDirSelected) ? SelectedInstantSaveDirectory : InitialInstantSaveDirectory ;
			_Settings->SaveString(InstantSaveDirectory,_Settings->LoadString(Dir).c_str());
			Dir = _Settings->LoadDword(UseSnapShotDirSelected) ? SelectedSnapShotDir : InitialSnapShotDir ;
			_Settings->SaveString(SnapShotDir,_Settings->LoadString(Dir).c_str());
		
		}
		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	case WM_CLOSE:
		_asm int 3
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK PluginSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PluginList * List = NULL;
	int index;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;

			//Set the text for all gui Items
			SetDlgItemText(hDlg,RSP_ABOUT,GS(PLUG_ABOUT));
			SetDlgItemText(hDlg,GFX_ABOUT,GS(PLUG_ABOUT));
			SetDlgItemText(hDlg,AUDIO_ABOUT,GS(PLUG_ABOUT));
			SetDlgItemText(hDlg,CONT_ABOUT,GS(PLUG_ABOUT));

			SetDlgItemText(hDlg,IDC_RSP_NAME,GS(PLUG_RSP));
			SetDlgItemText(hDlg,IDC_GFX_NAME,GS(PLUG_GFX));
			SetDlgItemText(hDlg,IDC_AUDIO_NAME,GS(PLUG_AUDIO));
			SetDlgItemText(hDlg,IDC_CONT_NAME,GS(PLUG_CTRL));		
			
			SetDlgItemText(hDlg,IDC_HLE_GFX,GS(PLUG_HLE_GFX));
			SetDlgItemText(hDlg,IDC_HLE_AUDIO,GS(PLUG_HLE_AUDIO));		

			//Create the list of plugins
			CPluginList  Plugins(_Settings);
			List = new PluginList;
			*List = Plugins.GetPluginList();
			SetProp((HWND)hDlg,"Plugin List",List);
			
			for (int count = 0; count <  sizeof(Items) / sizeof(Items[0]); count++) {
				Items[count].CurrentPlugin = _Settings->LoadString(Items[count].Setting_ID);
			}

			//Add in all the plugins to the gui
			for (PluginList::iterator PluginIter = List->begin(); PluginIter != List->end(); PluginIter++) {
				PLUGIN * Plugin = &(*PluginIter);

				for (int count = 0; count <  sizeof(Items) / sizeof(Items[0]); count++) {
					if (Plugin->info.Type != Items[count].Type) { continue; }

					index = SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_ADDSTRING,(WPARAM)0, (LPARAM)&Plugin->info.Name);
					SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_SETITEMDATA ,(WPARAM)index, (LPARAM)Plugin);
					
					//See if added plugin is currently selected					
					if(_stricmp(Items[count].CurrentPlugin.c_str(),Plugin->FileName.c_str()) != 0) { continue; }
					SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_SETCURSEL,(WPARAM)index,(LPARAM)0);
					EnableWindow(GetDlgItem(hDlg,Items[count].AboutID),Plugin->InfoFunction);
					break;
				}
			}

			if (_Settings->LoadDword(UseHighLevelGfx)) { SendMessage(GetDlgItem((HWND)hDlg,IDC_HLE_GFX),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(UseHighLevelAudio)) { SendMessage(GetDlgItem((HWND)hDlg,IDC_HLE_AUDIO),BM_SETCHECK, BST_CHECKED,0); }

		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_HLE_GFX:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
				
				if ((SendMessage(GetDlgItem(hDlg,IDC_HLE_GFX),  BM_GETSTATE, 0,0) & BST_CHECKED) == 0)
				{
					stdstr Caption(GS(MSG_SET_LLE_GFX_TITLE)),Message(GS(MSG_SET_LLE_GFX_MSG));
					if (MessageBox(hDlg,Message.c_str(),Caption.c_str(),MB_OKCANCEL|MB_ICONWARNING) != IDOK)
					{
						SendMessage(GetDlgItem((HWND)hDlg,IDC_HLE_GFX),BM_SETCHECK, BST_CHECKED,0);
						break;
					}
				}
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_HLE_AUDIO:
			{
				SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
				
				if ((SendMessage(GetDlgItem(hDlg,IDC_HLE_AUDIO),  BM_GETSTATE, 0,0) & BST_CHECKED) != 0)
				{
					stdstr Caption(GS(MSG_SET_HLE_AUD_TITLE)),Message(GS(MSG_SET_HLE_AUD_MSG));
					if (MessageBox(hDlg,Message.c_str(),Caption.c_str(),MB_OKCANCEL|MB_ICONWARNING) != IDOK)
					{
						SendMessage(GetDlgItem((HWND)hDlg,IDC_HLE_AUDIO),BM_SETCHECK, BST_UNCHECKED,0);
						break;
					}
				}
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		}
		//Highlight the about button if the plugin has the dll about function
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			for (int count = 0; count <  sizeof(Items) / sizeof(Items[0]); count++) {
				if (LOWORD(wParam) != Items[count].ListID) { continue; }
				index = SendMessage(GetDlgItem(hDlg,LOWORD(wParam)),CB_GETCURSEL,0,0);
				if (index == CB_ERR) { break; }
				PLUGIN * Plugin = (PLUGIN *)SendMessage(GetDlgItem(hDlg,LOWORD(wParam)),CB_GETITEMDATA,(WPARAM)index,0);
				EnableWindow(GetDlgItem(hDlg,Items[count].AboutID),Plugin->InfoFunction);
				break;
			}
			SendMessage(GetParent(hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
		} else {
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
			
			//Check for about button pressed
			for (int count = 0; count <  sizeof(Items) / sizeof(Items[0]); count++) {
				if (LOWORD(wParam) != Items[count].AboutID) { continue; }

				index = SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_GETCURSEL,0,0);
				PLUGIN * Plugin = (PLUGIN *)SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_GETITEMDATA,(WPARAM)index,0);

				CPluginList Plugins(_Settings);

				Plugins.DllAbout((void *)hDlg,Plugin->FullPath.c_str());
			}
		}
		break;
	case WM_NOTIFY:
		//Save each selected plugin
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

			List = (PluginList *)GetProp(hDlg,"Plugin List");			

			bool PluginsChanged = false;
			for (int count = 0; count <  sizeof(Items) / sizeof(Items[0]); count++) {
				index = SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_GETCURSEL,0,0);
				if (index < 0) { continue; }
				PLUGIN * Plugin = (PLUGIN *)SendMessage(GetDlgItem(hDlg,Items[count].ListID),CB_GETITEMDATA,(WPARAM)index,0);
				if (_Settings->LoadString(Items[count].Setting_ID) != Plugin->FileName.c_str()) {
					PluginsChanged = true;
					_Settings->SaveString(Items[count].Setting_ID,Plugin->FileName.c_str());
					_Settings->SaveDword(Items[count].ChangeSettingID,true);
				}
			}

			if (_Settings->LoadDword(UseHighLevelGfx) != SendMessage(GetDlgItem(hDlg,IDC_HLE_GFX),  BM_GETSTATE, 0,0) == BST_CHECKED)
			{
				_Settings->SaveDword(UseHighLevelGfx,  SendMessage(GetDlgItem(hDlg,IDC_HLE_GFX),  BM_GETSTATE, 0,0) == BST_CHECKED);
				_Settings->SaveDword(Items[0].ChangeSettingID,true);
				PluginsChanged = true;

			}
			if (_Settings->LoadDword(UseHighLevelAudio) != SendMessage(GetDlgItem(hDlg,IDC_HLE_AUDIO),  BM_GETSTATE, 0,0) == BST_CHECKED)
			{
				_Settings->SaveDword(UseHighLevelAudio,SendMessage(GetDlgItem(hDlg,IDC_HLE_AUDIO),BM_GETSTATE, 0,0) == BST_CHECKED);
				_Settings->SaveDword(Items[0].ChangeSettingID,true);
				PluginsChanged = true;

			}

			if (PluginsChanged) {
				if (_Settings->LoadDword(CPU_Running) != 0) {
					Classes->System->ExternalEvent(ChangePlugins);
				} else {
					Classes->System->Plugins()->Reset();
					Classes->Gui->RefreshMenu();
				}
			}
		}

		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			List = (PluginList *)GetProp(hDlg,"Plugin List");
			delete List;
			
			RemoveProp((HWND)hDlg,"Plugin List");
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return false;
	}
	return true;
}

bool RomBrowserConfig_FieldsChanged (HWND hDlg,ROMBROWSER_FIELDS_LIST * Fields) {
	int ItemsSelected = 0;
	for (int count = 0; count < Fields->size(); count++) {
		if ((*Fields)[count].Pos >= 0) { ItemsSelected += 1; }
	}

	int listCount = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_GETCOUNT,0,0);
	if (listCount != ItemsSelected) { return true; }
	for (int Item = 0; Item < listCount; Item ++ ){
		int Pos = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETITEMDATA,Item,0);
		if ((*Fields)[Pos].Pos != Item) { return true; };
	}
	return false;
}

int CALLBACK RomBrowserConfigProc (DWORD hDlg, DWORD uMsg, DWORD wParam, DWORD lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			
			if (_Settings->LoadDword(RomBrowser)) { SendMessage(GetDlgItem((HWND)hDlg,IDC_USE_ROMBROWSER),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(RomBrowserRecursive)) { SendMessage(GetDlgItem((HWND)hDlg,IDC_RECURSION),BM_SETCHECK, BST_CHECKED,0); }

			ROMBROWSER_FIELDS_LIST * Fields = &Classes->Gui->m_Fields;
			
			for (int count = 0; count < Fields->size(); count ++) {
				int Pos = (*Fields)[count].Pos;
				if (Pos < 0) { 
					int index = SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_ADDSTRING,0,(LPARAM)GS((*Fields)[count].LangID));
					SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_SETITEMDATA,index,count);
					continue;
				}
				int listCount = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_GETCOUNT,0,0);
				if (Pos > listCount) { Pos = listCount; }
				int index = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_INSERTSTRING,Pos,(LPARAM)GS((*Fields)[count].LangID));
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETITEMDATA,index,count);
			}
			
			//char String[256];
			//sprintf(String,"%d",_Settings->LoadDword(RememberedRomFilesCount));
			//SetDlgItemText((HWND)hDlg,IDC_REMEMBER,String);
			//sprintf(String,"%d",_Settings->LoadDword(RememberedRomDir));
			//SetDlgItemText((HWND)hDlg,IDC_REMEMBERDIR,String);
			Notify().BreakPoint(__FILE__,__LINE__); 
			
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT1,GS(RB_MAX_ROMS));
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT2,GS(RB_ROMS));
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT3,GS(RB_MAX_DIRS));
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT4,GS(RB_DIRS));
			SetDlgItemText((HWND)hDlg,IDC_USE_ROMBROWSER,GS(RB_USE));
//			SetDlgItemText((HWND)hDlg,IDC_REFRESH_BROSWER,GS(RB_REFRESH));
			SetDlgItemText((HWND)hDlg,IDC_RECURSION,GS(RB_DIR_RECURSION));
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT5,GS(RB_AVALIABLE_FIELDS));
			SetDlgItemText((HWND)hDlg,IDC_ROMSEL_TEXT6,GS(RB_SHOW_FIELDS));
			SetDlgItemText((HWND)hDlg,IDC_ADD,GS(RB_ADD));
			SetDlgItemText((HWND)hDlg,IDC_REMOVE,GS(RB_REMOVE));
			SetDlgItemText((HWND)hDlg,IDC_UP,GS(RB_UP));
			SetDlgItemText((HWND)hDlg,IDC_DOWN,GS(RB_DOWN));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADD:
			{
				char String[100];
				int index, listCount, Data;

				index = SendMessage(GetDlgItem((HWND)hDlg,IDC_AVALIABLE),LB_GETCURSEL,0,0);
				if (index < 0) { break; }
				SendMessage(GetDlgItem((HWND)hDlg,IDC_AVALIABLE),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem((HWND)hDlg,IDC_AVALIABLE),LB_GETITEMDATA,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_DELETESTRING,index,0);
				listCount = SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_GETCOUNT,0,0);
				if (index >= listCount) { index -= 1;}
				SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_SETCURSEL,index,0);
				index = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_ADDSTRING,0,(LPARAM)String);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_REMOVE:
			{
				char String[100];
				int index, listCount, Data;

				index = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETCURSEL,0,0);
				if (index < 0) { break; }
				SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_DELETESTRING,index,0);
				listCount = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_GETCOUNT,0,0);
				if (index >= listCount) { index -= 1;}
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETCURSEL,index,0);
				index = SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_ADDSTRING,0,(LPARAM)String);
				SendDlgItemMessage((HWND)hDlg,IDC_AVALIABLE,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_UP:
			{
				char String[100];
				int index, Data;

				index = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETCURSEL,0,0);
				if (index <= 0) { break; }
				SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_DELETESTRING,index,0);
				index = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_INSERTSTRING,index - 1,(LPARAM)String);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETCURSEL,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_DOWN:
			{
				char String[100];
				int index,listCount,Data;

				index = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETCURSEL,0,0);
				if (index < 0) { break; }
				listCount = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_GETCOUNT,0,0);
				if ((index + 1) == listCount) { break; }
				SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_DELETESTRING,index,0);
				index = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_INSERTSTRING,index + 1,(LPARAM)String);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETCURSEL,index,0);
				SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_REMEMBER:
		case IDC_REMEMBERDIR:
			if (HIWORD(wParam) != EN_CHANGE) { break; }
		case IDC_USE_ROMBROWSER:
//		case IDC_REFRESH_BROSWER:
		case IDC_RECURSION:
			SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			break;
		}
		{
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
			if (RomBrowserConfig_FieldsChanged((HWND)hDlg,&Classes->Gui->m_Fields)) {
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
		}
		break;
	case WM_NOTIFY:
		//Save each selected plugin
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

			bool ResetMenu = false;

			Notify().BreakPoint(__FILE__,__LINE__); 

			/*int OldRememberedRomFiles = _Settings->LoadDword(RememberedRomFilesCount);
			int RomsToRemember = GetDlgItemInt((HWND)hDlg,IDC_REMEMBER,NULL,FALSE);
			if (OldRememberedRomFiles != RomsToRemember) {
				if (RomsToRemember < 0) { RomsToRemember = 0; }
				if (RomsToRemember > MaxRememberedFiles) { RomsToRemember = MaxRememberedFiles; }
				_Settings->SaveDword(RememberedRomFilesCount,RomsToRemember);
				ResetMenu = true;

				//Reset the amount to the max on the gui
				char String[100]; sprintf(String,"%d",RomsToRemember);
				SetDlgItemText((HWND)hDlg,IDC_REMEMBER,String);
			}
			
			int OldRememberedRomDirs = _Settings->LoadDword(RememberedRomDir);
			int DirsToRemember = GetDlgItemInt((HWND)hDlg,IDC_REMEMBERDIR,NULL,FALSE);
			if (OldRememberedRomDirs != DirsToRemember) {
				if (DirsToRemember < 0) { DirsToRemember = 0; }
				if (DirsToRemember > MaxRememberedDirs) { DirsToRemember = MaxRememberedDirs; }
				_Settings->SaveDword(RememberedRomDir,DirsToRemember);
				ResetMenu = true;

				//Reset the amount to the max on the gui
				char String[100]; sprintf(String,"%d",DirsToRemember);
				SetDlgItemText((HWND)hDlg,IDC_REMEMBERDIR,String);
			}*/
			//Get the field information
			ROMBROWSER_FIELDS_LIST * Fields = &Classes->Gui->m_Fields;
		
			bool ResetColoums = false;
			bool RefreshData  = false;
			
			//Handle changes in fields
			if (RomBrowserConfig_FieldsChanged((HWND)hDlg,Fields)) {
				//So we save the correct coloumn widths
				Classes->Gui->SaveRomListColoumnInfo();
				
				//Modify the Field List to have the new fields used
				for (int count = 0; count < Fields->size(); count ++) { (*Fields)[count].Pos = -1; }
				int listCount = SendDlgItemMessage((HWND)hDlg,IDC_USING,LB_GETCOUNT,0,0);
				for (int Item = 0; Item < listCount; Item ++ ){
					int Pos = SendMessage(GetDlgItem((HWND)hDlg,IDC_USING),LB_GETITEMDATA,Item,0);
					(*Fields)[Pos].Pos = Item;
				}
						
				//Reset the Rom Browser to see any changes
				ResetColoums = true;
				RefreshData  = true;
			}
			
			//Handle recursion
			bool Recursion = SendMessage(GetDlgItem((HWND)hDlg,IDC_RECURSION),BM_GETSTATE, 0,0) == BST_CHECKED?true:false;
			if (Recursion != (_Settings->LoadDword(RomBrowserRecursive) != 0)) {
				_Settings->SaveDword(RomBrowserRecursive,(DWORD)Recursion);
				RefreshData  = true;
			}

			bool HaveRomBrowser = SendMessage(GetDlgItem((HWND)hDlg,IDC_USE_ROMBROWSER),BM_GETSTATE, 0,0) == BST_CHECKED?true:false;
			if (HaveRomBrowser != (_Settings->LoadDword(RomBrowser) != 0)) {
				_Settings->SaveDword(RomBrowser,(DWORD)HaveRomBrowser);
				ResetMenu = true;
				if (HaveRomBrowser) {
					Classes->Gui->ShowRomList();
					ResetColoums = true;
					RefreshData  = true;
				} else {
					Classes->Gui->HideRomList();
					ResetColoums = false;
					RefreshData  = false;
				}
			}
			if (ResetMenu)    { Classes->Gui->GetNotifyClass()->RefreshMenu(); }
			if (ResetColoums) { 
				Classes->Gui->ResetRomBrowserColomuns();   
				Classes->Gui->SaveRomListColoumnInfo();
			}
			if (RefreshData)  {
				Classes->Gui->RefreshRomBrowser(); 
				Classes->Gui->HighLightLastRom();
			}
		
		}

		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return false;
	}
	return true;
}

BOOL CALLBACK GeneralOptionsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			
			if (_Settings->LoadDword(AutoSleep))       { SendMessage(GetDlgItem((HWND)hDlg,IDC_AUTOSLEEP),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(AutoFullScreen))  { SendMessage(GetDlgItem((HWND)hDlg,IDC_LOAD_FULLSCREEN),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(RememberCheats))  { SendMessage(GetDlgItem((HWND)hDlg,IDC_REMEMBER_CHEAT),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(DisableScrSaver)) { SendMessage(GetDlgItem((HWND)hDlg,IDC_SCREEN_SAVER),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(BasicMode))       { SendMessage(GetDlgItem((HWND)hDlg,IDC_BASIC_MODE),BM_SETCHECK, BST_CHECKED,0); }
			if (_Settings->LoadDword(DisplayFrameRate)){ SendMessage(GetDlgItem((HWND)hDlg,IDC_DISPLAY_FRAMERATE),BM_SETCHECK, BST_CHECKED,0); }

			SetDlgItemText((HWND)hDlg,IDC_AUTOSLEEP,         GS(OPTION_AUTO_SLEEP));
			SetDlgItemText((HWND)hDlg,IDC_LOAD_FULLSCREEN,   GS(OPTION_AUTO_FULLSCREEN));
			SetDlgItemText((HWND)hDlg,IDC_BASIC_MODE,        GS(OPTION_BASIC_MODE));
			SetDlgItemText((HWND)hDlg,IDC_REMEMBER_CHEAT,    GS(OPTION_REMEMBER_CHEAT));
			SetDlgItemText((HWND)hDlg,IDC_SCREEN_SAVER,      GS(OPTION_DISABLE_SS));
			SetDlgItemText((HWND)hDlg,IDC_DISPLAY_FRAMERATE, GS(OPTION_DISPLAY_FR));

			DWORD FRDisplayType = _Settings->LoadDword(FrameDisplayType);
			AddDropDownItem(hDlg,IDC_FRAME_DISPLAY_TYPE,GS(STR_FR_VIS),FR_VIs,&FRDisplayType);
			AddDropDownItem(hDlg,IDC_FRAME_DISPLAY_TYPE,GS(STR_FR_DLS),FR_DLs,&FRDisplayType);
			AddDropDownItem(hDlg,IDC_FRAME_DISPLAY_TYPE,GS(STR_FR_PERCENT),FR_PERCENT,&FRDisplayType);

			ShowWindow(GetDlgItem((HWND)hDlg,IDC_FRAME_DISPLAY_TYPE),SendMessage(GetDlgItem(hDlg,IDC_DISPLAY_FRAMERATE),BM_GETSTATE, 0,0) == BST_CHECKED ? SW_SHOW : SW_HIDE);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FRAME_DISPLAY_TYPE:
			if (HIWORD(wParam) == LBN_SELCHANGE) { 
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_AUTOSLEEP:
		case IDC_LOAD_FULLSCREEN:
		case IDC_BASIC_MODE:
		case IDC_REMEMBER_CHEAT:
		case IDC_SCREEN_SAVER:
			SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			break;
		case IDC_DISPLAY_FRAMERATE:
			SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			ShowWindow(GetDlgItem((HWND)hDlg,IDC_FRAME_DISPLAY_TYPE),(SendMessage(GetDlgItem(hDlg,IDC_DISPLAY_FRAMERATE),BM_GETSTATE, 0,0)  & BST_CHECKED) == BST_CHECKED ? SW_SHOW : SW_HIDE);
			break;
		}
		break;
	case WM_NOTIFY:
		//Save each selected plugin
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

			bool WasBasicMode = _Settings->LoadDword(BasicMode) != 0;

			//Save if using selected
			_Settings->SaveDword(AutoSleep,       SendMessage(GetDlgItem(hDlg,IDC_AUTOSLEEP),        BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(AutoFullScreen,  SendMessage(GetDlgItem(hDlg,IDC_LOAD_FULLSCREEN),  BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(BasicMode,       SendMessage(GetDlgItem(hDlg,IDC_BASIC_MODE),       BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(RememberCheats,  SendMessage(GetDlgItem(hDlg,IDC_REMEMBER_CHEAT),   BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(DisableScrSaver, SendMessage(GetDlgItem(hDlg,IDC_SCREEN_SAVER),     BM_GETSTATE, 0,0) == BST_CHECKED);
			_Settings->SaveDword(DisplayFrameRate,SendMessage(GetDlgItem(hDlg,IDC_DISPLAY_FRAMERATE),BM_GETSTATE, 0,0) == BST_CHECKED);
		
			int indx = SendDlgItemMessage(hDlg,IDC_FRAME_DISPLAY_TYPE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(FrameDisplayType,SendDlgItemMessage(hDlg,IDC_FRAME_DISPLAY_TYPE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			if (WasBasicMode != (_Settings->LoadDword(BasicMode) != 0)) {
				if (WasBasicMode && !((LPPSHNOTIFY)lParam)->lParam) {

					//add Advanced tabs
					for (int count = 0; count < (sizeof(AdvancedSettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
						PROPSHEETPAGE page;

						AdvancedSettingsTabs[count].Title = _Lang->GetString(AdvancedSettingsTabs[count].LanguageID);

						page.dwSize      = sizeof(PROPSHEETPAGE);
						page.dwFlags     = PSP_USETITLE;
						page.hInstance   = GetModuleHandle(NULL);
						page.pszTemplate = MAKEINTRESOURCE(AdvancedSettingsTabs[count].TemplateID);
						page.pfnDlgProc  = AdvancedSettingsTabs[count].pfnDlgProc;
						page.pszTitle    = AdvancedSettingsTabs[count].Title.c_str();
						page.lParam      = (DWORD)Classes;
						page.pfnCallback = NULL;
			
						PropSheet_AddPage(GetParent(hDlg),CreatePropertySheetPage(&page));
					}
				}
				if (!WasBasicMode && !((LPPSHNOTIFY)lParam)->lParam) {
					int MinPages = (sizeof(BasicSettingsTabs) / sizeof(SETTINGS_TAB));
					if (_Settings->LoadString(ROM_NAME).length() > 0) {						
						MinPages += (sizeof(RomSettingsTabs) / sizeof(SETTINGS_TAB));
					}
					for (int count = MaxConfigPages; count >= MinPages; count --) {
						PropSheet_RemovePage(GetParent(hDlg),count,0);
					}					
				}
				Classes->Gui->RefreshMenu();
			}
		}

		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return false;
	}
	return true;
}

BOOL CALLBACK RomSettingsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			
			SetDlgItemText(hDlg,IDC_CPU_TYPE_TEXT,GS(ROM_CPU_STYLE));
			SetDlgItemText(hDlg,IDC_FUNCFIND_TEXT,GS(ROM_FUNC_FIND));
			SetDlgItemText(hDlg,IDC_MEMORY_SIZE_TEXT,GS(ROM_MEM_SIZE));
			SetDlgItemText(hDlg,IDC_BLOCK_LINKING_TEXT,GS(ROM_ABL));
			SetDlgItemText(hDlg,IDC_SAVE_TYPE_TEXT,GS(ROM_SAVE_TYPE));
//			SetDlgItemText(hDlg,IDC_COUNTFACT_TEXT,GS(ROM_COUNTER_FACTOR));
			
			SetDlgItemText(hDlg,IDC_GOOD_NAME,_Settings->LoadString(ROM_GoodName).c_str());
			
			DWORD CPU_Type = _Settings->LoadDword(ROM_CPUType);
			AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(ROM_DEFAULT),    CPU_Default,    &CPU_Type);
			AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_INTERPTER), CPU_Interpreter,&CPU_Type);
			AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_RECOMPILER),CPU_Recompiler, &CPU_Type);
			if (_Settings->LoadDword(Debugger)) { 
				AddDropDownItem(hDlg,IDC_CPU_TYPE,GS(CORE_SYNC),  CPU_SyncCores,  &CPU_Type); 
			}

			DWORD FunctionLookup = _Settings->LoadDword(ROM_FunctionLookup);
			AddDropDownItem(hDlg,IDC_FUNCFIND,GS(ROM_DEFAULT),FuncFind_Default,&FunctionLookup);
			AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_PLOOKUP),FuncFind_PhysicalLookup,&FunctionLookup);
			AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_VLOOKUP),FuncFind_VirtualLookup,&FunctionLookup);
			//AddDropDownItem(hDlg,IDC_FUNCFIND,GS(FLM_CHANGEMEM),FuncFind_ChangeMemory,&FunctionLookup);

			DWORD RomRamSize = _Settings->LoadDword(ROM_RamSize);
			AddDropDownItem(hDlg,IDC_RDRAM_SIZE,GS(ROM_DEFAULT),-1,&RomRamSize);
			AddDropDownItem(hDlg,IDC_RDRAM_SIZE,GS(RDRAM_4MB),4,&RomRamSize);
			AddDropDownItem(hDlg,IDC_RDRAM_SIZE,GS(RDRAM_8MB),8,&RomRamSize);

			DWORD RomUseLinking = _Settings->LoadDword(ROM_BlockLinking);
			AddDropDownItem(hDlg,IDC_BLOCK_LINKING,GS(ROM_DEFAULT),-1,&RomUseLinking);
			AddDropDownItem(hDlg,IDC_BLOCK_LINKING,GS(ABL_ON),1,&RomUseLinking);
			AddDropDownItem(hDlg,IDC_BLOCK_LINKING,GS(ABL_OFF),0,&RomUseLinking);

			DWORD RomSaveUsing = _Settings->LoadDword(ROM_SaveChip);
			AddDropDownItem(hDlg,IDC_SAVE_TYPE,GS(SAVE_FIRST_USED),SaveChip_Auto,      &RomSaveUsing);
			AddDropDownItem(hDlg,IDC_SAVE_TYPE,GS(SAVE_4K_EEPROM), SaveChip_Eeprom_4K, &RomSaveUsing);
			AddDropDownItem(hDlg,IDC_SAVE_TYPE,GS(SAVE_16K_EEPROM),SaveChip_Eeprom_16K,&RomSaveUsing);
			AddDropDownItem(hDlg,IDC_SAVE_TYPE,GS(SAVE_SRAM),      SaveChip_Sram,      &RomSaveUsing);
			AddDropDownItem(hDlg,IDC_SAVE_TYPE,GS(SAVE_FLASHRAM),  SaveChip_FlashRam,  &RomSaveUsing);

			DWORD RomCF = _Settings->LoadDword(ROM_CounterFactor);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(ROM_DEFAULT),-1,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_1),1,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_2),2,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_3),3,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_4),4,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_5),5,&RomCF);
			AddDropDownItem(hDlg,IDC_COUNTFACT,GS(NUMBER_6),6,&RomCF);

			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SyncAudio)    != 0, IDC_SYNC_AUDIO,           GS(ROM_SYNC_AUDIO));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_UseTlb)       != 0, IDC_USE_TLB,              GS(ROM_USE_TLB));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_RegCache)     != 0, IDC_ROM_REGCACHE,         GS(ROM_REG_CACHE));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_DelaySI)      != 0, IDC_DELAY_SI,             GS(ROM_DELAY_SI));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_AudioSignal)  != 0, IDC_AUDIO_SIGNAL,         GS(ROM_AUDIO_SIGNAL));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SPHack)       != 0, IDC_ROM_SPHACK,           GS(ROM_SP_HACK));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_FixedAudio)   != 0, IDC_ROM_FIXEDAUDIO,       GS(ROM_FIXED_AUDIO));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_CustomSMM)    != 0, IDC_CUSTOM_SMM,           GS(ROM_CUSTOM_SMM));			
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SMM_Cache)    != 0, IDC_SMM_CACHE,            GS(ADVANCE_SMM_CACHE));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SMM_PIDMA)    != 0, IDC_SMM_DMA,              GS(ADVANCE_SMM_PIDMA));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SMM_TLB)      != 0, IDC_SMM_TLB,              GS(ADVANCE_SMM_TLB));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SMM_Protect)  != 0, IDC_SMM_PROTECT,          GS(ADVANCE_SMM_PROTECT));
			SetFlagControl(hDlg,_Settings->LoadDword(ROM_SMM_ValidFunc)!= 0, IDC_SMM_VALIDATE,         GS(ADVANCE_SMM_VALIDATE));			
			
			bool UseCustomSMM = _Settings->LoadDword(ROM_CustomSMM) != 0;			
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_FRAME),UseCustomSMM ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_CACHE),UseCustomSMM ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_DMA),UseCustomSMM ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_TLB),UseCustomSMM ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_PROTECT),UseCustomSMM ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SMM_VALIDATE),UseCustomSMM ? TRUE : FALSE);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CPU_TYPE:
		case IDC_RDRAM_SIZE:
		case IDC_SAVE_TYPE:
		case IDC_COUNTFACT:
			if (HIWORD(wParam) == LBN_SELCHANGE) { 
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_CUSTOM_SMM:
			{
				int MSGState = SendMessage(GetDlgItem(hDlg,IDC_CUSTOM_SMM),BM_GETSTATE, 0,0);
				bool UseCustomSMM = (SendMessage(GetDlgItem(hDlg,IDC_CUSTOM_SMM),BM_GETSTATE, 0,0) & BST_CHECKED) == BST_CHECKED;
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_FRAME),UseCustomSMM ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_CACHE),UseCustomSMM ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_DMA),UseCustomSMM ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_TLB),UseCustomSMM ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_PROTECT),UseCustomSMM ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_SMM_VALIDATE),UseCustomSMM ? TRUE : FALSE);
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_CACHE:
		//case IDC_LINKED:
		//case IDC_PI_DMA:
		case IDC_TLB:
		//case IDC_PROTECT:
		//case IDC_VALIDATE:
		//case IDC_VALIDATE_FUNCTION:
		case IDC_USE_TLB:
		case IDC_BLOCK_LINKING:
//		case IDC_DELAY_DLIST:
		case IDC_DELAY_SI:
			SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			break;
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");

			int indx = SendDlgItemMessage(hDlg,IDC_CPU_TYPE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_CPUType,SendDlgItemMessage(hDlg,IDC_CPU_TYPE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));
			
			indx = SendDlgItemMessage(hDlg,IDC_RDRAM_SIZE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_RamSize,SendDlgItemMessage(hDlg,IDC_RDRAM_SIZE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_SAVE_TYPE,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_SaveChip,SendDlgItemMessage(hDlg,IDC_SAVE_TYPE,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_FUNCFIND,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_FunctionLookup,SendDlgItemMessage(hDlg,IDC_FUNCFIND,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_COUNTFACT,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_CounterFactor,SendDlgItemMessage(hDlg,IDC_COUNTFACT,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			indx = SendDlgItemMessage(hDlg,IDC_BLOCK_LINKING,CB_GETCURSEL,0,0); 
			_Settings->SaveDword(ROM_BlockLinking,SendDlgItemMessage(hDlg,IDC_BLOCK_LINKING,CB_GETITEMDATA,indx >= 0 ? indx : 0,0));

			_Settings->SaveDword(ROM_SyncAudio,    (SendMessage(GetDlgItem(hDlg,IDC_SYNC_AUDIO),      BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_UseTlb,       (SendMessage(GetDlgItem(hDlg,IDC_USE_TLB),         BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_RegCache,     (SendMessage(GetDlgItem(hDlg,IDC_ROM_REGCACHE),    BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_DelaySI,      (SendMessage(GetDlgItem(hDlg,IDC_DELAY_SI),        BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_AudioSignal,  (SendMessage(GetDlgItem(hDlg,IDC_AUDIO_SIGNAL),    BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SPHack,       (SendMessage(GetDlgItem(hDlg,IDC_ROM_SPHACK),      BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_FixedAudio,   (SendMessage(GetDlgItem(hDlg,IDC_ROM_FIXEDAUDIO),  BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_CustomSMM,    (SendMessage(GetDlgItem(hDlg,IDC_CUSTOM_SMM),      BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SMM_Cache,    (SendMessage(GetDlgItem(hDlg,IDC_SMM_CACHE),       BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SMM_PIDMA,    (SendMessage(GetDlgItem(hDlg,IDC_SMM_DMA),         BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SMM_TLB,      (SendMessage(GetDlgItem(hDlg,IDC_SMM_TLB),         BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SMM_Protect,  (SendMessage(GetDlgItem(hDlg,IDC_SMM_PROTECT),     BM_GETSTATE, 0,0) == BST_CHECKED));
			_Settings->SaveDword(ROM_SMM_ValidFunc,(SendMessage(GetDlgItem(hDlg,IDC_SMM_VALIDATE),    BM_GETSTATE, 0,0) == BST_CHECKED));
		}

		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return false;
	}
	return true;
}

BOOL CALLBACK RomStatusProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;
			SetProp((HWND)hDlg,"Classes",(SETTING_CLASSES *)ps->lParam);
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)ps->lParam;
			
			SetDlgItemText(hDlg,IDC_STATUS_TEXT,GS(RB_STATUS));
			SetDlgItemText(hDlg,IDC_NOTES_CORE_TEXT,GS(RB_NOTES_CORE));
			SetDlgItemText(hDlg,IDC_NOTES_PLUGIN_TEXT,GS(RB_NOTES_PLUGIN));

			CIniFile RomIniFile  (_Settings->LoadString(RomDatabaseFile).c_str());
			strlist Keys;
			RomIniFile.GetKeyList("Rom Status",Keys);
			stdstr Status = _Settings->LoadString(ROM_Status);
			for (strlist::iterator item = Keys.begin(); item != Keys.end(); item++ ) {
				if (strstr(item->c_str(),".Sel") != NULL) { continue; }
				if (strstr(item->c_str(),".Auto") != NULL) { continue; }
				DWORD CurrentStatus = _stricmp(Status.c_str(),item->c_str()) == 0;
				AddDropDownItem(hDlg,IDC_STATUS_TYPE,item->c_str(),true,&CurrentStatus);
			}

			SetDlgItemText(hDlg,IDC_NOTES_CORE,_Settings->LoadString(ROM_CoreNotes).c_str());
			SetDlgItemText(hDlg,IDC_NOTES_PLUGIN,_Settings->LoadString(ROM_PluginNotes).c_str());
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STATUS_TYPE:
			if (HIWORD(wParam) == LBN_SELCHANGE) { 
				SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
			}
			break;
		case IDC_NOTES_CORE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char text[2000];
				
				GetDlgItemText(hDlg,IDC_NOTES_CORE,text,sizeof(text));
				if (stricmp(text,_Settings->LoadString(ROM_CoreNotes).c_str()) != 0) 
				{ 
					SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
				}
			}
		case IDC_NOTES_PLUGIN:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char text[2000];
				
				GetDlgItemText(hDlg,IDC_NOTES_PLUGIN,text,sizeof(text));
				if (stricmp(text,_Settings->LoadString(ROM_PluginNotes).c_str()) != 0) 
				{ 
					SendMessage(GetParent((HWND)hDlg),PSM_CHANGED ,(WPARAM)hDlg,0);
				}
			}
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			SETTING_CLASSES * Classes = (SETTING_CLASSES *)GetProp((HWND)hDlg,"Classes");
			
			bool changed = false;
			char text[2000];
			GetDlgItemText(hDlg,IDC_STATUS_TYPE,text,sizeof(text));
			if (stricmp(text,_Settings->LoadString(ROM_Status).c_str()) != 0) { changed = true; }
			_Settings->SaveString(ROM_Status,text);

			GetDlgItemText(hDlg,IDC_NOTES_CORE,text,sizeof(text));
			if (stricmp(text,_Settings->LoadString(ROM_CoreNotes).c_str()) != 0) { changed = true; }
			if (strlen(text) > 0) {
				_Settings->SaveString(ROM_CoreNotes,text);
			} else {
				_Settings->SaveString(ROM_CoreNotes,"");
			}
			GetDlgItemText(hDlg,IDC_NOTES_PLUGIN,text,sizeof(text));
			if (stricmp(text,_Settings->LoadString(ROM_PluginNotes).c_str()) != 0) { changed = true; }
			if (strlen(text) > 0) {
				_Settings->SaveString(ROM_PluginNotes,text);
			} else {
				_Settings->SaveString(ROM_PluginNotes,"");
			}
			if (changed) {
				Classes->Gui->RefreshRomBrowser(); 
				Classes->Gui->HighLightLastRom();
			}
		}

		//Free allocated memory
		if (((NMHDR FAR *) lParam)->code == PSN_RESET || 
			(((NMHDR FAR *) lParam)->code == PSN_APPLY && ((LPPSHNOTIFY)lParam)->lParam)) 
		{ 
			RemoveProp((HWND)hDlg,"Classes");
		}                        
		break;
	default:
		return false;
	}
	return true;
}

BOOL TestExtensionRegistered ( char * Extension ) {
	char ShortAppName[] = { "PJ64" }; 
	HKEY hKeyResults = 0;
	char Association[100];
	long lResult;
	DWORD Type, Bytes;

	lResult = RegOpenKey( HKEY_CLASSES_ROOT,Extension,&hKeyResults);	
	if (lResult != ERROR_SUCCESS) { return FALSE; }
	
	Bytes = sizeof(Association);
	lResult = RegQueryValueEx(hKeyResults,"",0,&Type,(LPBYTE)(&Association),&Bytes);
	RegCloseKey(hKeyResults);
	if (lResult != ERROR_SUCCESS) { return FALSE;  }

	if (strcmp(Association,ShortAppName) != 0) { return FALSE; }
	return TRUE;
}

void RegisterExtension ( char * Extension, BOOL RegisterWithPj64 ) {
	char ShortAppName[] = { "PJ64" }; 
	char sKeyValue[] = { "Project 64" };
  	char app_path[_MAX_PATH];
	
	char String[200];
	DWORD Disposition = 0;
	HKEY hKeyResults = 0;
	long lResult;

	//Find Application name
	GetModuleFileName(NULL,app_path,sizeof(app_path));
 
	//creates a Root entry for sKeyName
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, ShortAppName,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)sKeyValue,sizeof(sKeyValue));
	RegCloseKey(hKeyResults);

	// Set the command line for "MyApp".
	sprintf(String,"%s\\DefaultIcon",ShortAppName);
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, String,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	sprintf(String,"%s",app_path);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)String,strlen(String));
	RegCloseKey(hKeyResults);
	
	//set the icon for the file extension
	sprintf(String,"%s\\shell\\open\\command",ShortAppName);
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, String,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	sprintf(String,"%s %%1",app_path);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)String,strlen(String));
	RegCloseKey(hKeyResults);

	// creates a Root entry for passed associated with sKeyName
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, Extension,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (RegisterWithPj64) {
		RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)ShortAppName,sizeof(ShortAppName));
	} else {
		RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)"",1);
	}
 
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);	
}

BOOL CALLBACK ShellIntegrationProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		{
			//record class for future usage	
			LPPROPSHEETPAGE ps = (LPPROPSHEETPAGE)lParam;

			SetDlgItemText(hDlg,IDC_SHELL_INT_TEXT,GS(SHELL_TEXT));	
			if (TestExtensionRegistered(".v64")) { SendMessage(GetDlgItem(hDlg,IDC_V64),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".z64")) { SendMessage(GetDlgItem(hDlg,IDC_Z64),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".n64")) { SendMessage(GetDlgItem(hDlg,IDC_N64),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".rom")) { SendMessage(GetDlgItem(hDlg,IDC_ROM),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".jap")) { SendMessage(GetDlgItem(hDlg,IDC_JAP),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".pal")) { SendMessage(GetDlgItem(hDlg,IDC_PAL),BM_SETCHECK, BST_CHECKED,0); }
			if (TestExtensionRegistered(".usa")) { SendMessage(GetDlgItem(hDlg,IDC_USA),BM_SETCHECK, BST_CHECKED,0); }
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			RegisterExtension(".v64",SendMessage(GetDlgItem(hDlg,IDC_V64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".z64",SendMessage(GetDlgItem(hDlg,IDC_Z64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".n64",SendMessage(GetDlgItem(hDlg,IDC_N64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".rom",SendMessage(GetDlgItem(hDlg,IDC_ROM),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".jap",SendMessage(GetDlgItem(hDlg,IDC_JAP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".pal",SendMessage(GetDlgItem(hDlg,IDC_PAL),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".usa",SendMessage(GetDlgItem(hDlg,IDC_USA),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

#endif