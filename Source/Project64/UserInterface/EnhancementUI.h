#pragma once
#include <Project64\WTLApp.h>

class CEditEnhancement;

class CEnhancementUI :
    public CDialogImpl<CEnhancementUI>
{
    enum TV_CHECK_STATE
    {
        TV_STATE_UNKNOWN,
        TV_STATE_CLEAR,
        TV_STATE_CHECKED,
        TV_STATE_INDETERMINATE
    };

public:
    BEGIN_MSG_MAP_EX(CEnhancementUI)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        COMMAND_ID_HANDLER(ID_POPUP_EDIT, OnEditEnhancement)
        COMMAND_ID_HANDLER(ID_POPUP_ADDENHANCEMENT, OnAddEnhancement)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_CLICK, OnEnhancementListClicked)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_RCLICK, OnEnhancementListRClicked)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_DBLCLK, OnEnhancementListDClicked)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, TVN_SELCHANGED, OnEnhancementListSelChanged)
    END_MSG_MAP()

    enum { IDD = IDD_Enhancement_Config };

    CEnhancementUI();

    void Display(HWND hParent, bool BlockExecution);

private:
    friend CEditEnhancement;

    CEnhancementUI(const CEnhancementUI&);
    CEnhancementUI& operator=(const CEnhancementUI&);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnEditEnhancement(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    LRESULT OnAddEnhancement(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled );
    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    LRESULT OnEnhancementListClicked(NMHDR * lpnmh);
    LRESULT OnEnhancementListRClicked(NMHDR * lpnmh);
    LRESULT OnEnhancementListDClicked(NMHDR * lpnmh);
    LRESULT OnEnhancementListSelChanged(NMHDR * lpnmh);

    void AddCodeLayers(LPARAM ListID, const std::wstring & Name, HTREEITEM hParent, bool Active);
    void ChangeChildrenStatus(HTREEITEM hParent, bool Checked);
    void CheckParentStatus(HTREEITEM hParent);
    void RefreshList(void);
    TV_CHECK_STATE TV_GetCheckState(HTREEITEM hItem);
    bool TV_SetCheckState(HTREEITEM hItem, TV_CHECK_STATE state);

    CEnhancementList m_Enhancements;
    CTreeViewCtrl m_TreeList;
    HTREEITEM m_hSelectedItem;
    bool m_bModal;
};
