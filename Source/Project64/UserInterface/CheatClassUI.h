/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class CCheatsUI
{
public:
    CCheatsUI(void);
    ~CCheatsUI(void);

    bool IsCheatMessage(MSG * msg);
    void SelectCheats(HWND hParent, bool BlockExecution);

private:
    static int CALLBACK CheatAddProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam);
    static int CALLBACK CheatListProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam);
    static int CALLBACK ManageCheatsProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam);
    static int CALLBACK CheatsCodeExProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam);
    static int CALLBACK CheatsCodeQuantProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam);

    //information about the gui for selecting cheats
    HWND    m_Window, m_hSelectCheat, m_AddCheat, m_hCheatTree, m_hSelectedItem;
    void * const m_rcList, *const m_rcAdd;
    int  m_MinSizeDlg, m_MaxSizeDlg;
    int  m_EditCheat;
    bool m_DeleteingEntries;

    //Information about the current cheat we are editing
    stdstr        m_EditName;
    stdstr        m_EditCode;
    stdstr        m_EditOptions;
    stdstr        m_EditNotes;

    enum Dialog_State   { CONTRACTED, EXPANDED } m_DialogState;
    enum TV_CHECK_STATE { TV_STATE_UNKNOWN, TV_STATE_CLEAR, TV_STATE_CHECKED, TV_STATE_INDETERMINATE };
    enum { IDC_MYTREE = 0x500 };

    void AddCodeLayers(int CheatNumber, const stdstr &CheatName, HWND hParent, bool CheatActive);
    //Reload the cheats from the ini file to the select gui
    void RefreshCheatManager();
    void ChangeChildrenStatus(HWND hParent, bool Checked);
    void CheckParentStatus(HWND hParent);
    static stdstr ReadCodeString(HWND hDlg, bool &validcodes, bool &validoption, bool &nooptions, int &codeformat);
    static stdstr ReadOptionsString(HWND hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat);

    void RecordCheatValues(HWND hDlg);
    bool CheatChanged(HWND hDlg);
    void DeleteCheat(int Index);

    //Get Information about the Cheat
    stdstr GetCheatName(int CheatNo, bool AddExtension) const;
    static bool CheatUsesCodeExtensions(const stdstr &LineEntry);
    //Working with treeview
    static bool  TV_SetCheckState(HWND hwndTreeView, HWND hItem, TV_CHECK_STATE state);
    static int   TV_GetCheckState(HWND hwndTreeView, HWND hItem);

    static void  MenuSetText(HMENU hMenu, int MenuPos, const wchar_t * Title, const wchar_t * ShortCut);

    //UI Functions
    static stdstr GetDlgItemStr(HWND hDlg, int nIDDlgItem);
};

extern CCheatsUI * g_cheatUI;