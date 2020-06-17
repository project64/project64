#include "InputConfigUI.h"
#include "wtl.h"
#include "wtl-BitmapPicture.h"
#include <Common\stdtypes.h>
#include <Common\StdString.h>
#include "resource.h"

class CControllerSettings :
    public CPropertyPageImpl<CControllerSettings>
{
public:
    enum { IDD = IDD_Controller };

    BEGIN_MSG_MAP(CDebugSettings)
        MSG_WM_INITDIALOG(OnInitDialog)
        CHAIN_MSG_MAP(CPropertyPageImpl<CControllerSettings>)
    END_MSG_MAP()

    CControllerSettings();
    void SetControllerNumber(uint32_t ControllerNumber);
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);

private:
    std::wstring m_Title;
    uint32_t m_ControllerNumber;
    CBitmapPicture m_ControllerImg;
};

CControllerSettings::CControllerSettings() :
    m_ControllerNumber((uint32_t)-1)
{
}

void CControllerSettings::SetControllerNumber(uint32_t ControllerNumber)
{
    m_Title = stdstr_f("Player %d", ControllerNumber + 1).ToUTF16();
    SetTitle(m_Title.c_str());
    m_ControllerNumber = ControllerNumber;
}

BOOL CControllerSettings::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_ControllerImg.SubclassWindow(GetDlgItem(IDC_BMP_CONTROLLER));
    m_ControllerImg.SetBitmap(MAKEINTRESOURCE(IDB_CONTROLLER));
    return TRUE;
}

class CInputConfigUI: 
    public CPropertySheetImpl<CInputConfigUI>
{
public:
    CInputConfigUI();
    ~CInputConfigUI();

    void OnSheetInitialized();

private:
    CControllerSettings m_pgController[4];
};

void ConfigInput(void * hParent)
{
    CInputConfigUI().DoModal((HWND)hParent);
}

CInputConfigUI::CInputConfigUI() 
{
    m_psh.pszCaption = L"Configure Input";
    for (size_t i = 0, n = sizeof(m_pgController) / sizeof(m_pgController[0]); i < n; i++)
    {
        m_pgController[i].SetControllerNumber(i);
        AddPage(&m_pgController[i].m_psp);
    }
}

CInputConfigUI::~CInputConfigUI()
{
}

void CInputConfigUI::OnSheetInitialized()
{
    ModifyStyleEx(WS_EX_CONTEXTHELP,0);
}
