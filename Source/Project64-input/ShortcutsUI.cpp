#include "ShortcutsUI.h"
#include "CProject64Input.h"
#include "wtl.h"
#include "wtl-ScanButton.h"
#include "resource.h"
#include <Common/StdString.h>

class CShortcutsDlg :
    public CDialogImpl<CShortcutsDlg>
{
public:
    enum { IDD = IDD_Shortcuts };

    BEGIN_MSG_MAP(CShortcutsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_SUCCESS, OnScanSuccess)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_CANCELED, OnScanCanceled)
    END_MSG_MAP()

    CShortcutsDlg(SHORTCUTS & Shortcuts) :
        m_Shortcuts(Shortcuts),
        m_SetupIndex(-1),
        m_ButtonLockMouse(m_Shortcuts.LOCKMOUSE, IDC_EDIT_LOCKMOUSE, IDC_BTN_LOCKMOUSE)
    {
    }

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        SetWindowText(stdstr_f("Shortcuts").ToUTF16().c_str());

        CScanButton* Buttons[] = {
            &m_ButtonLockMouse
        };

        for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
        {
            Buttons[i]->SubclassWindow(m_hWnd);
            Buttons[i]->SetChangeCallback(stButtonChanged, (size_t)this);
        }

        return TRUE;
    }

    LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        SHORTCUTS& Shortcuts = g_InputPlugin->Shortcuts();
        Shortcuts = m_Shortcuts;
        g_InputPlugin->SaveShortcuts();
        EndDialog(wID);
        return 0;
    }
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }

private:
    SHORTCUTS& m_Shortcuts;
    CScanButton m_ButtonLockMouse;
    int32_t m_SetupIndex;

    static void stButtonChanged(size_t data, const BUTTON& Button) { ((CShortcutsDlg*)data)->ButtonChannged(Button); }

    void ButtonChannged(const BUTTON& Button)
    {
        BUTTON EmptyButton = { 0 };
        BUTTON* buttons[] =
        {
            &m_Shortcuts.LOCKMOUSE,
        };

        bool Changed = false;
        for (size_t b = 0; b < (sizeof(buttons) / sizeof(buttons[0])); b++)
        {
            if (buttons[b]->Offset == Button.Offset &&
                buttons[b]->AxisID == Button.AxisID &&
                buttons[b]->BtnType == Button.BtnType &&
                memcmp(&buttons[b]->DeviceGuid, &Button.DeviceGuid, sizeof(Button.DeviceGuid)) == 0)
            {
                *buttons[b] = EmptyButton;
                Changed = true;
            }
        }

        if (Changed)
        {
            CScanButton* ScanButtons[] = {
                &m_ButtonLockMouse,
            };

            for (size_t i = 0, n = sizeof(ScanButtons) / sizeof(ScanButtons[0]); i < n; i++)
            {
                ScanButtons[i]->DisplayButton();
            }
        }
        CPropertySheetWindow(GetParent()).SetModified(m_hWnd);
    }

    LRESULT OnScanSuccess(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (m_SetupIndex < 0)
        {
            return 0;
        }

        CScanButton* Buttons[] = {
            &m_ButtonLockMouse
        };

        m_SetupIndex += 1;
        if (m_SetupIndex < (sizeof(Buttons) / sizeof(Buttons[0])))
        {
            Buttons[m_SetupIndex]->DetectKey();
        }
        return 0;
    }

    LRESULT OnScanCanceled(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        m_SetupIndex = -1;
        return 0;
    }
};

void ConfigShortcut(SHORTCUTS & Shortcuts)
{
    CShortcutsDlg(Shortcuts).DoModal();
}
