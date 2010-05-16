#include "..\N64 System.h"

#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "Settings/SettingType/SettingsType-Cheats.h"

enum { WM_EDITCHEAT           = WM_USER + 0x120 };
enum { UM_CHANGECODEEXTENSION = WM_USER + 0x121 };

CCheats::CCheats (CN64Rom * const Rom, CNotification * const Notify ) :
	_Notify(Notify),
	_Rom(Rom),
	m_rcList(new RECT),
	m_rcAdd(new RECT),
	m_EditCheat(-1),
	m_DeleteingEntries(false),
	m_CheatSelectionChanged(false)
{
	m_Window       = NULL;
	m_hSelectCheat = NULL;
	m_AddCheat     = NULL;
	m_hCheatTree   = NULL;
}

CCheats::~CCheats(void) {
	delete m_rcList;
	delete m_rcAdd;
}

bool CCheats::LoadCode (int CheatNo, LPCSTR CheatString)
{
	if (!IsValid16BitCode(CheatString))
	{
		return false;
	}

	const char * ReadPos = CheatString;

	CODES Code;
	while (ReadPos)
	{
		GAMESHARK_CODE CodeEntry;

		CodeEntry.Command = AsciiToHex(ReadPos);
		ReadPos = strchr(ReadPos,' ');
		if (ReadPos == NULL) { break; }
		ReadPos +=1;

		if (strncmp(ReadPos,"????",4) == 0) {
			if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
			stdstr CheatExt = _Settings->LoadStringIndex(Cheat_Extension,CheatNo);
			if (CheatExt.empty()) { return false; }
			CodeEntry.Value = CheatExt[0] == '$'?(WORD)AsciiToHex(&CheatExt.c_str()[1]):(WORD)atol(CheatExt.c_str());
		} else if (strncmp(ReadPos,"??",2) == 0) {
			if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
			stdstr CheatExt = _Settings->LoadStringIndex(Cheat_Extension,CheatNo);
			if (CheatExt.empty()) { return false; }
			CodeEntry.Value = (BYTE)(AsciiToHex(ReadPos));
			CodeEntry.Value |= (CheatExt[0] == '$'?(BYTE)AsciiToHex(&CheatExt.c_str()[1]):(BYTE)atol(CheatExt.c_str())) << 16;
		} else if (strncmp(&ReadPos[2],"??",2) == 0) {	
			if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
			stdstr CheatExt = _Settings->LoadStringIndex(Cheat_Extension,CheatNo);
			if (CheatExt.empty()) { return false; }
			CodeEntry.Value = (WORD)(AsciiToHex(ReadPos) << 16);
			CodeEntry.Value |= CheatExt[0] == '$'?(BYTE)AsciiToHex(&CheatExt.c_str()[1]):(BYTE)atol(CheatExt.c_str());
		} else {
			CodeEntry.Value = (WORD)AsciiToHex(ReadPos);
		}
		Code.push_back(CodeEntry);
		
		ReadPos = strchr(ReadPos,',');
		if (ReadPos == NULL)
		{
			continue;
		}
		ReadPos++;
	}
	if (Code.size() == 0)
	{
		return false;
	}

	m_Codes.push_back(Code);
	return true;
}

void CCheats::LoadPermCheats (void) 
{
	if (_Settings->LoadBool(Debugger_DisableGameFixes))
	{
		return;
	}
	for (int CheatNo = 0; CheatNo < MaxCheats; CheatNo ++ ) 
	{
		stdstr LineEntry;
		if (!_Settings->LoadStringIndex(Rdb_GameCheatFix,CheatNo,LineEntry) || LineEntry.empty())
		{
			break;
		}
		LoadCode(-1, LineEntry.c_str());
	}
}

void CCheats::LoadCheats(bool DisableSelected) {
	m_CheatSelectionChanged = false;
	m_Codes.clear();
	LoadPermCheats();

	for (int CheatNo = 0; CheatNo < MaxCheats; CheatNo ++ ) 
	{
		stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,CheatNo);
		if (LineEntry.empty()) { break; }
		if (!_Settings->LoadBoolIndex(Cheat_Active,CheatNo))
		{
			continue;
		}
		if (DisableSelected)
		{
			_Settings->SaveBoolIndex(Cheat_Active,CheatNo,false);
			continue;
		}

		//Find the start and end of the name which is surrounded in ""
		int StartOfName = LineEntry.find("\"");
		if (StartOfName == -1) { continue; }
		int EndOfName = LineEntry.find("\"",StartOfName + 1);
		if (EndOfName == -1) { continue; }
		
		LoadCode(CheatNo, &LineEntry.c_str()[EndOfName + 2]);
	}
}

/********************************************************************************************
  ConvertXP64Address

  Purpose: Decode encoded XP64 address to physical address
  Parameters: 
  Returns: 
  Author: Witten

********************************************************************************************/
DWORD ConvertXP64Address (DWORD Address) {
	DWORD tmpAddress;

	tmpAddress = (Address ^ 0x68000000) & 0xFF000000;
	tmpAddress += ((Address + 0x002B0000) ^ 0x00810000) & 0x00FF0000;
	tmpAddress += ((Address + 0x00002B00) ^ 0x00008200) & 0x0000FF00;
	tmpAddress += ((Address + 0x0000002B) ^ 0x00000083) & 0x000000FF;
	return tmpAddress;
}

/********************************************************************************************
  ConvertXP64Value

  Purpose: Decode encoded XP64 value
  Parameters: 
  Returns: 
  Author: Witten

********************************************************************************************/
WORD ConvertXP64Value (WORD Value) {
	WORD  tmpValue;

	tmpValue = ((Value + 0x2B00) ^ 0x8400) & 0xFF00;
	tmpValue += ((Value + 0x002B) ^ 0x0085) & 0x00FF;
	return tmpValue;
}

void CCheats::ApplyCheats(CMipsMemory * _MMU)
{
	for (int CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat ++) 
	{
		const CODES & CodeEntry = m_Codes[CurrentCheat];
		for (int CurrentEntry = 0; CurrentEntry < CodeEntry.size();)
		{
			CurrentEntry += ApplyCheatEntry(_MMU, CodeEntry,CurrentEntry,TRUE);
		}
	}
}

void CCheats::ApplyGSButton (CMipsMemory * _MMU) 
{
	DWORD Address;
	for (int CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat ++) 
	{
		const CODES & CodeEntry = m_Codes[CurrentCheat];
		for (int CurrentEntry = 0; CurrentEntry < CodeEntry.size(); CurrentEntry ++)
		{
			const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
			switch (Code.Command & 0xFF000000) {
			case 0x88000000:
				Address = 0x80000000 | (Code.Command & 0xFFFFFF);
				_MMU->Store64(Address,Code.Value,_8Bit);
				break;
			case 0x89000000:
				Address = 0x80000000 | (Code.Command & 0xFFFFFF);
				_MMU->Store64(Address,Code.Value,_16Bit);
				break;
			// Xplorer64
			case 0xA8000000:
				Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
				_MMU->Store64(Address,ConvertXP64Value(Code.Value),_8Bit);
				break;
			case 0xA9000000:
				Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
				_MMU->Store64(Address,ConvertXP64Value(Code.Value),_16Bit);
				break;
			}
		}
	}
}

bool CCheats::IsValid16BitCode (LPCSTR CheatString) const
{
	const char * ReadPos = CheatString;
	bool GSButtonCheat = false, FirstEntry = true;

	while (ReadPos)
	{
		GAMESHARK_CODE CodeEntry;

		CodeEntry.Command = AsciiToHex(ReadPos);
		ReadPos = strchr(ReadPos,' ');
		if (ReadPos == NULL) { break; }
		ReadPos +=1;

		//validate Code Entry
		switch (CodeEntry.Command & 0xFF000000) {
		case 0x50000000:
		case 0x80000000: 
		case 0xA0000000:
		case 0xD0000000:
		case 0xD2000000:
		case 0xC8000000:
		case 0xE8000000:
		case 0x10000000: // Xplorer64
			break;
		case 0x81000000:
		case 0xA1000000:
		case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
		case 0xD3000000:
			if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
			{
				return false;
			}
			break;
		case 0x88000000:
		case 0xA8000000:
			if (FirstEntry)     { GSButtonCheat = true; } 
			if (!GSButtonCheat) { return false; }
			break;
		case 0x89000000:
			if (FirstEntry)     { GSButtonCheat = true; } 
			if (!GSButtonCheat) { return false; }
			if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
			{
				return false;
			}
			break;
		case 0xA9000000:
			if (FirstEntry)     { GSButtonCheat = true; } 
			if (!GSButtonCheat) { return false; }
			if (((ConvertXP64Address(CodeEntry.Command) & 0xFFFFFF) & 1) == 1)
			{
				return false;
			}
			break;
		case 0x11000000: // Xplorer64
		case 0xE9000000:
		case 0xC9000000:
			if (((ConvertXP64Address(CodeEntry.Command) & 0xFFFFFF) & 1) == 1)
			{
				return false;
			}
			break;
		default:
			return false;
		}

		FirstEntry = false;

		ReadPos = strchr(ReadPos,',');
		if (ReadPos == NULL)
		{
			continue;
		}
		ReadPos++;
	}
	return true;
}

int CCheats::ApplyCheatEntry (CMipsMemory * _MMU, const CODES & CodeEntry, int CurrentEntry, BOOL Execute )
{
	if (CurrentEntry < 0 || CurrentEntry >= CodeEntry.size())
	{
		return 0;
	}
	const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
	DWORD Address, dwMemory;
	WORD  Memory;

	switch (Code.Command & 0xFF000000) {
	// Gameshark / AR
	case 0x50000000:													// Added by Witten (witten@pj64cheats.net)
		{
			if ((CurrentEntry + 1) >= CodeEntry.size())
			{
				return 1;
			}

			const GAMESHARK_CODE & NextCodeEntry = CodeEntry[CurrentEntry + 1];
			int numrepeats = (Code.Command & 0x0000FF00) >> 8;
			int offset = Code.Command & 0x000000FF;
			int incr = Code.Value;
			int count;

			switch (NextCodeEntry.Command & 0xFF000000) {
			case 0x10000000: // Xplorer64
			case 0x80000000:
				Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
				Memory = NextCodeEntry.Value;
				for (count=0; count<numrepeats; count++) {
					_MMU->Store64(Address,Memory,_8Bit);
					Address += offset;
					Memory += incr;
				}
				return 2;
			case 0x11000000: // Xplorer64
			case 0x81000000:
				Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
				Memory = NextCodeEntry.Value;
				for (count=0; count<numrepeats; count++) {
					_MMU->Store64(Address,Memory,_16Bit);
					Address += offset;
					Memory += incr;
				}
				return 2;
			default: return 1;
			}
		}
		break;
	case 0x80000000:
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_8Bit); }
		break;
	case 0x81000000:
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_16Bit); }
		break;
	case 0xA0000000:
		Address = 0xA0000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_8Bit);  }
		break;
	case 0xA1000000:
		Address = 0xA0000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_16Bit); }
		break;
	case 0xD0000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_8Bit,false);
		if (dwMemory != Code.Value) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_16Bit,false);
		if (dwMemory != Code.Value) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xD2000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_8Bit,false);
		if (dwMemory == Code.Value) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xD3000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_16Bit,false);
		if (dwMemory == Code.Value) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;

	// Xplorer64 (Author: Witten)
	case 0x30000000:
	case 0x82000000:
	case 0x84000000:
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_8Bit); }
		break;
	case 0x31000000:
	case 0x83000000:
	case 0x85000000:
		Address = 0x80000000 | (Code.Command & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_16Bit); }
		break;
	case 0xE8000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,(BYTE)ConvertXP64Value(Code.Value),_8Bit); }
		break;
	case 0xE9000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,ConvertXP64Value(Code.Value),_16Bit); }
		break;
	case 0xC8000000:
		Address = 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,Code.Value,_8Bit);  }
		break;
	case 0xC9000000:
		Address = 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		if (Execute) { _MMU->Store64(Address,ConvertXP64Value(Code.Value),_16Bit); }
		break;
	case 0xB8000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_8Bit,false);
		if (dwMemory != ConvertXP64Value(Code.Value)) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xB9000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_16Bit,false);
		if (dwMemory != ConvertXP64Value(Code.Value)) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xBA000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_8Bit,false);
		if (dwMemory == ConvertXP64Value(Code.Value)) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0xBB000000:
		Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
		_MMU->Load32(Address,dwMemory,_16Bit,false);
		if (dwMemory == ConvertXP64Value(Code.Value)) { Execute = FALSE; }
		return ApplyCheatEntry(_MMU,CodeEntry,CurrentEntry + 1,Execute) + 1;
	case 0: return MaxGSEntries; break;
	}
	return 1;
}


DWORD CCheats::AsciiToHex (const char * HexValue) {
	DWORD Count, Finish, Value = 0;

	Finish = strlen(HexValue);
	if (Finish > 8 ) { Finish = 8; }

	for (Count = 0; Count < Finish; Count++){
		Value = (Value << 4);
		switch( HexValue[Count] ) {
		case '0': break;
		case '1': Value += 1; break;
		case '2': Value += 2; break;
		case '3': Value += 3; break;
		case '4': Value += 4; break;
		case '5': Value += 5; break;
		case '6': Value += 6; break;
		case '7': Value += 7; break;
		case '8': Value += 8; break;
		case '9': Value += 9; break;
		case 'A': Value += 10; break;
		case 'a': Value += 10; break;
		case 'B': Value += 11; break;
		case 'b': Value += 11; break;
		case 'C': Value += 12; break;
		case 'c': Value += 12; break;
		case 'D': Value += 13; break;
		case 'd': Value += 13; break;
		case 'E': Value += 14; break;
		case 'e': Value += 14; break;
		case 'F': Value += 15; break;
		case 'f': Value += 15; break;
		default: 
			Value = (Value >> 4);
			Count = Finish;
		}
	}
	return Value;
}

void CCheats::AddCodeLayers (int CheatNumber, stdstr &CheatName, WND_HANDLE hParent, bool CheatActive) {
	TV_INSERTSTRUCT tv;
	
	//Work out text to add
	char Text[500], Item[500];
	if (CheatName.length() > (sizeof(Text) - 5)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	strcpy(Text,CheatName.c_str());
	if (strchr(Text,'\\') > 0) { *strchr(Text,'\\') = 0; }

	//See if text is already added
	tv.item.mask       = TVIF_TEXT;
	tv.item.pszText    = Item;
	tv.item.cchTextMax = sizeof(Item);
	tv.item.hItem      = TreeView_GetChild((HWND)m_hCheatTree,hParent);
	while (tv.item.hItem) {
		TreeView_GetItem((HWND)m_hCheatTree,&tv.item);
		if (strcmp(Text,Item) == 0) { 
			//If already exists then just use existing one
			int State = TV_GetCheckState(m_hCheatTree,(WND_HANDLE)tv.item.hItem);
			if ((CheatActive && State == TV_STATE_CLEAR) || (!CheatActive && State == TV_STATE_CHECKED)) { 
				TV_SetCheckState(m_hCheatTree,(WND_HANDLE)tv.item.hItem,TV_STATE_INDETERMINATE); 
			}
			int StartPos = strlen(Text) + 1;
			stdstr TempCheatName;
			if (StartPos < CheatName.length())
			{
				TempCheatName = CheatName.substr(StartPos);
			}
			AddCodeLayers(CheatNumber,TempCheatName, (WND_HANDLE)tv.item.hItem, CheatActive);
			return; 
		}
		tv.item.hItem = TreeView_GetNextSibling((HWND)m_hCheatTree,tv.item.hItem);
	}

	//Add to dialog
	tv.hInsertAfter = TVI_SORT;
	tv.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tv.item.pszText = Text;
	tv.item.lParam  = CheatNumber;
	tv.hParent      = (HTREEITEM)hParent;
	hParent = (WND_HANDLE)TreeView_InsertItem((HWND)m_hCheatTree,&tv);
	TV_SetCheckState(m_hCheatTree,hParent,CheatActive?TV_STATE_CHECKED:TV_STATE_CLEAR);

	if (strcmp(Text,CheatName.c_str()) == 0) { return; }
	AddCodeLayers(CheatNumber,(stdstr)(CheatName.substr(strlen(Text) + 1)), hParent, CheatActive);
}

stdstr CCheats::GetCheatName(int CheatNo, bool AddExtension) const 
{
	if (CheatNo > MaxCheats) { _Notify->BreakPoint(__FILE__,__LINE__); }
	stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,CheatNo);
	if (LineEntry.length() == 0) { return LineEntry; }
	
	//Find the start and end of the name which is surrounded in ""
	int StartOfName = LineEntry.find("\"");
	if (StartOfName == -1) { return stdstr(""); }
	int EndOfName = LineEntry.find("\"",StartOfName + 1);
	if (EndOfName == -1) { return stdstr(""); }

	stdstr Name = LineEntry.substr(StartOfName + 1, EndOfName - StartOfName - 1 );
	LPCSTR CodeString = &(LineEntry.c_str())[EndOfName + 2];
	if (!IsValid16BitCode(CodeString))
	{
		Name.Format("*** %s",Name.c_str());
		Name.replace("\\","\\*** ");
	}
	if (AddExtension && CheatUsesCodeExtensions(LineEntry)) {
		stdstr CheatValue(_Settings->LoadStringIndex(Cheat_Extension,CheatNo));
		Name.Format("%s (=>%s)",Name.c_str(),CheatValue.c_str());
	}

	return Name;
}

bool CCheats::CheatUsesCodeExtensions (const stdstr &LineEntry) {
	//Find the start and end of the name which is surronded in ""
	if (LineEntry.length() == 0){ return false; }
	int StartOfName = LineEntry.find("\"");
	if (StartOfName == -1)      { return false; }
	int EndOfName = LineEntry.find("\"",StartOfName + 1);
	if (EndOfName == -1)        { return false; }

	//Read through the gameshark entries till you find a ??
	const char *ReadPos = &(LineEntry.c_str())[EndOfName + 2];
	bool CodeExtension = false;
	
	for (int count = 0; count < MaxGSEntries && CodeExtension == false; count ++) {
		if (strchr(ReadPos,' ') == NULL) { break; }
		ReadPos = strchr(ReadPos,' ') + 1;
		if (ReadPos[0] == '?' && ReadPos[1]== '?') { CodeExtension = true; }
		if (ReadPos[2] == '?' && ReadPos[3]== '?') { CodeExtension = true; }
		if (strchr(ReadPos,',') == NULL) { continue; }
		ReadPos = strchr(ReadPos,',') + 1;
	}
	return CodeExtension;
}

void CCheats::RefreshCheatManager(void) 
{
	if (m_Window == NULL) { return; }

    int CurrentEdit = m_EditCheat;
	
	m_DeleteingEntries = true;
	TreeView_DeleteAllItems((HWND)m_hCheatTree);
	m_DeleteingEntries = false;
	for (int count = 0; count < MaxCheats; count ++ ) {
		stdstr Name = GetCheatName(count,true);
		if (Name.length() == 0) { break; }

		AddCodeLayers(count,Name,(WND_HANDLE)TVI_ROOT, _Settings->LoadBoolIndex(Cheat_Active,count) != 0);
	}
}

stdstr CCheats::GetDlgItemStr (WND_HANDLE hDlg, int nIDDlgItem)
{
	HWND hDlgItem = GetDlgItem((HWND)hDlg,nIDDlgItem);
	int length = SendMessage(hDlgItem, WM_GETTEXTLENGTH, 0, 0);
	if (length == 0)
	{
		return "";
	}

	stdstr Result;
	Result.resize(length + 1);

    GetWindowText(hDlgItem,(char *)Result.c_str(),Result.length());

	return Result;
}

void CCheats::SelectCheats(WND_HANDLE hParent, bool BlockExecution) {
	if (m_Window != NULL) {
		SetForegroundWindow((HWND)m_Window);
		return;
	}
	if (hParent) 
	{
		if (BlockExecution)
		{
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_Select), 
				(HWND)hParent, (DLGPROC)ManageCheatsProc,(LPARAM)this);
		} else {
			CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_Select), 
				(HWND)hParent, (DLGPROC)ManageCheatsProc,(LPARAM)this);
		}
	}
}


bool CCheats::CheatChanged (WND_HANDLE hDlg)
{
	bool Changed = false;
	if (m_EditName    != GetDlgItemStr(hDlg,IDC_CODE_NAME) ||
	    m_EditCode    != GetDlgItemStr(hDlg,IDC_CHEAT_CODES) ||
  	    m_EditOptions != GetDlgItemStr(hDlg,IDC_CHEAT_OPTIONS) ||
	    m_EditNotes   != GetDlgItemStr(hDlg,IDC_NOTES))
	{
		Changed = true;
	}
	if (!Changed)
	{
		return false;
	}
	int Result = MessageBox((HWND)hDlg,GS(CHEAT_CHANGED_MSG),GS(CHEAT_CHANGED_TITLE),MB_YESNOCANCEL);
	if (Result == IDCANCEL)
	{
		return true;
	}
	if (Result == IDYES)
	{
		SendMessage((HWND)hDlg,WM_COMMAND, MAKELPARAM(IDC_ADD, 0), (LPARAM)GetDlgItem((HWND)hDlg,IDC_ADD));
	}
	return false;
}

void CCheats::RecordCheatValues ( WND_HANDLE hDlg )
{
	m_EditName    = GetDlgItemStr(hDlg,IDC_CODE_NAME);
	m_EditCode    = GetDlgItemStr(hDlg,IDC_CHEAT_CODES);
	m_EditOptions = GetDlgItemStr(hDlg,IDC_CHEAT_OPTIONS);
	m_EditNotes   = GetDlgItemStr(hDlg,IDC_NOTES);
}

int CALLBACK CCheats::CheatAddProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCheats   * _this = (CCheats *)lParam;
			SetProp((HWND)hDlg,"Class",_this);
			
			SetWindowText((HWND)hDlg,GS(CHEAT_ADDCHEAT_FRAME));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_NAME),GS(CHEAT_ADDCHEAT_NAME));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_CODE),GS(CHEAT_ADDCHEAT_CODE));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_LABEL_OPTIONS),GS(CHEAT_ADDCHEAT_OPT));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_CODE_DES),GS(CHEAT_ADDCHEAT_CODEDES));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_LABEL_OPTIONS_FORMAT),GS(CHEAT_ADDCHEAT_OPTDES));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_CHEATNOTES),GS(CHEAT_ADDCHEAT_NOTES));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_NEWCHEAT),GS(CHEAT_ADDCHEAT_NEW));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_ADD),GS(CHEAT_ADDCHEAT_ADD));
			SetProp((HWND)hDlg,"validcodes",false);
			_this->RecordCheatValues(hDlg);
		}
		break;
	case WM_COMMAND:
		{			
			switch (LOWORD(wParam)) {
			case IDC_CODE_NAME:
				if (HIWORD(wParam) == EN_CHANGE) {
					bool validcodes,validoptions, nooptions;
					int  CodeFormat;
					ReadCodeString(hDlg,validcodes,validoptions,nooptions,CodeFormat);
					if (!nooptions) { 
						ReadOptionsString(hDlg,validcodes,validoptions,nooptions,CodeFormat);
					}

					if (validcodes && (validoptions || nooptions) && SendDlgItemMessage((HWND)hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), true);
					}
					else {
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), false);
					}
				}
				break;
			case IDC_CHEAT_CODES:
				if (HIWORD(wParam) == EN_CHANGE) {
					bool validcodes,validoptions, nooptions;
					int  CodeFormat;
					ReadCodeString(hDlg,validcodes,validoptions,nooptions,CodeFormat);

					//GetDlgItemText(hDlg, IDC_CHEAT_CODES, str, 1024);
					if ((CodeFormat > 0) && !IsWindowEnabled(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS)) ) {
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS), true);
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS_FORMAT), true);
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_CHEAT_OPTIONS), true);
					}
					if ((CodeFormat <= 0) && IsWindowEnabled(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS)) ) {
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS), false);
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_LABEL_OPTIONS_FORMAT), false);
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_CHEAT_OPTIONS), false);
					}

					if (!nooptions) { 
						ReadOptionsString(hDlg,validcodes,validoptions,nooptions,CodeFormat);
					}

					if (validcodes && (validoptions || nooptions) && SendDlgItemMessage((HWND)hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), true);
					}
					else {
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), false);
					}
				}
				break;

			case IDC_CHEAT_OPTIONS:
				if (HIWORD(wParam) == EN_CHANGE) {
					bool validcodes,validoptions, nooptions;
					int  CodeFormat;
					ReadOptionsString(hDlg,validcodes,validoptions,nooptions,CodeFormat);
					
					if (validcodes && (validoptions || nooptions) && SendDlgItemMessage((HWND)hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), true);
					}
					else {
						EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), false);
					}
				}
				break;
			case IDC_ADD:
				{
					CCheats       * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
					CNotification * _Notify   = _this->_Notify;
					
					stdstr NewCheatName = GetDlgItemStr(hDlg,IDC_CODE_NAME);
					for (int count = 0; count < MaxCheats; count ++) {
						if (_this->m_EditCheat == count)
						{
							continue;
						}
						stdstr CheatName(_this->GetCheatName(count,false));
						if (CheatName.length() == 0) 
						{
							if (_this->m_EditCheat < 0)
							{
								_this->m_EditCheat = count;
							}
							break;
						}
						if (stricmp(CheatName.c_str(),NewCheatName.c_str()) == 0) {
							_Notify->DisplayError(GS(MSG_CHEAT_NAME_IN_USE));
							SetFocus(GetDlgItem((HWND)hDlg,IDC_CODE_NAME));
							return true;
						}
					}
					if (_this->m_EditCheat < 0 && count == MaxCheats) {
						_Notify->DisplayError(GS(MSG_MAX_CHEATS));
						return true;
					}
					
					//Update the entries
					bool validcodes,validoptions, nooptions;
					int  CodeFormat;
					stdstr_f Cheat("\"%s\"%s",NewCheatName.c_str(),ReadCodeString(hDlg,validcodes,validoptions,nooptions,CodeFormat).c_str());
					stdstr Options = ReadOptionsString(hDlg,validcodes,validoptions,nooptions,CodeFormat);
						
					_Settings->SaveStringIndex(Cheat_Entry, _this->m_EditCheat,Cheat.c_str());
					_Settings->SaveStringIndex(Cheat_Notes, _this->m_EditCheat,GetDlgItemStr(hDlg,IDC_NOTES));
					_Settings->SaveStringIndex(Cheat_Options, _this->m_EditCheat,Options);
					_this->RecordCheatValues(hDlg);
					CSettingTypeCheats::FlushChanges();
					_this->RefreshCheatManager();
				}
				break;
			case IDC_NEWCHEAT:
				{
					CCheats       * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
					
					if (_this->CheatChanged(hDlg))
					{
						break;
					}
					_this->m_EditCheat = -1;
					SetDlgItemText((HWND)hDlg,IDC_CODE_NAME,"");
					SetDlgItemText((HWND)hDlg,IDC_CHEAT_CODES,"");
					SetDlgItemText((HWND)hDlg,IDC_CHEAT_OPTIONS,"");
					SetDlgItemText((HWND)hDlg,IDC_NOTES,"");
					EnableWindow(GetDlgItem((HWND)hDlg, IDC_ADD), false);
					EnableWindow(GetDlgItem((HWND)hDlg, IDC_CHEAT_OPTIONS), false);
					SetDlgItemText((HWND)hDlg,IDC_ADD,GS(CHEAT_ADDNEW));
					_this->RecordCheatValues(hDlg);
				}
				break;
			}
		}
		break;
	case WM_EDITCHEAT:
		{
			CCheats       * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
			CNotification * _Notify   = _this->_Notify;
			_this->m_EditCheat = wParam;
			if (_this->m_EditCheat < 0) 
			{
				break;
			}

			if (_this->CheatChanged(hDlg))
			{
				break;
			}

			stdstr CheatEntryStr = _Settings->LoadStringIndex(Cheat_Entry,_this->m_EditCheat);
			LPCSTR String = CheatEntryStr.c_str();
			
			//Set Cheat Name
			int len = strrchr(String,'"') - strchr(String,'"') - 1;
			stdstr CheatName(strchr(String,'"') + 1);
			CheatName.resize(len);
			SetDlgItemText((HWND)hDlg,IDC_CODE_NAME,CheatName.c_str());

			//Add Gameshark codes to screen			
			LPCSTR ReadPos = strrchr(String,'"') + 2;
			stdstr Buffer;
			do {
				char * End = strchr(ReadPos,',');
				if (End)
				{
					Buffer.append(ReadPos,End - ReadPos);
				} else {
					Buffer.append(ReadPos);
				}

				ReadPos = strchr(ReadPos,',');
				if (ReadPos != NULL) { 
					Buffer.append("\r\n");
					ReadPos += 1; 
				}
			} while (ReadPos);
			SetDlgItemText((HWND)hDlg,IDC_CHEAT_CODES,Buffer.c_str());

			//Add option values to screen			
			stdstr CheatOptionStr = _Settings->LoadStringIndex(Cheat_Options,_this->m_EditCheat);
			ReadPos = strchr(CheatOptionStr.c_str(),'$');
			Buffer.erase();
			if (ReadPos) {
				ReadPos += 1;
				do {
					char * End = strchr(ReadPos,',');
					if (End)
					{
						Buffer.append(ReadPos,End - ReadPos);
					} else {
						Buffer.append(ReadPos);
					}
					ReadPos = strchr(ReadPos,'$');
					if (ReadPos != NULL) { 
						Buffer.append("\r\n");
						ReadPos += 1; 
					}
				} while (ReadPos);
			}
			SetDlgItemText((HWND)hDlg,IDC_CHEAT_OPTIONS,Buffer.c_str());
			
			//Add cheat Notes
			stdstr CheatNotesStr = _Settings->LoadStringIndex(Cheat_Notes,_this->m_EditCheat);
			SetDlgItemText((HWND)hDlg,IDC_NOTES,CheatNotesStr.c_str());

		
			SendMessage((HWND)hDlg,WM_COMMAND, MAKELPARAM(IDC_CHEAT_CODES, EN_CHANGE), (LPARAM)GetDlgItem((HWND)hDlg,IDC_CHEAT_CODES));
			SetDlgItemText((HWND)hDlg,IDC_ADD,GS(CHEAT_EDITCHEAT_UPDATE));

			_this->RecordCheatValues(hDlg);
		}
		break;
	default:
		return false;
	}
	return true;
}

int CALLBACK CCheats::CheatListProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCheats   * _this = (CCheats *)lParam;
			SetProp((HWND)hDlg,"Class",_this);

			DWORD Style;
			RECT rcList;
			RECT rcButton;

			SetWindowText(GetDlgItem((HWND)hDlg,IDC_CHEATSFRAME),GS(CHEAT_LIST_FRAME));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_NOTESFRAME),GS(CHEAT_NOTES_FRAME));
			SetWindowText(GetDlgItem((HWND)hDlg,IDC_UNMARK),GS(CHEAT_MARK_NONE));

			GetWindowRect(GetDlgItem((HWND)hDlg, IDC_CHEATSFRAME), &rcList);
			GetWindowRect(GetDlgItem((HWND)hDlg, IDC_UNMARK), &rcButton);

			_this->m_hCheatTree = (WND_HANDLE)CreateWindowEx(WS_EX_CLIENTEDGE,WC_TREEVIEW,"",
					WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | TVS_HASLINES | 
					TVS_HASBUTTONS | TVS_LINESATROOT  | TVS_DISABLEDRAGDROP |WS_TABSTOP|
					TVS_FULLROWSELECT, 8, 15, rcList.right-rcList.left-16, 
					rcButton.top-rcList.top-22, (HWND)hDlg, (HMENU)IDC_MYTREE, GetModuleHandle(NULL), NULL);
			Style = GetWindowLong((HWND)_this->m_hCheatTree,GWL_STYLE);					
			SetWindowLong((HWND)_this->m_hCheatTree,GWL_STYLE,TVS_CHECKBOXES |TVS_SHOWSELALWAYS| Style);

			//Creats an image list from the bitmap in the resource section
			HIMAGELIST hImageList;
			HBITMAP hBmp;

			hImageList = ImageList_Create( 16,16, ILC_COLOR | ILC_MASK, 40, 40);
			hBmp = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_TRI_STATE));
			ImageList_AddMasked(hImageList,hBmp, RGB(255,0,255));
			DeleteObject(hBmp);
			
			TreeView_SetImageList((HWND)_this->m_hCheatTree,hImageList,TVSIL_STATE);

			_this->m_hSelectedItem = NULL;

		}
		break;
	case WM_COMMAND:
		{
			CCheats   * _this = (CCheats *)GetProp((HWND)hDlg,"Class");

			switch (LOWORD(wParam)) {
			case ID_POPUP_ADDNEWCHEAT:
				//DialogBox(hInst, MAKEINTRESOURCE(IDD_Cheats_Add),hDlg,(DLGPROC)CheatAddProc);
				break;
			case ID_POPUP_EDIT:
				//DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_Cheats_Edit),hDlg,(DLGPROC)CheatEditProc,(LPARAM)hSelectedItem);
				break;
			case ID_POPUP_DELETE:
				{
					TVITEM item;

					int Response = MessageBox((HWND)hDlg,GS(MSG_DEL_SURE),GS(MSG_DEL_TITLE),MB_YESNO|MB_ICONQUESTION);
					if (Response != IDYES) { break; }

					//Delete selected cheat
					item.hItem = (HTREEITEM)_this->m_hSelectedItem;
					item.mask = TVIF_PARAM ;
					TreeView_GetItem((HWND)_this->m_hCheatTree,&item);

					_this->ChangeChildrenStatus((WND_HANDLE)TVI_ROOT,false); 
					_this->DeleteCheat(item.lParam);
					_this->RefreshCheatManager();
				}
				break;
			case IDC_UNMARK: 
				_this->ChangeChildrenStatus((WND_HANDLE)TVI_ROOT,false); 
				_this->m_CheatSelectionChanged = true;
				break;
			}
		}
		break;
	case WM_NOTIFY:
		{
			CCheats   * _this = (CCheats *)GetProp((HWND)hDlg,"Class");

			if (_this->m_DeleteingEntries)
			{
				break;
			}
			LPNMHDR lpnmh = (LPNMHDR) lParam;
    
			if ((lpnmh->code  == NM_RCLICK) && (lpnmh->idFrom == IDC_MYTREE))
			{

				//Work out what item is selected
				TVHITTESTINFO ht = {0};
				DWORD dwpos = GetMessagePos();

				// include <windowsx.h> and <windows.h> header files
				ht.pt.x = GET_X_LPARAM(dwpos);
				ht.pt.y = GET_Y_LPARAM(dwpos);
				MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

				TreeView_HitTest(lpnmh->hwndFrom, &ht);
				_this->m_hSelectedItem = (WND_HANDLE)ht.hItem;
				if (_Settings->LoadBool(UserInterface_BasicMode)) { return true; }

				//Show Menu
				HMENU hMenu = LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CHEAT_MENU));
				HMENU hPopupMenu = GetSubMenu(hMenu,0);
				POINT Mouse;

				GetCursorPos(&Mouse);
					
				MenuSetText((MENU_HANDLE)hPopupMenu, 0, GS(CHEAT_ADDNEW), NULL);
				MenuSetText((MENU_HANDLE)hPopupMenu, 1, GS(CHEAT_EDIT), NULL);
				MenuSetText((MENU_HANDLE)hPopupMenu, 3, GS(CHEAT_DELETE), NULL);

				if (_this->m_hSelectedItem == NULL || 
					TreeView_GetChild((HWND)_this->m_hCheatTree,_this->m_hSelectedItem) != NULL) 
				{ 
					DeleteMenu(hPopupMenu,3,MF_BYPOSITION);
					DeleteMenu(hPopupMenu,2,MF_BYPOSITION);
					DeleteMenu(hPopupMenu,1,MF_BYPOSITION);
				}
				TrackPopupMenu((HMENU)hPopupMenu, 0, Mouse.x, Mouse.y, 0,(HWND)hDlg, NULL);
				DestroyMenu(hMenu);
			}
			if ((lpnmh->code  == NM_CLICK) && (lpnmh->idFrom == IDC_MYTREE))
			{
				TVHITTESTINFO ht = {0};
				DWORD dwpos = GetMessagePos();

				// include <windowsx.h> and <windows.h> header files
				ht.pt.x = GET_X_LPARAM(dwpos);
				ht.pt.y = GET_Y_LPARAM(dwpos);
				MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

				TreeView_HitTest(lpnmh->hwndFrom, &ht);

				if(TVHT_ONITEMSTATEICON & ht.flags)
				{

					switch (TV_GetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem)) {
					case TV_STATE_CLEAR:
					case TV_STATE_INDETERMINATE: 
						//Make sure that the item has a valid code extenstion selected
						if (TreeView_GetChild((HWND)_this->m_hCheatTree, (WND_HANDLE)ht.hItem) == NULL) {
							TVITEM item;
							item.mask  = TVIF_PARAM ;
							item.hItem = (HTREEITEM)ht.hItem;
							TreeView_GetItem((HWND)_this->m_hCheatTree,&item);
							stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,item.lParam);
							if (CheatUsesCodeExtensions(LineEntry)) 
							{
								stdstr CheatExtension;
								if (!_Settings->LoadStringIndex(Cheat_Extension,item.lParam,CheatExtension))
								{
									SendMessage((HWND)hDlg, UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
									TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_CLEAR); 
									break;
								}
							}
						}
						TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_CHECKED); 
						_this->ChangeChildrenStatus((WND_HANDLE)ht.hItem,true);
						_this->CheckParentStatus((WND_HANDLE)TreeView_GetParent((HWND)_this->m_hCheatTree,ht.hItem));
						break;
					case TV_STATE_CHECKED: 
						TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_CLEAR); 
						_this->ChangeChildrenStatus((WND_HANDLE)ht.hItem,false);
						_this->CheckParentStatus((WND_HANDLE)TreeView_GetParent((HWND)_this->m_hCheatTree,ht.hItem));
						break;
					}
					switch (TV_GetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem)) {
					case TV_STATE_CHECKED: TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_INDETERMINATE); break;
					case TV_STATE_CLEAR:   TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_CHECKED); break;
					case TV_STATE_INDETERMINATE: TV_SetCheckState(_this->m_hCheatTree,(WND_HANDLE)ht.hItem,TV_STATE_CLEAR); break;
					}

					_this->m_CheatSelectionChanged = true;
				}
			}
		   if ((lpnmh->code  == NM_DBLCLK) && (lpnmh->idFrom == IDC_MYTREE))
		   {
				TVHITTESTINFO ht = {0};
				DWORD dwpos = GetMessagePos();

				// include <windowsx.h> and <windows.h> header files
				ht.pt.x = GET_X_LPARAM(dwpos);
				ht.pt.y = GET_Y_LPARAM(dwpos);
				MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

				TreeView_HitTest(lpnmh->hwndFrom, &ht);

				if(TVHT_ONITEMLABEL & ht.flags)
				{
					PostMessage((HWND)hDlg, UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
				}
			}
			if ((lpnmh->code  == TVN_SELCHANGED) && (lpnmh->idFrom == IDC_MYTREE)) {
				HTREEITEM hItem;

				hItem = TreeView_GetSelection((HWND)_this->m_hCheatTree);
				if (TreeView_GetChild((HWND)_this->m_hCheatTree,hItem) == NULL) { 
					TVITEM item;

					item.mask = TVIF_PARAM ;
					item.hItem = hItem;
					TreeView_GetItem((HWND)_this->m_hCheatTree,&item);

					stdstr Notes(_Settings->LoadStringIndex(Cheat_Notes,item.lParam));
					SetDlgItemText((HWND)hDlg,IDC_NOTES,Notes.c_str());
					if (_this->m_AddCheat)
					{
						SendMessage((HWND)_this->m_AddCheat,WM_EDITCHEAT,item.lParam,0); //edit cheat 
					}
				} else {
					SetDlgItemText((HWND)hDlg,IDC_NOTES,"");
				}
			}
		}
		break;
	case UM_CHANGECODEEXTENSION:
		{
			CCheats   * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
;
			//Get the selected item
			_this->m_hSelectedItem = (WND_HANDLE)lParam;
			TVITEM item;
			item.mask = TVIF_PARAM ;
			item.hItem = (HTREEITEM)_this->m_hSelectedItem;
			TreeView_GetItem((HWND)_this->m_hCheatTree,&item);
		
			//Make sure the selected line can use code extensions	
			stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,item.lParam);
			if (!CheatUsesCodeExtensions(LineEntry)) { break; }

			stdstr Options;
			if (_Settings->LoadStringIndex(Cheat_Options,item.lParam,Options) && Options.length() > 0)
			{
				DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_CodeEx),(HWND)hDlg,(DLGPROC)CheatsCodeExProc,(LPARAM)_this);
			} else {
				stdstr Range;
				if (_Settings->LoadStringIndex(Cheat_Range,item.lParam,Range) && Range.length() > 0) 
				{
					DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_Range),(HWND)hDlg,(DLGPROC)CheatsCodeQuantProc,(LPARAM)_this);
				}
			}
			
			//Update cheat listing with new extention
			stdstr CheatName(_this->GetCheatName(item.lParam,true));
			char * Cheat = strrchr(CheatName.c_str(),'\\');
			if (Cheat == NULL) { 
				Cheat = const_cast<char *>(CheatName.c_str()); 
			} else {
				Cheat += 1;
			}
			item.mask = TVIF_TEXT;
			item.pszText = Cheat;
			item.cchTextMax = CheatName.length();
			TreeView_SetItem((HWND)_this->m_hCheatTree,&item);
		}
		break;
	default:
		return false;
	}
	return true;
}

int CALLBACK CCheats::CheatsCodeExProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCheats   * _this     = (CCheats *)lParam;
			SetProp((HWND)hDlg,"Class",_this);
			
			//Find the cheat Number of the option being selected
			TVITEM item;
			item.hItem = (HTREEITEM)_this->m_hSelectedItem;
			item.mask = TVIF_PARAM ;
			TreeView_GetItem((HWND)_this->m_hCheatTree,&item);
			stdstr CheatName = _this->GetCheatName(item.lParam,false);			

			//Set up language support for dialog
			SetWindowText((HWND)hDlg, GS(CHEAT_CODE_EXT_TITLE));
			SetDlgItemText((HWND)hDlg,IDC_NOTE, GS(CHEAT_CODE_EXT_TXT));
			SetDlgItemText((HWND)hDlg,IDOK, GS(CHEAT_OK));
			SetDlgItemText((HWND)hDlg,IDCANCEL, GS(CHEAT_CANCEL));
			SetDlgItemText((HWND)hDlg,IDC_CHEAT_NAME,CheatName.c_str());			
			
			//Read through and add all options to the list box
			stdstr Options(_Settings->LoadStringIndex(Cheat_Options,item.lParam));
			stdstr CurrentExt(_Settings->LoadStringIndex(Cheat_Extension,item.lParam));
			const char * ReadPos = Options.c_str();
			while (*ReadPos != 0) {
				const char * NextComma = strchr(ReadPos,',');
				int len = NextComma == NULL ? strlen(ReadPos) : NextComma - ReadPos;
				stdstr CheatExt(ReadPos);
				CheatExt.resize(len);
				int index = SendMessage(GetDlgItem((HWND)hDlg,IDC_CHEAT_LIST),LB_ADDSTRING,0,(LPARAM)CheatExt.c_str());
				if (CheatExt == CurrentExt) { 
					SendMessage(GetDlgItem((HWND)hDlg,IDC_CHEAT_LIST),LB_SETCURSEL,index,0);
				}
				//Move to next entry or end
				ReadPos = NextComma ? NextComma + 1 : ReadPos + strlen(ReadPos); 
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHEAT_LIST:
			if (HIWORD(wParam) == LBN_DBLCLK) { PostMessage((HWND)hDlg,WM_COMMAND,IDOK,0); break; }
			break;			
		case IDOK:
			{
				CCheats * _this = (CCheats *)GetProp((HWND)hDlg,"Class");

				//Find the cheat Number of the option being selected
				TVITEM item;
				item.hItem = (HTREEITEM)_this->m_hSelectedItem;
				item.mask = TVIF_PARAM ;
				TreeView_GetItem((HWND)_this->m_hCheatTree,&item);

				//Get the selected cheat extension
				char CheatExten[300];
				int index = SendMessage(GetDlgItem((HWND)hDlg,IDC_CHEAT_LIST),LB_GETCURSEL,0,0);
				if (index < 0) { index = 0; }
				SendMessage(GetDlgItem((HWND)hDlg,IDC_CHEAT_LIST),LB_GETTEXT,index,(LPARAM)CheatExten);
				
				_Settings->SaveStringIndex(Cheat_Extension,item.lParam,CheatExten);
				_this->m_CheatSelectionChanged = true;
			}
			RemoveProp((HWND)hDlg,"Class");
			EndDialog((HWND)hDlg,0);
			break;
		case IDCANCEL:
			RemoveProp((HWND)hDlg,"Class");
			EndDialog((HWND)hDlg,0);
			break;
		}
	default:
		return false;
	}
	return true;
}

int CALLBACK CCheats::CheatsCodeQuantProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) {
	static WORD Start, Stop, SelStart, SelStop;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCheats   * _this     = (CCheats *)lParam;
			SetProp((HWND)hDlg,"Class",_this);
			
			//Find the cheat Number of the option being selected
			TVITEM item;
			item.hItem = (HTREEITEM)_this->m_hSelectedItem;
			item.mask = TVIF_PARAM ;
			TreeView_GetItem((HWND)_this->m_hCheatTree,&item);
			stdstr CheatName = _this->GetCheatName(item.lParam,false);			
			stdstr RangeNote(_Settings->LoadStringIndex(Cheat_RangeNotes, item.lParam));
			stdstr Range(_Settings->LoadStringIndex(Cheat_Range,item.lParam));
			stdstr Value(_Settings->LoadStringIndex(Cheat_Extension,item.lParam));
	
			//Set up language support for dialog
			SetWindowText((HWND)hDlg, GS(CHEAT_CODE_EXT_TITLE));
			SetDlgItemText((HWND)hDlg, IDC_DIGITAL_TEXT, GS(CHEAT_CHOOSE_VALUE));
			SetDlgItemText((HWND)hDlg, IDC_VALUE_TEXT, GS(CHEAT_VALUE));
			SetDlgItemText((HWND)hDlg, IDC_NOTES_TEXT, GS(CHEAT_NOTES));
			SetDlgItemText((HWND)hDlg,IDC_NOTES,RangeNote.c_str());
			SetDlgItemText((HWND)hDlg,IDC_CHEAT_NAME,CheatName.c_str());
			SetDlgItemText((HWND)hDlg,IDC_VALUE,Value.c_str());

			Start = (WORD)(Range.c_str()[0] == '$'?AsciiToHex(&Range.c_str()[1]):atol(Range.c_str()));
			const char * ReadPos  = strrchr(Range.c_str(),'-');
			if (ReadPos != NULL) {
				Stop = (WORD)(ReadPos[1] == '$'?AsciiToHex(&ReadPos[2]):atol(&ReadPos[1]));			
			} else {
				Stop = 0;
			}

			char Text[500];
			sprintf(Text,"%s $%X %s $%X",GS(CHEAT_FROM),Start,GS(CHEAT_TO),Stop);
			SetDlgItemText((HWND)hDlg,IDC_RANGE,Text);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_VALUE:
			if (HIWORD(wParam) == EN_UPDATE) {
				TCHAR szTmp[10], szTmp2[10];
				DWORD Value;
				GetDlgItemText((HWND)hDlg,IDC_VALUE,szTmp,sizeof(szTmp));
				Value = szTmp[0] =='$'?AsciiToHex(&szTmp[1]):AsciiToHex(szTmp);
				if (Value > Stop)  { Value = Stop; }
				if (Value < Start) { Value = Start; }
				sprintf(szTmp2,"$%X",Value);
				if (strcmp(szTmp,szTmp2) != 0) {
					SetDlgItemText((HWND)hDlg,IDC_VALUE,szTmp2);
					if (SelStop == 0) { SelStop = strlen(szTmp2); SelStart = SelStop; }
					SendDlgItemMessage((HWND)hDlg,IDC_VALUE,EM_SETSEL,(WPARAM)SelStart,(LPARAM)SelStop);
				} else {
					WORD NewSelStart, NewSelStop;
					SendDlgItemMessage((HWND)hDlg,IDC_VALUE,EM_GETSEL,(WPARAM)&NewSelStart,(LPARAM)&NewSelStop);
					if (NewSelStart != 0) { SelStart = NewSelStart; SelStop = NewSelStop; }
				}
			}
			break;
		case IDOK:
			{
				CCheats * _this = (CCheats *)GetProp((HWND)hDlg,"Class");

				//Find the cheat Number of the option being selected
				TVITEM item;
				item.hItem = (HTREEITEM)_this->m_hSelectedItem;
				item.mask = TVIF_PARAM ;
				TreeView_GetItem((HWND)_this->m_hCheatTree,&item);

				//Get the selected cheat extension
				TCHAR CheatExten[300], szTmp[10];
				DWORD Value;

				GetDlgItemText((HWND)hDlg,IDC_VALUE,szTmp,sizeof(szTmp));
				Value = szTmp[0] =='$'?AsciiToHex(&szTmp[1]):AsciiToHex(szTmp);
				if (Value > Stop) { Value = Stop; }
				if (Value < Start) { Value = Start; }
				sprintf(CheatExten,"$%X",Value);
				
				_Settings->SaveStringIndex(Cheat_Extension, item.lParam,CheatExten);
				_this->m_CheatSelectionChanged = true;
			}
			RemoveProp((HWND)hDlg,"Class");
			EndDialog((HWND)hDlg,0);
			break;
		case IDCANCEL:
			RemoveProp((HWND)hDlg,"Class");
			EndDialog((HWND)hDlg,0);
			break;
		}
	default:
		return false;
	}
	return true;
}

bool CCheats::IsCheatMessage( MSG * msg )
{
	if (m_Window)
	{
		return IsDialogMessage((HWND)m_Window,msg) != 0;
	}
	return false;
}

int CALLBACK CCheats::ManageCheatsProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCheats   * _this = (CCheats *)lParam;
			SetProp((HWND)hDlg,"Class",_this);
			_this->m_Window = hDlg;

			WINDOWPLACEMENT WndPlac;
			WndPlac.length = sizeof(WndPlac);
			GetWindowPlacement((HWND)hDlg, &WndPlac);

			SetWindowText((HWND)hDlg, GS(CHEAT_TITLE));
			_this->m_hSelectCheat = (WND_HANDLE)CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_List),(HWND)hDlg,(DLGPROC)CheatListProc,(LPARAM)_this);
			SetWindowPos((HWND)_this->m_hSelectCheat,HWND_TOP, 5, 8, 0, 0, SWP_NOSIZE);
			ShowWindow((HWND)_this->m_hSelectCheat,SW_SHOW);

			RECT * rc = &WndPlac.rcNormalPosition;
			if (_Settings->LoadDword(UserInterface_BasicMode)) 
			{
				RECT * rcAdd = (RECT *)_this->m_rcAdd, * rcList = (RECT *)_this->m_rcList;
				GetWindowRect(GetDlgItem((HWND)_this->m_hSelectCheat, IDC_CHEATSFRAME), rcList);
				_this->m_MinSizeDlg = rcList->right - rcList->left + 16;
				_this->m_MaxSizeDlg = _this->m_MinSizeDlg;

				_this->m_DialogState = CONTRACTED;
				WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
				SetWindowPlacement((HWND)hDlg, &WndPlac);

				ShowWindow(GetDlgItem((HWND)hDlg, IDC_STATE),SW_HIDE);
			} else {
				_this->m_AddCheat = (WND_HANDLE)CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_Add),(HWND)hDlg,(DLGPROC)CheatAddProc,(LPARAM)_this);
				SetWindowPos((HWND)_this->m_AddCheat, HWND_TOP, (rc->right - rc->left)/2, 8, 0, 0, SWP_NOSIZE);
				ShowWindow((HWND)_this->m_AddCheat,SW_HIDE);


				RECT * rcAdd = (RECT *)_this->m_rcAdd, * rcList = (RECT *)_this->m_rcList;
				GetWindowRect(GetDlgItem((HWND)_this->m_hSelectCheat, IDC_CHEATSFRAME), rcList);
				GetWindowRect(GetDlgItem((HWND)_this->m_AddCheat, IDC_ADDCHEATSFRAME), rcAdd);
				_this->m_MinSizeDlg = rcList->right - rcList->left + 32;
				_this->m_MaxSizeDlg = rcAdd->right  - rcList->left + 32;

				_this->m_DialogState = CONTRACTED;
				WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
				SetWindowPlacement((HWND)hDlg, &WndPlac);

				GetClientRect((HWND)hDlg, rc);
				HWND hStateButton = GetDlgItem((HWND)hDlg, IDC_STATE);
				SetWindowPos(hStateButton, HWND_TOP, (rc->right - rc->left) - 16, 0, 16, rc->bottom - rc->top, 0);
				HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RIGHT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
				SendDlgItemMessage((HWND)hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
			}

			//re-center cheat window
			RECT rcDlg, rcParent;
			GetWindowRect((HWND)hDlg, &rcDlg);
			GetWindowRect(GetParent((HWND)hDlg), &rcParent);

			int DlgWidth = rcDlg.right - rcDlg.left;
			int DlgHeight = rcDlg.bottom - rcDlg.top;
			
			int X = (((rcParent.right - rcParent.left) - DlgWidth) / 2) + rcParent.left;
			int Y = (((rcParent.bottom - rcParent.top) - DlgHeight) / 2) + rcParent.top;

			SetWindowPos((HWND)hDlg,NULL,X,Y,0,0,SWP_NOZORDER|SWP_NOSIZE);

			_this->RefreshCheatManager();
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			{
				CCheats * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
				if (_this->m_AddCheat) {
					DestroyWindow((HWND)_this->m_AddCheat);
					_this->m_AddCheat = NULL;
				}
				_this->m_Window = NULL;
			}
			RemoveProp((HWND)hDlg,"Class");
			EndDialog((HWND)hDlg,0);
			break;
		case IDC_STATE:
			{
				CCheats * _this = (CCheats *)GetProp((HWND)hDlg,"Class");
				WINDOWPLACEMENT WndPlac;
				WndPlac.length = sizeof(WndPlac);
				GetWindowPlacement((HWND)hDlg, &WndPlac);
	
				if (_this->m_DialogState == CONTRACTED)
				{
					_this->m_DialogState = EXPANDED;
					WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MaxSizeDlg;
					SetWindowPlacement((HWND)hDlg, &WndPlac);

					RECT clientrect;
					GetClientRect((HWND)hDlg, &clientrect);
					HWND hStateButton = GetDlgItem((HWND)hDlg, IDC_STATE);
					SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);

					HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LEFT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
					SendDlgItemMessage((HWND)hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
		
					ShowWindow((HWND)_this->m_AddCheat,SW_SHOW);
				}
				else
				{
					_this->m_DialogState = CONTRACTED;
					WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
					SetWindowPlacement((HWND)hDlg, &WndPlac);

		            HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RIGHT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
					SendDlgItemMessage((HWND)hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);

					RECT clientrect;
					GetClientRect((HWND)hDlg, &clientrect);
					HWND hStateButton = GetDlgItem((HWND)hDlg, IDC_STATE);
					SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);
		
					ShowWindow((HWND)_this->m_AddCheat,SW_HIDE);
				}
			}
			break;
		}
		break;
	default:
		return false;
	}
	return true;
}

bool CCheats::TV_SetCheckState(WND_HANDLE hwndTreeView, WND_HANDLE hItem, TV_CHECK_STATE state)
{
    TVITEM tvItem;

    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = (HTREEITEM)hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    /*Image 1 in the tree-view check box image list is the
    unchecked box. Image 2 is the checked box.*/

	switch (state) {
	case TV_STATE_CHECKED: tvItem.state = INDEXTOSTATEIMAGEMASK(1); break;
	case TV_STATE_CLEAR: tvItem.state = INDEXTOSTATEIMAGEMASK(2); break;
	case TV_STATE_INDETERMINATE: tvItem.state = INDEXTOSTATEIMAGEMASK(3); break;
	default: tvItem.state = INDEXTOSTATEIMAGEMASK(0); break;
	}
    return TreeView_SetItem((HWND)hwndTreeView, &tvItem) != 0;
}

int CCheats::TV_GetCheckState(WND_HANDLE hwndTreeView, WND_HANDLE hItem)
{
    TVITEM tvItem;

    // Prepare to receive the desired information.
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = (HTREEITEM)hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    // Request the information.
    TreeView_GetItem((HWND)hwndTreeView, &tvItem);

    // Return zero if it's not checked, or nonzero otherwise.
	switch (tvItem.state >> 12) {
	case 1: return TV_STATE_CHECKED;
	case 2: return TV_STATE_CLEAR;
	case 3: return TV_STATE_INDETERMINATE;
	}
	return ((int)(tvItem.state >> 12) -1);
}

void CCheats::MenuSetText ( MENU_HANDLE hMenu, int MenuPos, const char * Title, char * ShotCut) {
	MENUITEMINFO MenuInfo;
	char String[256];

	if (Title == NULL || strlen(Title) == 0) { return; }

	memset(&MenuInfo, 0, sizeof(MENUITEMINFO));
	MenuInfo.cbSize = sizeof(MENUITEMINFO);
	MenuInfo.fMask = MIIM_TYPE;
	MenuInfo.fType = MFT_STRING;
	MenuInfo.fState = MFS_ENABLED;
	MenuInfo.dwTypeData = String;
	MenuInfo.cch = 256;

	GetMenuItemInfo((HMENU)hMenu,MenuPos,true,&MenuInfo);
	if (strchr(Title,'\t') != NULL) { *(strchr(Title,'\t')) = '\0'; }
	strcpy(String,Title);
	if (ShotCut) { sprintf(String,"%s\t%s",String,ShotCut); }
	SetMenuItemInfo((HMENU)hMenu,MenuPos,true,&MenuInfo);
}

void CCheats::DeleteCheat(int Index)
{
	for (int CheatNo = Index; CheatNo < MaxCheats; CheatNo ++ ) 
	{
		stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,CheatNo + 1);
		if (LineEntry.empty()) 
		{
			_Settings->DeleteSettingIndex(Cheat_RangeNotes,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Range,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Options,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Notes,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Extension,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Entry,CheatNo);
			_Settings->DeleteSettingIndex(Cheat_Active,CheatNo);
			break; 
		}
		stdstr Value;
		if (_Settings->LoadStringIndex(Cheat_RangeNotes,CheatNo+1,Value))
		{
			_Settings->SaveStringIndex(Cheat_RangeNotes,CheatNo, Value);
		} else {
			_Settings->DeleteSettingIndex(Cheat_RangeNotes,CheatNo);
		}

		if (_Settings->LoadStringIndex(Cheat_Range,CheatNo+1,Value))
		{
			_Settings->SaveStringIndex(Cheat_Range,CheatNo, Value);
		} else {
			_Settings->DeleteSettingIndex(Cheat_Range,CheatNo);
		}

		if (_Settings->LoadStringIndex(Cheat_Options,CheatNo+1,Value))
		{
			_Settings->SaveStringIndex(Cheat_Options,CheatNo, Value);
		} else {
			_Settings->DeleteSettingIndex(Cheat_Options,CheatNo);
		}
		
		if (_Settings->LoadStringIndex(Cheat_Notes,CheatNo+1,Value))
		{
			_Settings->SaveStringIndex(Cheat_Notes,CheatNo, Value);
		} else {
			_Settings->DeleteSettingIndex(Cheat_Notes,CheatNo);
		}

		if (_Settings->LoadStringIndex(Cheat_Extension,CheatNo+1,Value))
		{
			_Settings->SaveStringIndex(Cheat_Extension,CheatNo, Value);
		} else {
			_Settings->DeleteSettingIndex(Cheat_Extension,CheatNo);
		}

		bool bValue;
		if (_Settings->LoadBoolIndex(Cheat_Active,CheatNo+1,bValue))
		{
			_Settings->SaveBoolIndex(Cheat_Active,CheatNo, bValue);
		} else {
			_Settings->DeleteSettingIndex(Cheat_Active,CheatNo);
		}
		_Settings->SaveStringIndex(Cheat_Entry,CheatNo, LineEntry);		
	}
	CSettingTypeCheats::FlushChanges();
}

void CCheats::ChangeChildrenStatus(WND_HANDLE hParent, bool Checked) {
	HTREEITEM hItem = TreeView_GetChild((HWND)m_hCheatTree, hParent);
	if (hItem == NULL) {
		if ((HTREEITEM)hParent == TVI_ROOT) { return; }

		TVITEM item;
		item.mask = TVIF_PARAM ;
		item.hItem = (HTREEITEM)hParent;
		TreeView_GetItem((HWND)m_hCheatTree,&item);

		//if cheat uses a extension and it is not set then do not set it
		if (Checked) {
			stdstr LineEntry = _Settings->LoadStringIndex(Cheat_Entry,item.lParam);
			if (CheatUsesCodeExtensions(LineEntry)) {
				stdstr CheatExten;
				if (!_Settings->LoadStringIndex(Cheat_Extension,item.lParam,CheatExten) || CheatExten.empty())
				{
					return;
				}
			}
		}
		//Save Cheat
		TV_SetCheckState(m_hCheatTree,hParent,Checked?TV_STATE_CHECKED:TV_STATE_CLEAR); 
		_Settings->SaveBoolIndex(Cheat_Active,item.lParam,Checked);
		return; 
	}
	TV_CHECK_STATE state = TV_STATE_UNKNOWN;
	while (hItem != NULL) {
		TV_CHECK_STATE ChildState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree,(WND_HANDLE)hItem);
		if ((ChildState != TV_STATE_CHECKED || !Checked) && 
			(ChildState != TV_STATE_CLEAR   || Checked))
		{
			ChangeChildrenStatus((WND_HANDLE)hItem,Checked);
		}
		ChildState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree,(WND_HANDLE)hItem);
		if (state == TV_STATE_UNKNOWN) { state = ChildState; }
		if (state != ChildState) { state = TV_STATE_INDETERMINATE; }
		hItem = TreeView_GetNextSibling((HWND)m_hCheatTree,hItem);
	}	
	if (state != TV_STATE_UNKNOWN) {
		TV_SetCheckState(m_hCheatTree,hParent,state); 
	}
}

void CCheats::CheckParentStatus(WND_HANDLE hParent) {
	TV_CHECK_STATE CurrentState, InitialState;
	HTREEITEM hItem;

	if (!hParent) { return; }
	hItem = TreeView_GetChild((HWND)m_hCheatTree, (HTREEITEM)hParent);	
	InitialState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree,hParent);
	CurrentState = (TV_CHECK_STATE)TV_GetCheckState((WND_HANDLE)m_hCheatTree,(WND_HANDLE)hItem);
	
	while (hItem != NULL) {
		if (TV_GetCheckState((WND_HANDLE)m_hCheatTree,(WND_HANDLE)hItem) != CurrentState) { 
			CurrentState = TV_STATE_INDETERMINATE; 
			break; 
		}
		hItem = TreeView_GetNextSibling((HWND)m_hCheatTree,hItem);
	}
	TV_SetCheckState((WND_HANDLE)m_hCheatTree,(WND_HANDLE)hParent,CurrentState); 
	if (InitialState != CurrentState) { 
		CheckParentStatus((WND_HANDLE)TreeView_GetParent((HWND)m_hCheatTree,(HTREEITEM)hParent));
	}
}

stdstr CCheats::ReadCodeString (WND_HANDLE hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat ) {
	int numlines, linecount, len;
	char str[128];
	int i;
	char* formatnormal =   "XXXXXXXX XXXX";
	char* formatoptionlb = "XXXXXXXX XX??";
	char* formatoptionw =  "XXXXXXXX ????";
	char tempformat[128];

	validcodes = true;
	nooptions = true;
	codeformat = -1;
	int numcodes = 0;

	char codestring[2048];
	memset(codestring, '\0', sizeof(codestring));

	numlines = SendDlgItemMessage((HWND)hDlg, IDC_CHEAT_CODES, EM_GETLINECOUNT, 0, 0);
	if (numlines == 0) { validcodes = false; }
	
	for (linecount=0; linecount<numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
	{
		memset(tempformat, 0, sizeof(tempformat));

		//str[0] = sizeof(str) > 255?255:sizeof(str);
		*(LPWORD)str = sizeof(str);
		len = SendDlgItemMessage((HWND)hDlg, IDC_CHEAT_CODES, EM_GETLINE, (WPARAM)linecount, (LPARAM)(LPCSTR)str);
		str[len] = 0;

		if (len <= 0) { continue; }

		for (i=0; i<128; i++) {
			if (((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9'))) { // Is hexvalue
				tempformat[i] = 'X';
			}
			if ((str[i] == ' ') || (str[i] == '?')) {
				tempformat[i] = str[i];
			}
			if (str[i] == 0) { break; }
		}
		if (strcmp(tempformat, formatnormal) == 0) {
			strcat(codestring, ",");
			strcat(codestring, str);
			numcodes++;
			if (codeformat < 0) codeformat = 0;
		}
		else if (strcmp(tempformat, formatoptionlb) == 0) {
			if (codeformat != 2) {
				strcat(codestring, ",");
				strcat(codestring, str);
				numcodes++;
				codeformat = 1;
				nooptions = false;
				validoptions = false;
			}
			else validcodes = false;
		}
		else if (strcmp(tempformat, formatoptionw) == 0) {
			if (codeformat != 1) {
				strcat(codestring, ",");
				strcat(codestring, str);
				numcodes++;
				codeformat = 2;
				nooptions = false;
				validoptions = false;
			}
			else validcodes = false;
		}
		else {
			validcodes = false;
		}
	}
	if (strlen(codestring) == 0)
	{
		validcodes = false;
	}
	return codestring;
}

stdstr CCheats::ReadOptionsString(WND_HANDLE hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat)
{
	int numlines, linecount, len;
	char str[128];
	int i, j;

	int leftorder = 0;
	int rightorder = 0;


	validoptions = true;
	int numoptions = 0;

	char optionsstring[2048];
	memset(optionsstring, '\0', sizeof(optionsstring));

	numlines = SendDlgItemMessage((HWND)hDlg, IDC_CHEAT_OPTIONS, EM_GETLINECOUNT, 0, 0);
	
	for (linecount=0; linecount<numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
	{
		memset(str,0,sizeof(str));
		//str[0] = sizeof(str) > 255?255:sizeof(str);
		*(LPWORD)str = sizeof(str);
		len = SendDlgItemMessage((HWND)hDlg, IDC_CHEAT_OPTIONS, EM_GETLINE, (WPARAM)linecount, (LPARAM)(LPCSTR)str);
		str[len] = 0;

		if (len > 0) {
			switch (codeformat) {
			case 1: //option = lower byte
				if (len >= 2) {
					for (i=0; i<2; i++) {
						if (!(((str[i] >= 'a') && (str[i] <= 'f')) || ((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9')))) {
							validoptions = false;
							break;
						}
					}

					if ((str[2] != ' ') && (len > 2)) {
						validoptions = false;
						break;
					}

					for (j=0; j<2; j++) {
						str[j] = toupper(str[j]);
					}

					if (optionsstring[0] == 0)
					{
						strcat(optionsstring, "$");
					} else {
						strcat(optionsstring, ",$");
					}
					strcat(optionsstring, str);
					numoptions++;
				}
				else {
					validoptions = false;
					break;
				}
				break;

			case 2: //option = word
				if (len >= 4) {
					for (i=0; i<4; i++) {
						if (!(((str[i] >= 'a') && (str[i] <= 'f')) || ((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9')))) {
							validoptions = false;
							break;
						}
					}

					if (str[4] != ' ' && (len > 4)) {
						validoptions = false;
						break;
					}

					for (j=0; j<4; j++) {
						str[j] = toupper(str[j]);
					}

					strcat(optionsstring, ",$");
					strcat(optionsstring, str);
					numoptions++;
				}
				else {
					validoptions = false;
					break;
				}
				break;

			default:
				break;
			}
		}
	}

	if (numoptions < 1) validoptions = false;
	return optionsstring;
}



