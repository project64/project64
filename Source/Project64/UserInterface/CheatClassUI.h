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
#include <Project64\WTLApp.h>
#include <Project64-core/N64System/Enhancement/Enhancement.h>
#include <Project64-core/N64System/Enhancement/EnhancementList.h>

class CEditCheat;
class CCheatsUI;

class CCheatList :
    public CDialogImpl<CCheatList>
{
    enum
    {
        UM_CHANGECODEEXTENSION = WM_USER + 0x121,
    };

public:
    BEGIN_MSG_MAP_EX(CCheatList)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(UM_CHANGECODEEXTENSION, OnChangeCodeExtension)
        COMMAND_ID_HANDLER(IDC_UNMARK, OnUnmark)
        COMMAND_ID_HANDLER(ID_POPUP_DELETE, OnPopupDelete)
        NOTIFY_HANDLER_EX(IDC_MYTREE, NM_CLICK, OnTreeClicked)
        NOTIFY_HANDLER_EX(IDC_MYTREE, NM_RCLICK, OnTreeRClicked)
        NOTIFY_HANDLER_EX(IDC_MYTREE, NM_DBLCLK, OnTreeDClicked)
        NOTIFY_HANDLER_EX(IDC_MYTREE, TVN_SELCHANGED, OnTreeSelChanged)
    END_MSG_MAP()

    enum { IDD = IDD_Cheats_List };

    CCheatList(CEnhancementList & Cheats, CEditCheat & EditCheat);
    ~CCheatList();

    void RefreshItems();

private:
    CCheatList(void);
    CCheatList(const CCheatList&);
    CCheatList& operator=(const CCheatList&);

    enum TV_CHECK_STATE { TV_STATE_UNKNOWN, TV_STATE_CLEAR, TV_STATE_CHECKED, TV_STATE_INDETERMINATE };

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnChangeCodeExtension(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUnmark(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPopupDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTreeClicked(NMHDR* lpnmh);
    LRESULT OnTreeRClicked(NMHDR* lpnmh);
    LRESULT OnTreeDClicked(NMHDR* lpnmh);
    LRESULT OnTreeSelChanged(NMHDR* lpnmh);
    void AddCodeLayers(LPARAM ListID, const std::wstring &Name, HTREEITEM hParent, bool CheatActive);
    void ChangeChildrenStatus(HTREEITEM hParent, bool Checked);
    void CheckParentStatus(HTREEITEM hParent);
    void DeleteCheat(LPARAM Enhancement);
    TV_CHECK_STATE TV_GetCheckState(HTREEITEM hItem);
    bool TV_SetCheckState(HTREEITEM hItem, TV_CHECK_STATE state);
    static void MenuSetText(HMENU hMenu, int MenuPos, const wchar_t * Title, const wchar_t * ShortCut);

    enum { IDC_MYTREE = 0x500 };

    CEnhancementList & m_Cheats;
    CEditCheat & m_EditCheat;
    CTreeViewCtrl m_hCheatTree;
    HTREEITEM m_hSelectedItem;
    bool m_DeleteingEntries;
};

class CEditCheat :
    public CDialogImpl<CEditCheat>
{
    friend CCheatsUI;

public:
    enum
    {
        WM_EDITCHEAT = WM_USER + 0x120,
    };

    enum CodeFormat
    {
        CodeFormat_Invalid = -1,
        CodeFormat_Normal = 0,
        CodeFormat_LowerByte = 1,
        CodeFormat_Word = 2,
    };

    BEGIN_MSG_MAP_EX(CEditCheat)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_EDITCHEAT, OnEditCheat)
        COMMAND_ID_HANDLER(IDC_ADD, OnAddCheat)
        COMMAND_ID_HANDLER(IDC_NEWCHEAT, OnNewCheat)
        COMMAND_HANDLER(IDC_CODE_NAME, EN_CHANGE, OnCodeNameChanged)
        COMMAND_HANDLER(IDC_CHEAT_CODES, EN_CHANGE, OnCheatCodeChanged)
        COMMAND_HANDLER(IDC_CHEAT_OPTIONS, EN_CHANGE, OnCheatOptionsChanged)
    END_MSG_MAP()

    enum { IDD = IDD_Cheats_Add };

    CEditCheat(CEnhancementList & Cheats, CCheatList & CheatList);
    ~CEditCheat();

private:
    CEditCheat();
    CEditCheat(const CEditCheat&);
    CEditCheat& operator=(const CEditCheat&);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEditCheat(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAddCheat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNewCheat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCodeNameChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCheatCodeChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCheatOptionsChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    CEnhancement::CodeEntries ReadCodeEntries(bool &ValidCodes, bool &ValidOptions, bool &NoOptions, CodeFormat & Format);
    CEnhancement::CodeOptions ReadOptions(bool &validoptions, CodeFormat Format);

    void RecordCurrentValues(void);
    bool ValuesChanged(void);
    std::string GetItemText(int nIDDlgItem);

    CEnhancementList & m_Cheats;
    CCheatList & m_CheatList;
    std::string m_EditName;
    std::string m_EditCode;
    std::string m_EditOptions;
    std::string m_EditNotes;
    CEnhancement * m_EditEnhancement;
};

class CEnhancementCodeEx :
    public CDialogImpl<CEnhancementCodeEx>
{
public:
    BEGIN_MSG_MAP_EX(CEnhancementCodeEx)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_CHEAT_LIST, LBN_DBLCLK, OnListDblClick)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    enum { IDD = IDD_Cheats_CodeEx };

    CEnhancementCodeEx(CEnhancement * Enhancement);

private:
    CEnhancementCodeEx();
    CEnhancementCodeEx(const CEnhancementCodeEx&);
    CEnhancementCodeEx& operator=(const CEnhancementCodeEx&);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOkCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    CEnhancement * m_Enhancement;
};

class CCheatsUI :
    public CDialogImpl<CCheatsUI>
{
    friend CCheatList;
    friend CEditCheat;
    friend CEnhancementCodeEx;

public:
    BEGIN_MSG_MAP_EX(CCheatsUI)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(IDC_STATE, OnStateChange)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    enum { IDD = IDD_Cheats_Select };

    CCheatsUI(void);
    ~CCheatsUI(void);

    void Display(HWND hParent, bool BlockExecution);

private:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnStateChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    CEnhancementList m_Cheats;
    CEditCheat m_EditCheat;
    CCheatList m_SelectCheat;
    CButton m_StateBtn;
    int  m_MinSizeDlg, m_MaxSizeDlg;
    bool m_bModal;

    enum Dialog_State { CONTRACTED, EXPANDED } m_DialogState;
};