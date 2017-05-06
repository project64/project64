#include "stdafx.h"

VIRTUAL_KEY CMenuShortCutKey::m_VirtualKeyList[] = {
    { "VK_LBUTTON", 0x01, "VK_LBUTTON" },
    { "VK_RBUTTON", 0x02, "VK_RBUTTON" },
    { "VK_CANCEL", 0x03, "VK_CANCEL" },
    { "VK_MBUTTON", 0x04, "VK_MBUTTON" },
    { "VK_XBUTTON1", 0x05, "VK_XBUTTON1" },
    { "VK_XBUTTON2", 0x06, "VK_XBUTTON2" },
    { "VK_BACK", 0x08, "VK_BACK" },
    { "VK_TAB", 0x09, "VK_TAB" },
    { "VK_CLEAR", 0x0C, "VK_CLEAR" },
    { "VK_RETURN", 0x0D, "Return" },
    { "VK_SHIFT", 0x10, "VK_SHIFT" },
    { "VK_CONTROL", 0x11, "VK_CONTROL" },
    { "VK_MENU", 0x12, "VK_MENU" },
    { "VK_PAUSE", 0x13, "Pause" },
    { "VK_CAPITAL", 0x14, "VK_CAPITAL" },
    { "VK_KANA", 0x15, "VK_KANA" },
    { "VK_HANGUL", 0x15, "VK_HANGUL" },
    { "VK_JUNJA", 0x17, "VK_JUNJA" },
    { "VK_FINAL", 0x18, "VK_FINAL" },
    { "VK_HANJA", 0x19, "VK_HANJA" },
    { "VK_KANJI", 0x19, "VK_KANJI" },
    { "VK_ESCAPE", 0x1B, "Esc" },
    { "VK_CONVERT", 0x1C, "VK_CONVERT" },
    { "VK_NONCONVERT", 0x1D, "VK_NONCONVERT" },
    { "VK_ACCEPT", 0x1E, "VK_ACCEPT" },
    { "VK_MODECHANGE", 0x1F, "VK_MODECHANGE" },
    { "VK_SPACE", 0x20, "Space" },
    { "VK_PRIOR", 0x21, "Page Up" },
    { "VK_NEXT", 0x22, "Page Down" },
    { "VK_END", 0x23, "End" },
    { "VK_HOME", 0x24, "Home" },
    { "VK_LEFT", 0x25, "Left" },
    { "VK_UP", 0x26, "Up" },
    { "VK_RIGHT", 0x27, "Right" },
    { "VK_DOWN", 0x28, "Down" },
    { "VK_SELECT", 0x29, "VK_SELECT" },
    { "VK_PRINT", 0x2A, "VK_PRINT" },
    { "VK_EXECUTE", 0x2B, "VK_EXECUTE" },
    { "VK_SNAPSHOT", 0x2C, "VK_SNAPSHOT" },
    { "VK_INSERT", 0x2D, "Insert" },
    { "VK_DELETE", 0x2E, "Delete" },
    { "VK_HELP", 0x2F, "Help" },
    { "VK_0", 0x30, "0" },
    { "VK_1", 0x31, "1" },
    { "VK_2", 0x32, "2" },
    { "VK_3", 0x33, "3" },
    { "VK_4", 0x34, "4" },
    { "VK_5", 0x35, "5" },
    { "VK_6", 0x36, "6" },
    { "VK_7", 0x37, "7" },
    { "VK_8", 0x38, "8" },
    { "VK_9", 0x39, "9" },
    { "VK_A", 0x41, "A" },
    { "VK_B", 0x42, "B" },
    { "VK_C", 0x43, "C" },
    { "VK_D", 0x44, "D" },
    { "VK_E", 0x45, "E" },
    { "VK_F", 0x46, "F" },
    { "VK_G", 0x47, "G" },
    { "VK_H", 0x48, "H" },
    { "VK_I", 0x49, "I" },
    { "VK_J", 0x4A, "J" },
    { "VK_K", 0x4B, "K" },
    { "VK_L", 0x4C, "L" },
    { "VK_M", 0x4D, "M" },
    { "VK_N", 0x4E, "N" },
    { "VK_O", 0x4F, "O" },
    { "VK_P", 0x50, "P" },
    { "VK_Q", 0x51, "Q" },
    { "VK_R", 0x52, "R" },
    { "VK_S", 0x53, "S" },
    { "VK_T", 0x54, "T" },
    { "VK_U", 0x55, "U" },
    { "VK_V", 0x56, "V" },
    { "VK_W", 0x57, "W" },
    { "VK_X", 0x58, "X" },
    { "VK_Y", 0x59, "Y" },
    { "VK_Z", 0x5A, "Z" },
    { "VK_LWIN", 0x5B, "VK_LWIN" },
    { "VK_RWIN", 0x5C, "VK_RWIN" },
    { "VK_APPS", 0x5D, "VK_APPS" },
    { "VK_SLEEP", 0x5D, "VK_SLEEP" },
    { "VK_NUMPAD0", 0x60, "Numpad0" },
    { "VK_NUMPAD1", 0x61, "Numpad1" },
    { "VK_NUMPAD2", 0x62, "Numpad2" },
    { "VK_NUMPAD3", 0x63, "Numpad3" },
    { "VK_NUMPAD4", 0x64, "Numpad4" },
    { "VK_NUMPAD5", 0x65, "Numpad5" },
    { "VK_NUMPAD6", 0x66, "Numpad6" },
    { "VK_NUMPAD7", 0x67, "Numpad7" },
    { "VK_NUMPAD8", 0x68, "Numpad8" },
    { "VK_NUMPAD9", 0x69, "Numpad9" },
    { "VK_MULTIPLY", 0x6A, "*" },
    { "VK_ADD", 0x6B, "+" },
    { "VK_SEPARATOR", 0x6C, "" },
    { "VK_SUBTRACT", 0x6D, "-" },
    { "VK_DECIMAL", 0x6E, "." },
    { "VK_DIVIDE", 0x6F, "/" },
    { "VK_F1", 0x70, "F1" },
    { "VK_F2", 0x71, "F2" },
    { "VK_F3", 0x72, "F3" },
    { "VK_F4", 0x73, "F4" },
    { "VK_F5", 0x74, "F5" },
    { "VK_F6", 0x75, "F6" },
    { "VK_F7", 0x76, "F7" },
    { "VK_F8", 0x77, "F8" },
    { "VK_F9", 0x78, "F9" },
    { "VK_F10", 0x79, "F10" },
    { "VK_F11", 0x7A, "F11" },
    { "VK_F12", 0x7B, "F12" },
    { "VK_F13", 0x7C, "F13" },
    { "VK_F14", 0x7D, "F14" },
    { "VK_F15", 0x7E, "F15" },
    { "VK_F16", 0x7F, "F16" },
    { "VK_F17", 0x80, "F17" },
    { "VK_F18", 0x81, "F18" },
    { "VK_F19", 0x82, "F19" },
    { "VK_F20", 0x83, "F20" },
    { "VK_F21", 0x84, "F21" },
    { "VK_F22", 0x85, "F22" },
    { "VK_F23", 0x86, "F23" },
    { "VK_F24", 0x87, "F24" },
    { "VK_NUMLOCK", 0x90, "Numlock" },
    { "VK_SCROLL", 0x91, "VK_SCROLL" },
    { "VK_LSHIFT", 0xA0, "VK_LSHIFT" },
    { "VK_RSHIFT", 0xA1, "VK_RSHIFT" },
    { "VK_LCONTROL", 0xA2, "VK_LCONTROL" },
    { "VK_RCONTROL", 0xA3, "VK_RCONTROL" },
    { "VK_LMENU", 0xA4, "VK_LMENU" },
    { "VK_RMENU", 0xA5, "VK_RMENU" },
    { "VK_BROWSER_BACK", 0xA6, "" },
    { "VK_BROWSER_FORWARD", 0xA7, "" },
    { "VK_BROWSER_REFRESH", 0xA8, "" },
    { "VK_BROWSER_STOP", 0xA9, "" },
    { "VK_BROWSER_SEARCH", 0xAA, "" },
    { "VK_BROWSER_FAVORITES", 0xAB, "" },
    { "VK_BROWSER_HOME", 0xAC, "" },
    { "VK_VOLUME_MUTE", 0xAD, "" },
    { "VK_VOLUME_DOWN", 0xAE, "" },
    { "VK_VOLUME_UP", 0xAF, "" },
    { "VK_MEDIA_NEXT_TRACK", 0xB0, "" },
    { "VK_MEDIA_PREV_TRACK", 0xB1, "" },
    { "VK_MEDIA_STOP", 0xB2, "" },
    { "VK_MEDIA_PLAY_PAUSE", 0xB3, "" },
    { "VK_LAUNCH_MAIL", 0xB4, "" },
    { "VK_LAUNCH_MEDIA_SELECT", 0xB5, "" },
    { "VK_LAUNCH_APP1", 0xB6, "" },
    { "VK_LAUNCH_APP2", 0xB7, "" },
    { "VK_OEM_1 (;:)", 0xBA, "" },
    { "VK_OEM_PLUS", 0xBB, "+" },
    { "VK_OEM_COMMA", 0xBC, "" },
    { "VK_OEM_MINUS", 0xBD, "-" },
    { "VK_OEM_PERIOD", 0xBE, "." },
    { "VK_OEM_2 (/?)", 0xBF, "" },
    { "VK_OEM_3 (`~)", 0xC0, "~" },
    { "VK_ATTN", 0xF6, "" },
    { "VK_CRSEL", 0xF7, "" },
    { "VK_EXSEL", 0xF8, "" },
    { "VK_EREOF", 0xF9, "" },
    { "VK_PLAY", 0xFA, "" },
    { "VK_ZOOM", 0xFB, "" },
    { "VK_NONAME", 0xFC, "" },
    { "VK_PA1", 0xFD, "" },
    { "VK_OEM_CLEAR", 0xFE }
};

CMenuShortCutKey::CMenuShortCutKey(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode, bool bUserAdded, bool bInactive) :
    m_key(key),
    m_bCtrl(bCtrl),
    m_bAlt(bAlt),
    m_bShift(bShift),
    m_AccessMode(AccessMode),
    m_bUserAdded(bUserAdded),
    m_bInactive(bInactive)
{
    m_ShortCutName = "";
    for (int i = 0, n = sizeof(m_VirtualKeyList) / sizeof(m_VirtualKeyList[0]); i < n; i++)
    {
        if (key == m_VirtualKeyList[i].Key)
        {
            m_ShortCutName = m_VirtualKeyList[i].KeyName;
            break;
        }
    }
    if (m_bShift) { m_ShortCutName.Format("Shift+%s", m_ShortCutName.c_str()); }
    if (m_bCtrl) { m_ShortCutName.Format("Ctrl+%s", m_ShortCutName.c_str()); }
    if (m_bAlt) { m_ShortCutName.Format("Alt+%s", m_ShortCutName.c_str()); }
}

VIRTUAL_KEY * CMenuShortCutKey::VirtualKeyList(int &Size)
{
    Size = sizeof(m_VirtualKeyList) / sizeof(m_VirtualKeyList[0]);
    return (VIRTUAL_KEY *)m_VirtualKeyList;
}

CMenuShortCutKey::RUNNING_STATE CMenuShortCutKey::RunningState(void)
{
    if (g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        return UISettingsLoadBool(UserInterface_InFullScreen) ? CMenuShortCutKey::RUNNING_STATE_FULLSCREEN : CMenuShortCutKey::RUNNING_STATE_WINDOWED;
    }
    return RUNNING_STATE_NOT_RUNNING;
}

bool CMenuShortCutKey::Match(WORD key, bool bCtrl, bool bAlt, bool bShift, RUNNING_STATE RunningState) const
{
    if (key != m_key) { return false; }
    if (bShift != m_bShift) { return false; }
    if (bCtrl != m_bCtrl) { return false; }
    if (bAlt != m_bAlt) { return false; }
    if (!Active(RunningState)) { return false; }
    return true;
}

bool CMenuShortCutKey::Active(RUNNING_STATE RunningState) const
{
    switch (RunningState)
    {
    case CMenuShortCutKey::RUNNING_STATE_NOT_RUNNING:
        if (m_AccessMode == CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING ||
            m_AccessMode == CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN ||
            m_AccessMode == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    case CMenuShortCutKey::RUNNING_STATE_WINDOWED:
        if (m_AccessMode == CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW ||
            m_AccessMode == CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN ||
            m_AccessMode == CMenuShortCutKey::ACCESS_GAME_RUNNING ||
            m_AccessMode == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    case CMenuShortCutKey::RUNNING_STATE_FULLSCREEN:
        if (m_AccessMode == CMenuShortCutKey::ACCESS_GAME_RUNNING_FULLSCREEN ||
            m_AccessMode == CMenuShortCutKey::ACCESS_GAME_RUNNING ||
            m_AccessMode == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

CShortCutItem::CShortCutItem(LanguageStringID Section, LanguageStringID Title, ACCESS_MODE Access)
{
    Reset(Section, Title, Access);
}

void CShortCutItem::Reset(LanguageStringID Section, LanguageStringID Title, ACCESS_MODE Access)
{
    m_Section = Section;
    m_Title = Title;
    m_Access = Access;
}

void CShortCutItem::AddShortCut(WORD key, bool bCtrl, bool bAlt, bool bShift, ACCESS_MODE AccessMode, bool bUserAdded, bool bInactive)
{
    m_AccelList.push_back(CMenuShortCutKey(key, bCtrl, bAlt, bShift, AccessMode, bUserAdded, bInactive));
}

void CShortCutItem::RemoveItem(CMenuShortCutKey * ShortCut)
{
    if (ShortCut->UserAdded())
    {
        for (SHORTCUT_KEY_LIST::iterator item = m_AccelList.begin(); item != m_AccelList.end(); item++)
        {
            if (ShortCut == &*item)
            {
                m_AccelList.erase(item);
                break;
            }
        }
    }
    else
    {
        ShortCut->SetInactive(true);
    }
}

bool CShortCutItem::Avaliable(CMenuShortCutKey::RUNNING_STATE RunningState)
{
    switch (RunningState)
    {
    case CMenuShortCutKey::RUNNING_STATE_NOT_RUNNING:
        if (m_Access == CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING ||
            m_Access == CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN ||
            m_Access == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    case CMenuShortCutKey::RUNNING_STATE_WINDOWED:
        if (m_Access == CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW ||
            m_Access == CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN ||
            m_Access == CMenuShortCutKey::ACCESS_GAME_RUNNING ||
            m_Access == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    case CMenuShortCutKey::RUNNING_STATE_FULLSCREEN:
        if (m_Access == CMenuShortCutKey::ACCESS_GAME_RUNNING_FULLSCREEN ||
            m_Access == CMenuShortCutKey::ACCESS_GAME_RUNNING ||
            m_Access == CMenuShortCutKey::ACCESS_ANYTIME)
        {
            return true;
        }
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

CShortCuts::CShortCuts()
{
    Load();
}

CShortCuts::~CShortCuts()
{
}

std::wstring CShortCuts::ShortCutString(int MenuID, CMenuShortCutKey::RUNNING_STATE RunningState)
{
    CGuard CS(m_CS);

    MSC_MAP::iterator MenuItem = m_ShortCuts.find(MenuID);
    if (MenuItem == m_ShortCuts.end()) { return L""; }

    const SHORTCUT_KEY_LIST & ShortCutList = MenuItem->second.GetAccelItems();
    for (SHORTCUT_KEY_LIST::const_iterator item = ShortCutList.begin(); item != ShortCutList.end(); item++)
    {
        if (!item->Active(RunningState) || item->Inactive())
        {
            continue;
        }
        return item->Name().ToUTF16();
    }
    return L"";
}

LanguageStringID CShortCuts::GetMenuItemName(WORD key, bool bCtrl, bool bAlt, bool bShift, RUNNING_STATE RunningState)
{
    CGuard CS(m_CS);

    for (MSC_MAP::iterator Item = m_ShortCuts.begin(); Item != m_ShortCuts.end(); Item++)
    {
        CShortCutItem & short_cut = Item->second;

        for (SHORTCUT_KEY_LIST::const_iterator AccelItem = short_cut.GetAccelItems().begin(); AccelItem != short_cut.GetAccelItems().end(); AccelItem++)
        {
            if (AccelItem->Inactive()) { continue; }
            if (!AccelItem->Match(key, bCtrl, bAlt, bShift, RunningState)) { continue; }
            return short_cut.Title();
        }
    }
    return EMPTY_STRING;
}

void CShortCuts::AddShortCut(WORD ID, LanguageStringID Section, LanguageStringID LangID, CMenuShortCutKey::ACCESS_MODE AccessMode)
{
    m_ShortCuts.insert(MSC_MAP::value_type(ID, CShortCutItem(Section, LangID, AccessMode)));
}

void CShortCuts::Load(bool InitialValues)
{
    CGuard CS(m_CS);

    m_ShortCuts.clear();

    AddShortCut(ID_FILE_OPEN_ROM, STR_SHORTCUT_FILEMENU, MENU_OPEN, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_FILE_ROM_INFO, STR_SHORTCUT_FILEMENU, MENU_ROM_INFO, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_FILE_STARTEMULATION, STR_SHORTCUT_FILEMENU, MENU_START, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_FILE_ENDEMULATION, STR_SHORTCUT_FILEMENU, MENU_END, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_FILE_ROMDIRECTORY, STR_SHORTCUT_FILEMENU, MENU_CHOOSE_ROM, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
    AddShortCut(ID_FILE_REFRESHROMLIST, STR_SHORTCUT_FILEMENU, MENU_REFRESH, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
    AddShortCut(ID_FILE_EXIT, STR_SHORTCUT_FILEMENU, MENU_EXIT, CMenuShortCutKey::ACCESS_ANYTIME);

    AddShortCut(ID_SYSTEM_RESET_SOFT, STR_SHORTCUT_SYSTEMMENU, MENU_RESET_SOFT, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_RESET_HARD, STR_SHORTCUT_SYSTEMMENU, MENU_RESET_HARD, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_PAUSE, STR_SHORTCUT_SYSTEMMENU, MENU_PAUSE, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_BITMAP, STR_SHORTCUT_SYSTEMMENU, MENU_BITMAP, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_LIMITFPS, STR_SHORTCUT_SYSTEMMENU, MENU_LIMIT_FPS, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_SWAPDISK, STR_SHORTCUT_SYSTEMMENU, MENU_SWAPDISK, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
    AddShortCut(ID_SYSTEM_SAVE, STR_SHORTCUT_SYSTEMMENU, MENU_SAVE, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_SAVEAS, STR_SHORTCUT_SYSTEMMENU, MENU_SAVE_AS, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
    AddShortCut(ID_SYSTEM_RESTORE, STR_SHORTCUT_SYSTEMMENU, MENU_RESTORE, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_SYSTEM_LOAD, STR_SHORTCUT_SYSTEMMENU, MENU_LOAD, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
    AddShortCut(ID_SYSTEM_CHEAT, STR_SHORTCUT_SYSTEMMENU, MENU_CHEAT, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_SYSTEM_GSBUTTON, STR_SHORTCUT_SYSTEMMENU, MENU_GS_BUTTON, CMenuShortCutKey::ACCESS_GAME_RUNNING);

    AddShortCut(ID_OPTIONS_DISPLAY_FR, STR_SHORTCUT_OPTIONS, OPTION_DISPLAY_FR, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_OPTIONS_CHANGE_FR, STR_SHORTCUT_OPTIONS, OPTION_CHANGE_FR, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_OPTIONS_INCREASE_SPEED, STR_SHORTCUT_OPTIONS, STR_INSREASE_SPEED, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_OPTIONS_DECREASE_SPEED, STR_SHORTCUT_OPTIONS, STR_DECREASE_SPEED, CMenuShortCutKey::ACCESS_GAME_RUNNING);

    AddShortCut(ID_CURRENT_SAVE_DEFAULT, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_DEFAULT, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_1, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_1, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_2, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_2, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_3, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_3, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_4, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_4, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_5, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_5, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_6, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_6, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_7, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_7, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_8, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_8, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_9, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_9, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_CURRENT_SAVE_10, STR_SHORTCUT_SAVESLOT, SAVE_SLOT_10, CMenuShortCutKey::ACCESS_GAME_RUNNING);

    //Option Menu
    AddShortCut(ID_OPTIONS_FULLSCREEN, STR_SHORTCUT_OPTIONS, MENU_FULL_SCREEN, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_OPTIONS_ALWAYSONTOP, STR_SHORTCUT_OPTIONS, MENU_ON_TOP, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_OPTIONS_CONFIG_GFX, STR_SHORTCUT_OPTIONS, MENU_CONFG_GFX, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_OPTIONS_CONFIG_AUDIO, STR_SHORTCUT_OPTIONS, MENU_CONFG_AUDIO, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_OPTIONS_CONFIG_CONT, STR_SHORTCUT_OPTIONS, MENU_CONFG_CTRL, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_OPTIONS_CONFIG_RSP, STR_SHORTCUT_OPTIONS, MENU_CONFG_RSP, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);
    AddShortCut(ID_OPTIONS_CPU_USAGE, STR_SHORTCUT_OPTIONS, MENU_SHOW_CPU, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    AddShortCut(ID_OPTIONS_SETTINGS, STR_SHORTCUT_OPTIONS, MENU_SETTINGS, CMenuShortCutKey::ACCESS_NOT_IN_FULLSCREEN);

    CPath ShortCutFile = UISettingsLoadStringVal(SupportFile_ShortCuts);
    if (!ShortCutFile.Exists() || InitialValues)
    {
        m_ShortCuts.find(ID_FILE_OPEN_ROM)->second.AddShortCut('O', TRUE, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_FILE_OPEN_ROM)->second.AddShortCut('O', TRUE, false, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_FILE_STARTEMULATION)->second.AddShortCut(VK_F11, false, false, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_FILE_ENDEMULATION)->second.AddShortCut(VK_F12, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_FILE_REFRESHROMLIST)->second.AddShortCut(VK_F5, false, false, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_FILE_EXIT)->second.AddShortCut(VK_F4, false, true, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_FILE_EXIT)->second.AddShortCut(VK_F4, false, true, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_DEFAULT)->second.AddShortCut(0xC0, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_1)->second.AddShortCut('1', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_2)->second.AddShortCut('2', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_3)->second.AddShortCut('3', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_4)->second.AddShortCut('4', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_5)->second.AddShortCut('5', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_6)->second.AddShortCut('6', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_7)->second.AddShortCut('7', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_8)->second.AddShortCut('8', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_9)->second.AddShortCut('9', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_CURRENT_SAVE_10)->second.AddShortCut('0', false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_FULLSCREEN)->second.AddShortCut(VK_RETURN, false, true, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_FULLSCREEN)->second.AddShortCut(VK_ESCAPE, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_ALWAYSONTOP)->second.AddShortCut('A', true, false, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_ALWAYSONTOP)->second.AddShortCut('A', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_OPTIONS_SETTINGS)->second.AddShortCut('T', true, false, false, CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_SETTINGS)->second.AddShortCut('T', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_SYSTEM_RESET_SOFT)->second.AddShortCut(VK_F1, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_RESET_HARD)->second.AddShortCut(VK_F1, false, false, true, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_PAUSE)->second.AddShortCut(VK_F2, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_PAUSE)->second.AddShortCut(VK_PAUSE, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_BITMAP)->second.AddShortCut(VK_F3, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_LIMITFPS)->second.AddShortCut(VK_F4, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_SWAPDISK)->second.AddShortCut('D', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_SYSTEM_SAVE)->second.AddShortCut(VK_F5, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_RESTORE)->second.AddShortCut(VK_F7, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_SYSTEM_LOAD)->second.AddShortCut('L', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_SYSTEM_SAVEAS)->second.AddShortCut('S', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_SYSTEM_CHEAT)->second.AddShortCut('C', true, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING_WINDOW);
        m_ShortCuts.find(ID_SYSTEM_GSBUTTON)->second.AddShortCut(VK_F9, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_INCREASE_SPEED)->second.AddShortCut(VK_OEM_PLUS, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
        m_ShortCuts.find(ID_OPTIONS_DECREASE_SPEED)->second.AddShortCut(VK_OEM_MINUS, false, false, false, CMenuShortCutKey::ACCESS_GAME_RUNNING);
    }
    else
    {
        CMenuShortCutKey::ACCESS_MODE AccessMode;
        int ID, key, bCtrl, bAlt, bShift, bUserAdded, bInactive;

        FILE *file = fopen(ShortCutFile, "r");
        if (file)
        {
            do
            {
                char Line[300];
                if (fgets(Line, sizeof(Line), file) != NULL)
                {
                    sscanf(Line, "%d,%d,%d,%d,%d,%d,%d,%d", &ID, &key, &bCtrl, &bAlt, &bShift, &AccessMode,
                        &bUserAdded, &bInactive);

                    MSC_MAP::iterator item = m_ShortCuts.find(ID);
                    if (item == m_ShortCuts.end()) { continue; }
                    item->second.AddShortCut((WORD)(key & 0xFFFF), bCtrl == 1, bAlt == 1, bShift == 1, AccessMode, bUserAdded == 1, bInactive == 1);
                }
            } while (feof(file) == 0);
            fclose(file);
        }
    }
}

void CShortCuts::Save(void)
{
    CGuard CS(m_CS);

    stdstr FileName = UISettingsLoadStringVal(SupportFile_ShortCuts);
    FILE *file = fopen(FileName.c_str(), "w");
    if (file == NULL)
    {
        return;
    }

    for (MSC_MAP::iterator Item = m_ShortCuts.begin(); Item != m_ShortCuts.end(); Item++)
    {
        for (SHORTCUT_KEY_LIST::const_iterator ShortCut = Item->second.GetAccelItems().begin(); ShortCut != Item->second.GetAccelItems().end(); ShortCut++)
        {
            fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d\n",
                Item->first,
                ShortCut->Key(),
                ShortCut->Ctrl(),
                ShortCut->Alt(),
                ShortCut->Shift(),
                ShortCut->AccessMode(),
                ShortCut->UserAdded(),
                ShortCut->Inactive());
        }
    }
    fclose(file);
}

HACCEL CShortCuts::GetAcceleratorTable(void)
{
    CGuard CS(m_CS);

    //Generate a ACCEL list
    int size = 0, MaxSize = m_ShortCuts.size() * 5;
    ACCEL * AccelList = new ACCEL[MaxSize];
    CMenuShortCutKey::RUNNING_STATE RunningState = CMenuShortCutKey::RunningState();

    for (MSC_MAP::iterator Item = m_ShortCuts.begin(); Item != m_ShortCuts.end(); Item++)
    {
        CShortCutItem & short_cut = Item->second;
        if (!short_cut.Avaliable(RunningState))
        {
            continue;
        }

        SHORTCUT_KEY_LIST ShortCutAccelList = short_cut.GetAccelItems();
        for (SHORTCUT_KEY_LIST::iterator AccelIter = ShortCutAccelList.begin(); AccelIter != ShortCutAccelList.end(); AccelIter++)
        {
            CMenuShortCutKey & Key = *AccelIter;
            if (Key.Inactive())
            {
                continue;
            }
            if (!Key.Active(RunningState))
            {
                continue;
            }
            if (size >= MaxSize) { break; }
            AccelList[size].cmd = (WORD)Item->first;
            AccelList[size].key = Key.Key();
            AccelList[size].fVirt = FVIRTKEY;
            if (Key.Alt()) { AccelList[size].fVirt |= FALT; }
            if (Key.Ctrl()) { AccelList[size].fVirt |= FCONTROL; }
            if (Key.Shift()) { AccelList[size].fVirt |= FSHIFT; }
            size += 1;
        }
    }

    WriteTrace(TraceUserInterface, TraceDebug, "CreateAcceleratorTable");
    HACCEL AccelTable = CreateAcceleratorTable(AccelList, size);
    WriteTrace(TraceUserInterface, TraceDebug, "Delete accel list");
    delete[] AccelList;
    return AccelTable;
}