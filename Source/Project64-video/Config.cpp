/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#include "Gfx_1.3.h"
#include "DepthBufferRender.h"
#include "Config.h"
#include "trace.h"
#include "ScreenResolution.h"
#include <Common/StdString.h>
#include <Settings/Settings.h>
#include "SettingsID.h"
#include "ScreenResolution.h"

#ifdef _WIN32
#include <Common/CriticalSection.h>
#include "resource.h"

#pragma warning(push)
#pragma warning(disable : 4091) // warning C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#pragma warning(disable : 4302) // warning C4302: 'type cast': truncation from 'LPCTSTR' to 'WORD'
#pragma warning(disable : 4458) // warning C4458: declaration of 'dwCommonButtons' hides class member
#pragma warning(disable : 4838) // warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable : 4996) // warning C4996: 'GetVersionExA': was declared deprecated
#pragma warning(disable : 4302) // warning C4302: 'type cast': truncation from 'LPCTSTR' to 'WORD'
#define _ATL_DISABLE_NOTHROW_NEW
#include <atlbase.h>
#include <wtl/atlapp.h>

#include <atlwin.h>
#include <wtl/atldlgs.h>
#include <wtl/atlctrls.h>
#include <wtl/atlcrack.h>
#pragma warning(pop)

extern HINSTANCE hinstDLL;
extern bool g_ghq_use;

extern CriticalSection * g_ProcessDListCS;

class CProject64VideoWtlModule :
    public CAppModule
{
public:
    CProject64VideoWtlModule(HINSTANCE hinst)
    {
        Init(NULL, hinst);
    }
    virtual ~CProject64VideoWtlModule(void)
    {
        Term();
    }
};

CProject64VideoWtlModule * WtlModule = NULL;

void ConfigInit(void * hinst)
{
    WtlModule = new CProject64VideoWtlModule((HINSTANCE)hinst);
}

void ConfigCleanup(void)
{
    if (WtlModule)
    {
        delete WtlModule;
        WtlModule = NULL;
    }
}

#endif

void CloseConfig();

#ifdef _WIN32

template < class T, class TT = CToolTipCtrl >
class CToolTipDialog
{
    // Data declarations and members
public:
    TT& GetTT() { return m_TT; }
protected:
    TT m_TT;
    UINT m_uTTStyle;
    UINT m_uToolFlags;
    // Construction
    CToolTipDialog(UINT uTTSTyle = TTS_NOPREFIX | TTS_BALLOON, UINT uToolFlags = TTF_IDISHWND | TTF_SUBCLASS)
        : m_TT(NULL), m_uTTStyle(uTTSTyle),
        m_uToolFlags(uToolFlags | TTF_SUBCLASS)
    {}

    void TTInit()
    {
        T* pT = (T*)this;
        ATLASSERT(::IsWindow(*pT));
        m_TT.Create(*pT, NULL, NULL, m_uTTStyle);
        CToolInfo ToolInfo(pT->m_uToolFlags, *pT, 0, 0, MAKEINTRESOURCE(pT->IDD));
        m_TT.AddTool(&ToolInfo);
        ::EnumChildWindows(*pT, SetTool, (LPARAM)pT);
        TTSize(0);
        TTActivate(TRUE);
    }
    // Operations
public:
    void TTActivate(BOOL bActivate)
    {
        m_TT.Activate(bActivate);
    }
    void TTSize(int nPixel)
    {
        m_TT.SetMaxTipWidth(nPixel);
    }

    void TTSetTxt(HWND hTool, _U_STRINGorID text)
    {
        m_TT.UpdateTipText(text, hTool);
    }
    void TTSetTxt(UINT idTool, _U_STRINGorID text)
    {
        TTSetTxt(GetHWND(idTool), text);
    }

    BOOL TTAdd(HWND hTool)
    {
        return SetTool(hTool, (LPARAM)(T*)this);
    }
    BOOL TTAdd(UINT idTool)
    {
        return TTAdd(GetHWND(idTool));
    }

    void TTRemove(HWND hTool)
    {
        m_TT.DelTool(hTool);
    }
    void TTRemove(UINT idTool)
    {
        TTRemove(GetHWND(idTool));
    }
    // Message map and handlers
    BEGIN_MSG_MAP(CToolTipDialog)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouse)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL& bHandled)
    {
        TTInit();
        bHandled = FALSE;
        return TRUE;
    }

    LRESULT OnMouse(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        T* pT = (T*)this;
        bHandled = FALSE;
        if (m_TT.IsWindow())
            m_TT.RelayEvent((LPMSG)pT->GetCurrentMessage());
        return 0;
    }
    // Implementation
private:
    HWND GetHWND(UINT idTool)
    {
        return ::GetDlgItem(*(T*)this, idTool);
    }

    static BOOL CALLBACK SetTool(HWND hTool, LPARAM pDlg)
    {
        T* pT = (T*)pDlg;
        int idTool = ::GetWindowLong(hTool, GWL_ID);
        if (idTool != IDC_STATIC)
        {
            CToolInfo ToolInfo(pT->m_uToolFlags, hTool, 0, 0, (LPTSTR)idTool);
            pT->m_TT.AddTool(&ToolInfo);
        }
        return TRUE;
    }
};

void SetComboBoxIndex(CComboBox & cmb, uint32_t data)
{
    cmb.SetCurSel(0);
    for (DWORD i = 0, n = cmb.GetCount(); i < n; i++)
    {
        if (cmb.GetItemData(i) == data)
        {
            cmb.SetCurSel(i);
            break;
        }
    }
}

class CConfigBasicPage;
class CConfigEmuSettings;
class CDebugSettings;
class CConfigTextureEnhancement;

class COptionsSheet : public CPropertySheetImpl < COptionsSheet >
{
public:
    // Construction
    COptionsSheet(_U_STRINGorID /*title*/ = (LPCTSTR)NULL, UINT /*uStartPage*/ = 0, HWND /*hWndParent*/ = NULL);
    ~COptionsSheet();

    void UpdateTextureSettings(void);

    // Maps
    BEGIN_MSG_MAP(COptionsSheet)
        CHAIN_MSG_MAP(CPropertySheetImpl<COptionsSheet>)
    END_MSG_MAP()

private:
    // Property pages
    CConfigBasicPage * m_pgBasicPage;
    CConfigEmuSettings * m_pgEmuSettings;
    CDebugSettings * m_pgDebugSettings;
    CConfigTextureEnhancement * m_pgTextureEnhancement;
    HPROPSHEETPAGE m_hTextureEnhancement;
};

class CConfigBasicPage :
    public CPropertyPageImpl<CConfigBasicPage>,
    public CToolTipDialog < CConfigBasicPage >
{
public:
    enum { IDD = IDD_CONFIG_BASIC };

    CConfigBasicPage(COptionsSheet * options_page) :
        m_options_page(options_page)
    {
    }

    BEGIN_MSG_MAP(CConfigBasicPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER_EX(IDC_CMB_WINDOW_RES, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_FS_RESOLUTION, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_VERTICAL_SYNC, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CBXANISOTROPIC, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_SHOW_TEXTURE_ENHANCEMENT, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_AUTODETECT_VRAM, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_USE_FRAME_BUFFER_OBJECT, BN_CLICKED, ItemChanged)
        CHAIN_MSG_MAP(CToolTipDialog<CConfigBasicPage>)
        CHAIN_MSG_MAP(CPropertyPageImpl<CConfigBasicPage>)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        TTInit();
        TTSize(400);
        m_WindowRes.Attach(GetDlgItem(IDC_CMB_WINDOW_RES));
        for (uint32_t i = 0, n = GetScreenResolutionCount(); i < n; i++)
        {
            m_WindowRes.SetItemData(m_WindowRes.AddString(GetScreenResolutionName(i)), i);
        }
        SetComboBoxIndex(m_WindowRes, g_settings->ScreenRes());
        TTSetTxt(IDC_CMB_WINDOW_RES, "Resolution:\n\nThis option selects the windowed resolution.\n\n[Recommended: 640x480, 800x600, 1024x768]");

        m_cbxVSync.Attach(GetDlgItem(IDC_CHK_VERTICAL_SYNC));
        m_cbxVSync.SetCheck(g_settings->vsync() ? BST_CHECKED : BST_UNCHECKED);
        TTSetTxt(IDC_CHK_VERTICAL_SYNC, "Vertical sync:\n\nThis option will enable the vertical sync, which will prevent tearing.\nNote: this option will ONLY have effect if vsync is set to \"Software Controlled\".");

        m_cbxTextureSettings.Attach(GetDlgItem(IDC_CHK_SHOW_TEXTURE_ENHANCEMENT));
        m_cbxTextureSettings.SetCheck(g_settings->texenh_options() ? BST_CHECKED : BST_UNCHECKED);

        m_cmbFSResolution.Attach(GetDlgItem(IDC_CMB_FS_RESOLUTION));
        int32_t size = 0;
        const char ** aRes = getFullScreenResList(&size);
        if (aRes && size)
        {
            for (int r = 0; r < size; r++)
            {
                m_cmbFSResolution.AddString(aRes[r]);
            }
            m_cmbFSResolution.SetCurSel(g_settings->FullScreenRes() < size ? g_settings->FullScreenRes() : 0);
        }
        TTSetTxt(IDC_CMB_FS_RESOLUTION, "Full screen resolution:\n\nThis sets the full screen resolution.\nAll the resolutions that your video card / monitor support should be displayed.\n\n[Recommended:native(max) resolution of your monitor - unless performance becomes an issue]");

        m_cbxAnisotropic.Attach(GetDlgItem(IDC_CBXANISOTROPIC));
        m_cbxAnisotropic.SetCheck(g_settings->wrpAnisotropic() ? BST_CHECKED : BST_UNCHECKED);
        TTSetTxt(IDC_CBXANISOTROPIC, "Anisotropic filtering:\n\nThis filter sharpens and brings out the details of textures that recede into the distance.\nWhen activated, it will use the max anisotropy your video card supports.\nHowever, this will override native way of texture filtering and may cause visual artifacts in some games.\n\n[Recommended: your preference, game dependant]");

        m_cbxFBO.Attach(GetDlgItem(IDC_CHK_USE_FRAME_BUFFER_OBJECT));
        TTSetTxt(IDC_CHK_USE_FRAME_BUFFER_OBJECT, "Use frame buffer objects:\n\nChanges the way FB effects are rendered - with or without usage of the OpenGL Frame Buffer Objects (FBO) extension.\nThe choice depends on game and your video card. FBO off is good for NVIDIA cards, while for ATI cards, it's usually best that FBOs are turned on.\nAlso, some FB effects works only with one of the methods, no matter, which card you have.\nOn the whole, with FBO off, compatibility/ accuracy is a bit better (which is the case for Resident Evil 2).\nHowever, with FBO on with some systems, it can actually be a bit faster in cases.\n\n[Recommended: video card and game dependant]");
        m_cbxFBO.SetCheck(g_settings->wrpFBO() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxVRAM.Attach(GetDlgItem(IDC_CHK_AUTODETECT_VRAM));
        TTSetTxt(IDC_CHK_AUTODETECT_VRAM, "Autodetect VRAM Size:\n\nSince OpenGL cannot do this reliably at the moment, the option to set this manually is available.\nIf checked, plugin will try to autodetect VRAM size.\nBut if this appears wrong, please uncheck and set it to correct value.\n\n[Recommended: on]");
        m_VramSize.Attach(GetDlgItem(IDC_SPIN_VRAM_SIZE));
        m_VramSize.SetBuddy(GetDlgItem(IDC_TXT_VRAM_SIZE));
        m_spinVRAM.Attach(GetDlgItem(IDC_TXT_VRAM_SIZE));
        m_cbxVRAM.SetCheck(g_settings->wrpVRAM() == 0 ? BST_CHECKED : BST_UNCHECKED);
        m_lblMb.Attach(GetDlgItem(IDC_LBL_MB));
        AutoDetectChanged();
        return TRUE;
    }

    bool OnApply()
    {
        char spinVRAM[100];
        m_spinVRAM.GetWindowText(spinVRAM, sizeof(spinVRAM));
        g_settings->SetScreenRes(m_WindowRes.GetCurSel());
        g_settings->SetVsync(m_cbxVSync.GetCheck() == BST_CHECKED);
        g_settings->SetTexenhOptions(m_cbxTextureSettings.GetCheck() == BST_CHECKED);
        g_settings->SetFullScreenRes(m_cmbFSResolution.GetCurSel());
        g_settings->SetWrpAnisotropic(m_cbxAnisotropic.GetCheck() == BST_CHECKED);
        g_settings->SetWrpVRAM(m_cbxVRAM.GetCheck() == BST_CHECKED ? 0 : atoi(spinVRAM));
        g_settings->SetWrpFBO(m_cbxFBO.GetCheck() == BST_CHECKED);

        if (g_settings->dirty())
        {
            g_settings->WriteSettings();
        }
        m_options_page->UpdateTextureSettings();
        return true;
    }
private:
    void ItemChanged(UINT /*Code*/, int id, HWND /*ctl*/)
    {
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
        if (id == IDC_CHK_AUTODETECT_VRAM)
        {
            AutoDetectChanged();
        }
    }

    void AutoDetectChanged(void)
    {
        m_spinVRAM.SetWindowText(m_cbxVRAM.GetCheck() == BST_CHECKED ? " auto" : stdstr_f("%d", g_settings->wrpVRAM() != 0 ? g_settings->wrpVRAM() : 32).c_str());
        m_spinVRAM.EnableWindow(m_cbxVRAM.GetCheck() != BST_CHECKED);
        m_VramSize.EnableWindow(m_cbxVRAM.GetCheck() != BST_CHECKED);
        m_lblMb.EnableWindow(m_cbxVRAM.GetCheck() != BST_CHECKED);
    }

    COptionsSheet * m_options_page;
    CComboBox m_WindowRes, m_cmbFSResolution;
    CButton m_cbxVSync;
    CButton m_cbxTextureSettings;
    CButton m_cbxAnisotropic;
    CButton m_cbxFBO;
    CButton m_cbxVRAM;
    CUpDownCtrl m_VramSize;
    CEdit m_spinVRAM;
    CStatic m_lblMb;
};

class CConfigEmuSettings :
    public CPropertyPageImpl<CConfigEmuSettings>,
    public CToolTipDialog < CConfigEmuSettings >
{
public:
    enum { IDD = IDD_EMULATION_SETTINGS };

    BEGIN_MSG_MAP(CConfigEmuSettings)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER_EX(IDC_CMB_FILTERING_MODE, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_BUFFER_SWAPPING, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_LOD_CALC, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_ASPECT_RATIO, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_FOG, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_BUFFER_CLEAR, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_FRAME_BUFFER_EMULATION, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_HARDWARE_FRAMEBUFFER, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_GET_FRAMEBUFFER, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_READ_EVERY_FRAME, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_RENDER_FRAME_AS_TEXTURE, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CHK_DETECT_CPU_WRITE, BN_CLICKED, ItemChanged)
        COMMAND_HANDLER_EX(IDC_SOFTWARE_DEPTH_BUFFER, BN_CLICKED, ItemChanged)
        CHAIN_MSG_MAP(CToolTipDialog<CConfigEmuSettings>)
        CHAIN_MSG_MAP(CPropertyPageImpl<CConfigEmuSettings>)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        TTInit();
        TTSize(400);

        if (g_romopen)
        {
            ::SetWindowText(GetDlgItem(IDC_INFO), "Current game emulation settings. Change with care!");
        }
        else
        {
            ::SetWindowText(GetDlgItem(IDC_INFO), "Default emulation settings. Not recommended to change!");
        }

        std::string tooltip = "Filtering mode:\n\nThere are three filtering modes possible:\n\n* Automatic filtering - filter exactly how the N64 specifies.\n* Point-sampled filtering - causes texels to appear square and sharp.\n* Bilinear filtering - interpolates the texture to make it appear more smooth.\n\n[Recommended: Automatic]";
        TTSetTxt(IDC_TXT_FILTERING_MODE, tooltip.c_str());
        TTSetTxt(IDC_CMB_FILTERING_MODE, tooltip.c_str());

        m_cmbFiltering.Attach(GetDlgItem(IDC_CMB_FILTERING_MODE));
        m_cmbFiltering.SetItemData(m_cmbFiltering.AddString("Automatic"), CSettings::Filter_Automatic);
        m_cmbFiltering.SetItemData(m_cmbFiltering.AddString("Force Bilinear"), CSettings::Filter_ForceBilinear);
        m_cmbFiltering.SetItemData(m_cmbFiltering.AddString("Force Point-sampled"), CSettings::Filter_ForcePointSampled);
        SetComboBoxIndex(m_cmbFiltering, (uint32_t)g_settings->filtering());

        tooltip = "Buffer swapping method:\n\nThere are 3 buffer swapping methods:\n\n* old - swap buffers when vertical interrupt has occurred.\n* new - swap buffers when set of conditions is satisfied. Prevents flicker on some games.\n* hybrid - mix of first two methods.  Can prevent even more flickering then previous method, but also can cause artefacts.\nIf you have flickering problems in a game (or graphics that don't show), try to change swapping method.\n\n[Recommended: new (hybrid for Paper Mario)]";
        TTSetTxt(IDC_TXT_BUFFER_SWAPPING, tooltip.c_str());
        TTSetTxt(IDC_CMB_BUFFER_SWAPPING, tooltip.c_str());
        m_cmbBufferSwap.Attach(GetDlgItem(IDC_CMB_BUFFER_SWAPPING));
        m_cmbBufferSwap.SetItemData(m_cmbBufferSwap.AddString("Old"), CSettings::SwapMode_Old);
        m_cmbBufferSwap.SetItemData(m_cmbBufferSwap.AddString("New"), CSettings::SwapMode_New);
        m_cmbBufferSwap.SetItemData(m_cmbBufferSwap.AddString("Hybrid"), CSettings::SwapMode_Hybrid);
        SetComboBoxIndex(m_cmbBufferSwap, g_settings->swapmode());

        tooltip = "Per-pixel level-of-detail calculation:\n\nN64 uses special mechanism for mip-mapping, which nearly impossible to reproduce correctly on PC hardware.\nThis option enables approximate emulation of this feature.\nFor example, it is required for the Peach/Bowser portrait's transition in Super Mario 64.\nThere are 3 modes:\n\n* off - LOD is not calculated\n* fast - fast imprecise LOD calculation.\n* precise - most precise LOD calculation possible, but more slow.\n\n[Recommended: your preference]";
        TTSetTxt(IDC_TXT_LOD_CALC, tooltip.c_str());
        TTSetTxt(IDC_CMB_LOD_CALC, tooltip.c_str());
        m_cmbLOD.Attach(GetDlgItem(IDC_CMB_LOD_CALC));
        m_cmbLOD.SetItemData(m_cmbLOD.AddString("off"), CSettings::LOD_Off);
        m_cmbLOD.SetItemData(m_cmbLOD.AddString("fast"), CSettings::LOD_Fast);
        m_cmbLOD.SetItemData(m_cmbLOD.AddString("precise"), CSettings::LOD_Precise);
        SetComboBoxIndex(m_cmbLOD, g_settings->lodmode());

        tooltip = "Aspect ratio of the output:\n\nMost N64 games use 4:3 aspect ratio, but some support widescreen too.\nYou may select appropriate aspect here and set widescreen mode in game settings->\nIn \"Stretch\" mode the output will be stretched to the entire screen, other modes may add black borders if necessary";
        TTSetTxt(IDC_TXT_ASPECT_RATIO, tooltip.c_str());
        TTSetTxt(IDC_CMB_ASPECT_RATIO, tooltip.c_str());

        m_cmbAspect.Attach(GetDlgItem(IDC_CMB_ASPECT_RATIO));
        m_cmbAspect.SetItemData(m_cmbAspect.AddString("4:3 (default)"), CSettings::Aspect_4x3);
        m_cmbAspect.SetItemData(m_cmbAspect.AddString("Force 16:9"), CSettings::Aspect_16x9);
        m_cmbAspect.SetItemData(m_cmbAspect.AddString("Stretch"), CSettings::Aspect_Stretch);
        m_cmbAspect.SetItemData(m_cmbAspect.AddString("Original"), CSettings::Aspect_Original);
        SetComboBoxIndex(m_cmbAspect, (uint32_t)g_settings->aspectmode());

        tooltip = "Fog enabled:\n\nSets fog emulation on//off.\n\n[Recommended: on]";
        TTSetTxt(IDC_CHK_FOG, tooltip.c_str());
        m_cbxFog.Attach(GetDlgItem(IDC_CHK_FOG));
        m_cbxFog.SetCheck(g_settings->fog() ? BST_CHECKED : BST_UNCHECKED);

        tooltip = "Buffer clear on every frame:\n\nForces the frame buffer to be cleared every frame drawn.\nUsually frame buffer clear is controlled by the game.\nHowever, in some cases it is not well emulated, and some garbage may be left on the screen.\nIn such cases, this option must be set on.\n\n[Recommended: on]";
        TTSetTxt(IDC_CHK_BUFFER_CLEAR, tooltip.c_str());
        m_cbxBuffer.Attach(GetDlgItem(IDC_CHK_BUFFER_CLEAR));
        m_cbxBuffer.SetCheck(g_settings->buff_clear() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBEnable.Attach(GetDlgItem(IDC_CHK_FRAME_BUFFER_EMULATION));
        TTSetTxt(IDC_CHK_FRAME_BUFFER_EMULATION, "Enable frame buffer emulation:\n\nIf on, plugin will try to detect frame buffer usage and apply appropriate frame buffer emulation.\n\n[Recommended: on for games which use frame buffer effects]");
        m_cbxFBEnable.SetCheck(g_settings->fb_emulation_enabled() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBHWFBE.Attach(GetDlgItem(IDC_CHK_HARDWARE_FRAMEBUFFER));
        TTSetTxt(IDC_CHK_HARDWARE_FRAMEBUFFER, "Enable hardware frame buffer emulation:\n\nIf this option is on, plugin will create auxiliary frame buffers in video memory instead of copying frame buffer content into main memory.\nThis allows plugin to run frame buffer effects without slowdown and without scaling image down to N64's native resolution.\nModern cards also fully support it.\n\n[Recommended: on, if supported by your hardware]");
        m_cbxFBHWFBE.SetCheck(g_settings->fb_hwfbe_set() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBGetFBI.Attach(GetDlgItem(IDC_CHK_GET_FRAMEBUFFER));
        TTSetTxt(IDC_CHK_GET_FRAMEBUFFER, "Get information about frame buffers:\n\nThis is compatibility option. It must be set on for Mupen64 and off for 1964");
        m_cbxFBGetFBI.SetCheck(g_settings->fb_get_info_enabled() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBReadEveryFrame.Attach(GetDlgItem(IDC_CHK_READ_EVERY_FRAME));
        TTSetTxt(IDC_CHK_READ_EVERY_FRAME, "Read every frame:\n\nIn some games plugin can't detect frame buffer usage.\nIn such cases you need to enable this option to see frame buffer effects.\nEvery drawn frame will be read from video card -> it works very slow.\n\n[Recommended: mostly off (needed only for a few games)]");
        m_cbxFBReadEveryFrame.SetCheck(g_settings->fb_ref_enabled() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBasTex.Attach(GetDlgItem(IDC_RENDER_FRAME_AS_TEXTURE));
        TTSetTxt(IDC_RENDER_FRAME_AS_TEXTURE, "Render N64 frame buffer as texture:\n\nWhen this option is enabled, content of each N64 frame buffer is rendered as texture over the frame, rendered by the plugin.\nThis prevents graphics lost, but may cause slowdowns and various glitches in some games.\n\n[Recommended: mostly off]");
        m_cbxFBasTex.SetCheck(g_settings->fb_read_back_to_screen_enabled() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxDetect.Attach(GetDlgItem(IDC_CHK_DETECT_CPU_WRITE));
        TTSetTxt(IDC_CHK_DETECT_CPU_WRITE, "Detect CPU write to the N64 frame buffer:\n\nThis option works as the previous options, but the plugin is trying to detect, when game uses CPU writes to N64 frame buffer.\nThe N64 frame buffer is rendered only when CPU writes is detected.\nUse this option for those games, in which you see still image or no image at all for some time with no reason.\n\n[Recommended: mostly off]");
        m_cbxDetect.SetCheck(g_settings->fb_cpu_write_hack_enabled() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxFBDepthBuffer.Attach(GetDlgItem(IDC_SOFTWARE_DEPTH_BUFFER));
        TTSetTxt(IDC_SOFTWARE_DEPTH_BUFFER, "Enable depth buffer rendering:\n\nThis option is used to fully emulate N64 depth buffer.\nIt is required for correct emulation of depth buffer based effects.\nHowever, it requires fast (>1GHz) CPU to work full speed.\n\n[Recommended: on for fast PC]");
        m_cbxFBDepthBuffer.SetCheck(g_settings->fb_depth_render_enabled() ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }

    bool OnApply()
    {
        g_settings->SetFiltering((CSettings::Filtering_t)m_cmbFiltering.GetItemData(m_cmbFiltering.GetCurSel()));
        g_settings->SetAspectmode((CSettings::AspectMode_t)m_cmbAspect.GetItemData(m_cmbAspect.GetCurSel()));
        g_settings->SetSwapMode((CSettings::SwapMode_t)m_cmbBufferSwap.GetItemData(m_cmbBufferSwap.GetCurSel()));
        g_settings->SetFog(m_cbxFog.GetCheck() == BST_CHECKED);
        g_settings->SetBuffClear(m_cbxBuffer.GetCheck() == BST_CHECKED);
        g_settings->SetLODmode((CSettings::PixelLevelOfDetail_t)m_cmbLOD.GetItemData(m_cmbLOD.GetCurSel()));

        CButton * fb_buttons[] =
        {
            &m_cbxFBEnable, &m_cbxFBHWFBE, &m_cbxFBReadEveryFrame,
            &m_cbxFBasTex, &m_cbxDetect,
            &m_cbxFBGetFBI, &m_cbxFBDepthBuffer
        };

        CSettings::fb_bits_t bits[] =
        {
            CSettings::fb_emulation, CSettings::fb_hwfbe, CSettings::fb_ref,
            CSettings::fb_read_back_to_screen, CSettings::fb_cpu_write_hack,
            CSettings::fb_get_info, CSettings::fb_depth_render
        };

        uint32_t fb_add_bits = 0, fb_remove_bits = 0;
        for (int i = 0; i < (sizeof(fb_buttons) / sizeof(fb_buttons[0])); i++)
        {
            if (fb_buttons[i]->GetCheck() == BST_CHECKED)
            {
                fb_add_bits |= bits[i];
            }
            else
            {
                fb_remove_bits |= bits[i];
            }
        }

        g_settings->UpdateFrameBufferBits(fb_add_bits, fb_remove_bits);
        if (g_settings->dirty())
        {
            g_settings->WriteSettings();
        }
        return true;
    }
private:
    void ItemChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

    CComboBox m_cmbFiltering;
    CComboBox m_cmbBufferSwap;
    CComboBox m_cmbLOD;
    CComboBox m_cmbAspect;
    CButton m_cbxFog;
    CButton m_cbxBuffer;
    CButton m_cbxFBEnable;
    CButton m_cbxFBHWFBE;
    CButton m_cbxFBGetFBI;
    CButton m_cbxFBReadEveryFrame;
    CButton m_cbxFBasTex;
    CButton m_cbxDetect;
    CButton m_cbxFBDepthBuffer;
};

class CDebugSettings :
    public CPropertyPageImpl<CDebugSettings>,
    public CToolTipDialog<CDebugSettings>
{
public:
    enum { IDD = IDD_DEBUG_SETTINGS };

    BEGIN_MSG_MAP(CDebugSettings)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_SETTINGS, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_UNKNOWN, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_GLIDE64, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_INTERFACE, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_RESOLUTION, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_GLITCH, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_RDP, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_TLUT, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_PNG, CBN_SELCHANGE, ItemChanged)
        COMMAND_HANDLER_EX(IDC_CMB_TRACE_OGLWRAPPER, CBN_SELCHANGE, ItemChanged)
        CHAIN_MSG_MAP(CToolTipDialog<CDebugSettings>)
        CHAIN_MSG_MAP(CPropertyPageImpl<CDebugSettings>)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        TTInit();
        TTSize(400);

        m_cmbTraceSettings.Attach(GetDlgItem(IDC_CMB_TRACE_SETTINGS));
        m_cmbTraceUnknown.Attach(GetDlgItem(IDC_CMB_TRACE_UNKNOWN));
        m_cmbTraceGlide64.Attach(GetDlgItem(IDC_CMB_TRACE_GLIDE64));
        m_cmbTraceInterface.Attach(GetDlgItem(IDC_CMB_TRACE_INTERFACE));
        m_cmbTraceresolution.Attach(GetDlgItem(IDC_CMB_TRACE_RESOLUTION));
        m_cmbTraceGlitch.Attach(GetDlgItem(IDC_CMB_TRACE_GLITCH));
        m_cmbTraceRDP.Attach(GetDlgItem(IDC_CMB_TRACE_RDP));
        m_cmbTraceTLUT.Attach(GetDlgItem(IDC_CMB_TRACE_TLUT));
        m_cmbTracePNG.Attach(GetDlgItem(IDC_CMB_TRACE_PNG));
        m_cmbTraceOGLWrapper.Attach(GetDlgItem(IDC_CMB_TRACE_OGLWRAPPER));
        m_cmbTraceRDPCommands.Attach(GetDlgItem(IDC_CMB_TRACE_RDP_COMMANDS));

        struct {
            CComboBox & cmb;
            uint16_t SettingId;
        } TraceCMB[] =
        {
            { m_cmbTraceSettings, Set_Logging_Settings },
            { m_cmbTraceUnknown, Set_Logging_Unknown },
            { m_cmbTraceGlide64, Set_Logging_Glide64 },
            { m_cmbTraceInterface, Set_Logging_Interface },
            { m_cmbTraceresolution, Set_Logging_Resolution },
            { m_cmbTraceGlitch, Set_Logging_Glitch },
            { m_cmbTraceRDP, Set_Logging_VideoRDP },
            { m_cmbTraceTLUT, Set_Logging_TLUT },
            { m_cmbTracePNG, Set_Logging_PNG },
            { m_cmbTraceOGLWrapper, Set_Logging_OGLWrapper },
            { m_cmbTraceRDPCommands, Set_Logging_RDPCommands },
        };

        for (size_t i = 0, n = sizeof(TraceCMB) / sizeof(TraceCMB[0]); i < n; i++)
        {
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Error"), TraceError);
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Warning"), TraceWarning);
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Notice"), TraceNotice);
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Info"), TraceInfo);
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Debug"), TraceDebug);
            TraceCMB[i].cmb.SetItemData(TraceCMB[i].cmb.AddString("Verbose"), TraceVerbose);
            SetComboBoxIndex(TraceCMB[i].cmb, (uint32_t)GetSetting(TraceCMB[i].SettingId));
        }
        return TRUE;
    }

    bool OnApply()
    {
        struct {
            CComboBox & cmb;
            uint16_t SettingId;
        } TraceCMB[] =
        {
            { m_cmbTraceSettings, Set_Logging_Settings },
            { m_cmbTraceUnknown, Set_Logging_Unknown },
            { m_cmbTraceGlide64, Set_Logging_Glide64 },
            { m_cmbTraceInterface, Set_Logging_Interface },
            { m_cmbTraceresolution, Set_Logging_Resolution },
            { m_cmbTraceGlitch, Set_Logging_Glitch },
            { m_cmbTraceRDP, Set_Logging_VideoRDP },
            { m_cmbTraceTLUT, Set_Logging_TLUT },
            { m_cmbTracePNG, Set_Logging_PNG },
            { m_cmbTraceOGLWrapper, Set_Logging_OGLWrapper },
            { m_cmbTraceRDPCommands, Set_Logging_RDPCommands },
        };
        for (size_t i = 0, n = sizeof(TraceCMB) / sizeof(TraceCMB[0]); i < n; i++)
        {
            SetSetting(TraceCMB[i].SettingId, TraceCMB[i].cmb.GetItemData(TraceCMB[i].cmb.GetCurSel()));
        }
        return true;
    }
private:
    void ItemChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

    CComboBox m_cmbTraceSettings;
    CComboBox m_cmbTraceUnknown;
    CComboBox m_cmbTraceGlide64;
    CComboBox m_cmbTraceInterface;
    CComboBox m_cmbTraceresolution;
    CComboBox m_cmbTraceGlitch;
    CComboBox m_cmbTraceRDP;
    CComboBox m_cmbTraceTLUT;
    CComboBox m_cmbTracePNG;
    CComboBox m_cmbTraceOGLWrapper;
    CComboBox m_cmbTraceRDPCommands;
};

class CConfigTextureEnhancement :
    public CPropertyPageImpl<CConfigTextureEnhancement>,
    public CToolTipDialog < CConfigTextureEnhancement >
{
public:
    enum { IDD = IDD_TEXTURE_ENHANCEMENT };

    BEGIN_MSG_MAP(CConfigTextureEnhancement)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(CToolTipDialog<CConfigTextureEnhancement>)
        CHAIN_MSG_MAP(CPropertyPageImpl<CConfigTextureEnhancement>)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        TTInit();
        TTSize(400);

        std::string tooltip = "Filters:\n\nApply a filter to either smooth or sharpen textures.\nThere are 4 different smoothing filters and 2 different sharpening filters.\nThe higher the number, the stronger the effect,\ni.e. \"Smoothing filter 4\" will have a much more noticeable effect than \"Smoothing filter 1\".\nBe aware that performance may have an impact depending on the game and/or the PC.\n\n[Recommended: your preference]";
        TTSetTxt(IDC_TXT_ENH_FILTER, tooltip.c_str());
        TTSetTxt(IDC_CMB_ENH_FILTER, tooltip.c_str());
        m_cmbEnhFilter.Attach(GetDlgItem(IDC_CMB_ENH_FILTER));
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("None"), CSettings::TextureFilter_None);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Smooth filtering 1"), CSettings::TextureFilter_SmoothFiltering);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Smooth filtering 2"), CSettings::TextureFilter_SmoothFiltering2);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Smooth filtering 3"), CSettings::TextureFilter_SmoothFiltering3);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Smooth filtering 4"), CSettings::TextureFilter_SmoothFiltering4);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Sharp filtering 1"), CSettings::TextureFilter_SharpFiltering1);
        m_cmbEnhFilter.SetItemData(m_cmbEnhFilter.AddString("Sharp filtering 2"), CSettings::TextureFilter_SharpFiltering2);
        SetComboBoxIndex(m_cmbEnhFilter, g_settings->ghq_fltr());

        tooltip = "Texture enhancement:\n\n7 different filters are selectable here, each one with a distinctive look.\nBe aware of possible performance impacts.\n\nIMPORTANT: 'Store' mode - saves textures in cache 'as is'.\nIt can improve performance in games, which load many textures.\nDisable 'Ignore backgrounds' option for better result.\n\n[Recommended: your preference]";
        TTSetTxt(IDC_TXT_ENHANCEMENT, tooltip.c_str());
        TTSetTxt(IDC_CMB_ENHANCEMENT, tooltip.c_str());
        m_cmbEnhEnhancement.Attach(GetDlgItem(IDC_CMB_ENHANCEMENT));
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("None"), CSettings::TextureEnht_None);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("X2"), CSettings::TextureEnht_X2);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("X2SAI"), CSettings::TextureEnht_X2SAI);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("HQ2X"), CSettings::TextureEnht_HQ2X);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("HQ2XS"), CSettings::TextureEnht_HQ2XS);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("LQ2X"), CSettings::TextureEnht_LQ2X);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("LQ2XS"), CSettings::TextureEnht_LQ2XS);
        m_cmbEnhEnhancement.SetItemData(m_cmbEnhEnhancement.AddString("HQ4X"), CSettings::TextureEnht_HQ4X);
        SetComboBoxIndex(m_cmbEnhEnhancement, g_settings->ghq_enht());

        tooltip = "Hi-res pack format:\n\nChoose which method is to be used for loading Hi-res texture packs.\nOnly Rice's format is available currently.\nLeave on \"None\" if you will not be needing to load hi-res packs.\n\n[Recommended: Rice's format. Default: \"None\"]";
        TTSetTxt(IDC_TXT_FORMAT_CHOICES, tooltip.c_str());
        TTSetTxt(IDC_CMB_FORMAT_CHOICES, tooltip.c_str());
        m_cmbHrsFormat.Attach(GetDlgItem(IDC_CMB_FORMAT_CHOICES));
        m_cmbHrsFormat.SetItemData(m_cmbHrsFormat.AddString("None"), CSettings::HiResPackFormat_None);
        m_cmbHrsFormat.SetItemData(m_cmbHrsFormat.AddString("Rice format"), CSettings::HiResPackFormat_Riceformat);
        SetComboBoxIndex(m_cmbHrsFormat, g_settings->ghq_hirs());

        m_cmbTextureCompression.Attach(GetDlgItem(IDC_CMB_TEX_COMPRESS_MEHTOD));
        m_cmbTextureCompression.SetItemData(m_cmbTextureCompression.AddString("S3TC"), CSettings::TextureCompression_S3TC);
        m_cmbTextureCompression.SetItemData(m_cmbTextureCompression.AddString("FXT1"), CSettings::TextureCompression_FXT1);
        SetComboBoxIndex(m_cmbTextureCompression, g_settings->ghq_cmpr());

        tooltip = "Texture cache size:\n\nEnhanced and filtered textures can be cached to aid performance.\nThis setting will adjust how much PC memory will be dedicated for texture cache.\nThis helps boost performance if there are subsequent requests for the same texture (usually the case).\nNormally, 128MB should be more than enough but there is a sweet spot for each game.\nSuper Mario may not need more than 32megs, but Conker streams a lot of textures, so setting 256+ megs can boost performance.\nAdjust accordingly if you are encountering speed issues.\n'0' disables cache.\n\n[Recommended: PC and game dependant]";
        TTSetTxt(IDC_TXT_TEXTURE_CACHE, tooltip.c_str());
        TTSetTxt(IDC_SPIN_TEXTURE_CACHE, tooltip.c_str());
        TTSetTxt(IDC_TEXT_MB, tooltip.c_str());
        m_textTexCache.Attach(GetDlgItem(IDC_TXT_TEXTURE_CACHE));
        m_textTexCache.SetWindowTextA(stdstr_f("%d", g_settings->ghq_cache_size()).c_str());
        m_spinEnhCacheSize.Attach(GetDlgItem(IDC_SPIN_TEXTURE_CACHE));
        m_spinEnhCacheSize.SetBuddy(m_textTexCache);

        TTSetTxt(IDC_CHK_IGNORE_BACKGROUND, "Ignore Backgrounds:\n\nIt is used to skip enhancement for long narrow textures, usually used for backgrounds.\nThis may save texture memory greatly and increase performance.\n\n[Recommended: on (off for 'Store' mode)]");
        m_cbxEnhIgnoreBG.Attach(GetDlgItem(IDC_CHK_IGNORE_BACKGROUND));
        m_cbxEnhIgnoreBG.SetCheck(g_settings->ghq_enht_nobg() ? BST_CHECKED : BST_UNCHECKED);

        tooltip = "Texture compression:\n\nTextures will be compressed using selected texture compression method.\nThe overall compression ratio is about 1/6 for FXT1 and 1/4 for S3TC.\nIn addition to saving space on the texture cache, the space occupied on the GFX hardware's texture RAM, by the enhanced textures, will be greatly reduced.\nThis minimizes texture RAM usage, decreasing the number of texture swaps to the GFX hardware leading to performance gains.\nHowever, due to the nature of lossy compression of FXT1 and S3TC, using this option can sometimes lead to quality degradation of small size textures and color banding of gradient colored textures.\n\n[Recommended: off]";
        TTSetTxt(IDC_CHK_TEX_COMPRESSION, tooltip.c_str());
        TTSetTxt(IDC_CHK_HIRES_TEX_COMPRESSION, tooltip.c_str());

        m_cbxEnhTexCompression.Attach(GetDlgItem(IDC_CHK_TEX_COMPRESSION));
        m_cbxEnhTexCompression.SetCheck(g_settings->ghq_enht_cmpr() ? BST_CHECKED : BST_UNCHECKED);
        m_cbxHrsTexCompression.Attach(GetDlgItem(IDC_CHK_HIRES_TEX_COMPRESSION));
        m_cbxHrsTexCompression.SetCheck(g_settings->ghq_hirs_cmpr() ? BST_CHECKED : BST_UNCHECKED);

        TTSetTxt(IDC_CHK_COMPRESS_CACHE, "Compress texture cache:\n\nMemory will be compressed so that more textures can be held in the texture cache.\nThe compression ratio varies with each texture, but 1/5 of the original size would be a modest approximation.\nThey will be decompressed on-the-fly, before being downloaded to the gfx hardware.\nThis option will still help save memory space even when using texture compression.\n\n[Recommended: on]");
        m_cbxEnhCompressCache.Attach(GetDlgItem(IDC_CHK_COMPRESS_CACHE));
        m_cbxEnhCompressCache.SetCheck(g_settings->ghq_enht_gz() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxHrsTile.Attach(GetDlgItem(IDC_CHK_TILE_TEX));
        TTSetTxt(IDC_CHK_TILE_TEX, "Tile textures:\n\nWhen on, wide texture will be split on several tiles to fit in one 256-width texture.\nThis tiled texture takes much less video memory space and thus overall performance will increase.\nHowever, corresponding polygons must be split too, and this is not polished yet - various issues are possible, including black lines and polygons distortions.\n\n[Recommended: off]");
        m_cbxHrsTile.SetCheck(g_settings->ghq_hirs_tile() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxHrsForce16.Attach(GetDlgItem(IDC_CHK_FORCE_16BPP_TEXT));
        TTSetTxt(IDC_CHK_FORCE_16BPP_TEXT, "Force 16bpp textures:\n\nThe color of the textures will be reduced to 16bpp.\nThis is another space saver and performance enhancer.\nThis halves the space used on the texture cache and the GFX hardware's texture RAM.\nColor reduction is done so that the original quality is preserved as much as possible.\nDepending on the texture, this usually is hardly noticeable.\nSometimes though, it can be: skies are a good example.\n\n[Recommended: off]");
        m_cbxHrsForce16.SetCheck(g_settings->ghq_hirs_f16bpp() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxHrsTexEdit.Attach(GetDlgItem(IDC_CHK_TEX_DUMP_EDIT));
        TTSetTxt(IDC_CHK_TEX_DUMP_EDIT, "Texture dumping mode:\n\nIn this mode, you have that ability to dump textures on screen to the appropriate folder.\nYou can also reload textures while the game is running to see how they look instantly - big time saver!\n\nHotkeys:\n\"R\" reloads hires textures from the texture pack\n\"D\" toggles texture dumps on/off.");
        m_cbxHrsTexEdit.SetCheck(g_settings->ghq_hirs_dump() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxHrsAltCRC.Attach(GetDlgItem(IDC_CHK_ALT_CRC));
        TTSetTxt(IDC_CHK_ALT_CRC, "Alternative CRC calculation:\n\nThis option enables emulation of a palette CRC calculation bug in RiceVideo.\nIf some textures are not loaded, try to set this option on/off.\n\n[Recommended: texture pack dependant, mostly on]");
        m_cbxHrsAltCRC.SetCheck(g_settings->ghq_hirs_altcrc() ? BST_CHECKED : BST_UNCHECKED);
        if (g_settings->ghq_hirs_dump())
        {
            m_cbxHrsAltCRC.EnableWindow(false);
        }

        m_cbxHrsCompressCache.Attach(GetDlgItem(IDC_CHK_HRS_COMPRESS_CACHE));
        TTSetTxt(IDC_CHK_HRS_COMPRESS_CACHE, "Compress texture cache:\n\nWhen game started, plugin loads all its hi-resolution textures into PC memory.\nSince hi-resolution textures are usually large, the whole pack can take hundreds megabytes of memory.\nCache compression allows save memory space greatly.\nTextures will be decompressed on-the-fly, before being downloaded to the gfx hardware.\nThis option will still help save memory space even when using texture compression.\n\n[Recommended: on]");
        m_cbxHrsCompressCache.SetCheck(g_settings->ghq_hirs_gz() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxHrsLetFly.Attach(GetDlgItem(IDC_CHK_USE_ALPHA_FULLY));
        TTSetTxt(IDC_CHK_USE_ALPHA_FULLY, "Use Alpha channel fully:\n\nWhen this option is off, 16bit rgba textures will be loaded using RiceVideo style, with 1bit for alpha channel.\nWhen it is on, GlideHQ will check, how alpha channel is used by the hires texture, and select most appropriate format for it.\nThis gives texture designers freedom to play with alpha, as they need, regardless of format of original N64 texture.\nFor older and badly designed texture packs it can cause unwanted black borders.\n\n[Recommended: texture pack dependant]");
        m_cbxHrsLetFly.SetCheck(g_settings->ghq_hirs_let_texartists_fly() ? BST_CHECKED : BST_UNCHECKED);

        m_cbxSaveTexCache.Attach(GetDlgItem(IDC_CHK_TEX_CACHE_HD));
        TTSetTxt(IDC_CHK_TEX_CACHE_HD, "Save texture cache to HD:\n\nFor enhanced textures cache:\nThis will save all previously loaded and enhanced textures to HD.\nSo upon next game launch, all the textures will be instantly loaded, resulting in smoother performance.\n\nFor high-resolution textures cache:\nAfter creation, loading hi-res texture will take only a few seconds upon game launch, as opposed to the 5 to 60 seconds a pack can take to load without this cache file.\nThe only downside here is upon any changes to the pack, the cache file will need to be manually deleted.\n\nSaved cache files go into a folder called \"Cache\" within the Textures folder.\n\n[Highly Recommended: on]");
        m_cbxSaveTexCache.SetCheck(g_settings->ghq_cache_save() ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }

    bool OnApply()
    {
        char texcache[100];
        m_textTexCache.GetWindowText(texcache, sizeof(texcache));

        g_settings->SetGhqFltr((CSettings::TextureFilter_t)m_cmbEnhFilter.GetItemData(m_cmbEnhFilter.GetCurSel()));
        g_settings->SetGhqEnht((CSettings::TextureEnhancement_t)m_cmbEnhEnhancement.GetItemData(m_cmbEnhEnhancement.GetCurSel()));
        g_settings->SetGhqCacheSize(atoi(texcache));
        g_settings->SetGhqEnhtNobg(m_cbxEnhIgnoreBG.GetCheck() == BST_CHECKED);
        g_settings->SetGhqEnhtCmpr(m_cbxEnhTexCompression.GetCheck() == BST_CHECKED);
        g_settings->SetGhqEnhtGz(m_cbxEnhCompressCache.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirs((CSettings::HiResPackFormat_t)m_cmbHrsFormat.GetItemData(m_cmbHrsFormat.GetCurSel()));
        g_settings->SetGhqHirsTile(m_cbxHrsTile.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsF16bpp(m_cbxHrsForce16.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsDump(m_cbxHrsTexEdit.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsAltcrc(m_cbxHrsAltCRC.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsCmpr(m_cbxHrsTexCompression.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsGz(m_cbxHrsCompressCache.GetCheck() == BST_CHECKED);
        g_settings->SetGhqHirsLetTexartistsFly(m_cbxHrsLetFly.GetCheck() == BST_CHECKED);
        g_settings->SetGhqCmpr((CSettings::TextureCompression_t)m_cmbTextureCompression.GetItemData(m_cmbTextureCompression.GetCurSel()));
        g_settings->SetGhqCacheSave(m_cbxSaveTexCache.GetCheck() == BST_CHECKED);
        if (g_settings->dirty())
        {
            g_settings->WriteSettings();
        }
        return true;
    }
private:
    void ItemChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }

    CComboBox m_cmbEnhFilter;
    CComboBox m_cmbEnhEnhancement;
    CComboBox m_cmbHrsFormat;
    CComboBox m_cmbTextureCompression;
    CButton m_cbxEnhIgnoreBG;
    CButton m_cbxEnhTexCompression;
    CButton m_cbxHrsTexCompression;
    CButton m_cbxEnhCompressCache;
    CButton m_cbxHrsTile;
    CButton m_cbxHrsForce16;
    CButton m_cbxHrsTexEdit;
    CButton m_cbxHrsAltCRC;
    CButton m_cbxHrsCompressCache;
    CButton m_cbxHrsLetFly;
    CButton m_cbxSaveTexCache;
    CEdit m_textTexCache;
    CUpDownCtrl m_spinEnhCacheSize;
};

COptionsSheet::COptionsSheet(_U_STRINGorID /*title*/, UINT /*uStartPage*/, HWND /*hWndParent*/) :
    m_pgBasicPage(new CConfigBasicPage(this)),
    m_pgEmuSettings(new CConfigEmuSettings),
    m_pgDebugSettings(new CDebugSettings),
    m_pgTextureEnhancement(NULL),
    m_hTextureEnhancement(0)
{
    AddPage(&m_pgBasicPage->m_psp);
    if (g_settings->advanced_options())
    {
        AddPage(&m_pgEmuSettings->m_psp);
    }
    if (g_settings->debugger_enabled())
    {
        AddPage(&m_pgDebugSettings->m_psp);
    }
    UpdateTextureSettings();
}

COptionsSheet::~COptionsSheet()
{
    delete m_pgBasicPage;
    delete m_pgEmuSettings;
    delete m_pgDebugSettings;
    delete m_pgTextureEnhancement;
}

void COptionsSheet::UpdateTextureSettings(void)
{
    if (g_settings->texenh_options())
    {
        if (m_hTextureEnhancement == NULL)
        {
            m_pgTextureEnhancement = new CConfigTextureEnhancement;
            m_hTextureEnhancement = m_pgTextureEnhancement->Create();
            AddPage(m_hTextureEnhancement);
        }
    }
    else if (m_hTextureEnhancement != NULL)
    {
        RemovePage(m_hTextureEnhancement);
        m_hTextureEnhancement = NULL;
        delete m_pgTextureEnhancement;
        m_pgTextureEnhancement = NULL;
    }
}
#endif

/******************************************************************
Function: DllConfig
Purpose:  This function is optional function that is provided
to allow the user to configure the dll
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
void CALL DllConfig(void * hParent)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");
#ifdef _WIN32
    CGuard guard(*g_ProcessDListCS);

    if (g_romopen)
    {
        ReleaseGfx();
        rdp.free();
        rdp.init();
        if (g_ghq_use)
        {
            ext_ghq_shutdown();
            g_ghq_use = false;
        }
    }
    else
    {
        char name[21] = "DEFAULT";
        g_settings->ReadGameSettings(name);
        ZLUT_init();
    }

    COptionsSheet("Glide64 settings").DoModal((HWND)hParent);
    CloseConfig();
#endif
}

void CloseConfig()
{
    if (g_romopen)
    {
        if (g_settings->fb_depth_render_enabled())
        {
            ZLUT_init();
        }
        InitGfx();
    }
}

#ifdef _WIN32
class CAboutDlg :
    public CDialogImpl<CAboutDlg>
{
public:
    enum { IDD = IDD_ABOUTBOX };

    BEGIN_MSG_MAP(CAboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        return TRUE;
    }

    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }
};
#endif

/******************************************************************
Function: DllAbout
Purpose:  This function is optional function that is provided
to give further information about the DLL.
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
void CALL DllAbout(void * /*hParent*/)
{
#ifdef _WIN32
    CAboutDlg dlg;
    dlg.DoModal();
#endif
}