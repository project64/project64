#include "OptionsUI.h"
#include "wtl.h"
#include "resource.h"
#include <Common/StdString.h>

class COptionsDlg :
    public CDialogImpl<COptionsDlg>
{
public:
    enum { IDD = IDD_Options };

    BEGIN_MSG_MAP(COptionsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    COptionsDlg(uint32_t ControlIndex, CONTROL & ControlInfo, N64CONTROLLER & Controller) :
        m_ControlIndex(ControlIndex),
        m_ControlInfo(ControlInfo),
        m_Controller(Controller)
    {
    }

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        SetWindowText(stdstr_f("Options - Player %d", m_ControlIndex + 1).ToUTF16().c_str());
        CButton(GetDlgItem(IDC_REAL_N64_RANGE)).SetCheck(m_Controller.RealN64Range ? BST_CHECKED : BST_UNCHECKED);
        CButton(GetDlgItem(IDC_REMOVE_DUPLICATE)).SetCheck(m_Controller.RemoveDuplicate ? BST_CHECKED : BST_UNCHECKED);

        m_Sensitivity.Attach(GetDlgItem(IDC_SLIDE_SENSITIVITY));
        m_Sensitivity.SetTicFreq(1);
        m_Sensitivity.SetRangeMin(1);
        m_Sensitivity.SetRangeMax(100);
        m_Sensitivity.SetPos(m_Controller.Sensitivity);
        CWindow(GetDlgItem(IDC_TXT_SENSITIVITY)).SetWindowText(stdstr_f("Mouse Sensitivity: %d%%", m_Sensitivity.GetPos()).ToUTF16().c_str());

        CComboBox ControllerPak(GetDlgItem(IDC_PAKTYPE));
        int Index = ControllerPak.AddString(L"None");
        ControllerPak.SetItemData(Index, PLUGIN_NONE);
        if (m_ControlInfo.Plugin == PLUGIN_NONE)
        {
            ControllerPak.SetCurSel(Index);
        }
        Index = ControllerPak.AddString(L"Mem Pak");
        ControllerPak.SetItemData(Index, PLUGIN_MEMPAK);
        if (m_ControlInfo.Plugin == PLUGIN_MEMPAK)
        {
            ControllerPak.SetCurSel(Index);
        }
        Index = ControllerPak.AddString(L"Rumble Pak");
        ControllerPak.SetItemData(Index, PLUGIN_RUMBLE_PAK);
        if (m_ControlInfo.Plugin == PLUGIN_RUMBLE_PAK)
        {
            ControllerPak.SetCurSel(Index);
        }
        return TRUE;
    }

    LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        bool bChanged = false;
        bool RealN64Range = CButton(GetDlgItem(IDC_REAL_N64_RANGE)).GetCheck() == BST_CHECKED;
        if (RealN64Range != m_Controller.RealN64Range)
        {
            m_Controller.RealN64Range = RealN64Range;
            bChanged = true;
        }
        bool RemoveDuplicate = CButton(GetDlgItem(IDC_REMOVE_DUPLICATE)).GetCheck() == BST_CHECKED;
        if (RemoveDuplicate != m_Controller.RemoveDuplicate)
        {
            m_Controller.RemoveDuplicate = RemoveDuplicate;
            bChanged = true;
        }

        uint8_t Sensitivity = (uint8_t)m_Sensitivity.GetPos();
        if (Sensitivity != m_Controller.Sensitivity)
        {
            m_Controller.Sensitivity = Sensitivity;
            bChanged = true;
        }

        CComboBox ControllerPak(GetDlgItem(IDC_PAKTYPE));
        DWORD_PTR PakItemData = ControllerPak.GetItemData(ControllerPak.GetCurSel());
        int32_t Pak = (int32_t)(PakItemData & 0xFFFFFFFF);
        if (Pak != m_ControlInfo.Plugin)
        {
            m_ControlInfo.Plugin = Pak;
            bChanged = true;
        }

        if (bChanged)
        {
            GetParent().SendMessage(PSM_CHANGED);
        }
        EndDialog(wID);
        return 0;
    }
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }
    LRESULT OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        LONG SliderId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
        if (SliderId == IDC_SLIDE_SENSITIVITY)
        {
            CWindow(GetDlgItem(IDC_TXT_SENSITIVITY)).SetWindowText(stdstr_f("Mouse Sensitivity: %d%%", m_Sensitivity.GetPos()).ToUTF16().c_str());
        }
        return 0;
    }
    uint32_t m_ControlIndex;
    CONTROL & m_ControlInfo;
    N64CONTROLLER & m_Controller;
    CTrackBarCtrl m_Sensitivity;
};

void ConfigOption(uint32_t ControlIndex, CONTROL & ControlInfo, N64CONTROLLER & Controller)
{
    COptionsDlg(ControlIndex, ControlInfo, Controller).DoModal();
}
