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
#include "stdafx.h"

#include "SettingsPage.h"
#include "SettingsPage-Game-Plugin.h"

CGamePluginPage::CGamePluginPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemTextW(m_hWnd, RSP_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemTextW(m_hWnd, GFX_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemTextW(m_hWnd, AUDIO_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemTextW(m_hWnd, CONT_ABOUT, wGS(PLUG_ABOUT).c_str());

    SetDlgItemTextW(m_hWnd, IDC_RSP_NAME, wGS(PLUG_RSP).c_str());
    SetDlgItemTextW(m_hWnd, IDC_GFX_NAME, wGS(PLUG_GFX).c_str());
    SetDlgItemTextW(m_hWnd, IDC_AUDIO_NAME, wGS(PLUG_AUDIO).c_str());
    SetDlgItemTextW(m_hWnd, IDC_CONT_NAME, wGS(PLUG_CTRL).c_str());

    SetDlgItemTextW(m_hWnd, IDC_HLE_GFX, wGS(PLUG_HLE_GFX).c_str());
    SetDlgItemTextW(m_hWnd, IDC_HLE_AUDIO, wGS(PLUG_HLE_AUDIO).c_str());

    m_GfxGroup.Attach(GetDlgItem(IDC_GFX_NAME));
    m_AudioGroup.Attach(GetDlgItem(IDC_AUDIO_NAME));
    m_ControlGroup.Attach(GetDlgItem(IDC_CONT_NAME));
    m_RspGroup.Attach(GetDlgItem(IDC_RSP_NAME));

    AddPlugins(GFX_LIST, Game_EditPlugin_Gfx, PLUGIN_TYPE_GFX);
    AddPlugins(AUDIO_LIST, Game_EditPlugin_Audio, PLUGIN_TYPE_AUDIO);
    AddPlugins(CONT_LIST, Game_EditPlugin_Contr, PLUGIN_TYPE_CONTROLLER);
    AddPlugins(RSP_LIST, Game_EditPlugin_RSP, PLUGIN_TYPE_RSP);

    AddModCheckBox(GetDlgItem(IDC_HLE_GFX), Game_UseHleGfx);
    AddModCheckBox(GetDlgItem(IDC_HLE_AUDIO), Game_UseHleAudio);

    UpdatePageSettings();
}

void CGamePluginPage::AddPlugins(int ListId, SettingID Type, PLUGIN_TYPE PluginType)
{
    stdstr Default;
    bool PluginSelected = g_Settings->LoadStringVal(Type, Default);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(ListId), Type);
    if (!PluginSelected)
    {
        ComboBox->SetDefault(NULL);
    }
    ComboBox->AddItemW(wGS(PLUG_DEFAULT).c_str(), NULL);

    for (int i = 0, n = m_PluginList.GetPluginCount(); i < n; i++)
    {
        const CPluginList::PLUGIN * Plugin = m_PluginList.GetPluginInfo(i);
        if (Plugin == NULL)
        {
            continue;
        }
        if (Plugin->Info.Type != PluginType)
        {
            continue;
        }
        if (PluginSelected && _stricmp(Default.c_str(), Plugin->FileName.c_str()) == 0)
        {
            ComboBox->SetDefault((WPARAM)Plugin);
        }
        ComboBox->AddItem(Plugin->Info.Name, (WPARAM)Plugin);
    }
}

void CGamePluginPage::ShowAboutButton(int id)
{
    CModifiedComboBox * ComboBox = NULL;
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        if ((int)(cb_iter->second->GetMenu()) != id)
        {
            continue;
        }
        ComboBox = cb_iter->second;
        break;
    }
    if (ComboBox == NULL)
    {
        return;
    }
    int index = ComboBox->GetCurSel();
    if (index == CB_ERR)
    {
        return;
    }

    const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox->GetItemDataPtr(index);
    if (PluginPtr == NULL)
    {
        return;
    }

    const CPluginList::PLUGIN * Plugin = *PluginPtr;
    if (Plugin == NULL)
    {
        return;
    }

    //Load the plugin
    UINT LastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HMODULE hLib = LoadLibrary(Plugin->FullPath);
    SetErrorMode(LastErrorMode);
    if (hLib == NULL)
    {
        return;
    }

    //Get DLL about
    void(CALL *DllAbout) (HWND hWnd);
    DllAbout = (void(CALL *)(HWND))GetProcAddress(hLib, "DllAbout");

    //call the function from the dll
    DllAbout(m_hWnd);

    FreeLibrary(hLib);
}

void CGamePluginPage::PluginItemChanged(int id, int AboutID, bool bSetChanged)
{
    CModifiedComboBox * ComboBox = NULL;
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        if ((int)(cb_iter->second->GetMenu()) != id)
        {
            continue;
        }
        ComboBox = cb_iter->second;
        break;
    }
    if (ComboBox == NULL)
    {
        return;
    }

    int index = ComboBox->GetCurSel();
    if (index == CB_ERR)
    {
        return;
    }
    const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox->GetItemDataPtr(index);
    if (PluginPtr)
    {
        const CPluginList::PLUGIN * Plugin = *PluginPtr;
        if (Plugin)
        {
            ::EnableWindow(GetDlgItem(AboutID), Plugin->AboutFunction);
        }
    }

    if (bSetChanged)
    {
        ComboBox->SetChanged(true);
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
}

void CGamePluginPage::UpdatePageSettings(void)
{
    UpdateCheckBoxes();
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        CModifiedComboBox * ComboBox = cb_iter->second;
        stdstr SelectedValue;

        bool PluginChanged = g_Settings->LoadStringVal(cb_iter->first, SelectedValue);
        ComboBox->SetChanged(PluginChanged);
        if (PluginChanged)
        {
            for (int i = 0, n = m_PluginList.GetPluginCount(); i < n; i++)
            {
                const CPluginList::PLUGIN * Plugin = m_PluginList.GetPluginInfo(i);
                if (Plugin == NULL)
                {
                    continue;
                }
                if (_stricmp(SelectedValue.c_str(), Plugin->FileName.c_str()) != 0)
                {
                    continue;
                }
                ComboBox->SetDefault((WPARAM)Plugin);
            }
        }
        else {
            ComboBox->SetDefault(NULL);
        }
    }
    PluginItemChanged(GFX_LIST, GFX_ABOUT, false);
    PluginItemChanged(AUDIO_LIST, AUDIO_ABOUT, false);
    PluginItemChanged(CONT_LIST, CONT_ABOUT, false);
    PluginItemChanged(RSP_LIST, RSP_ABOUT, false);
}

void CGamePluginPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGamePluginPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CGamePluginPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGamePluginPage>::ApplySettings(UpdateScreen);
}

bool CGamePluginPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGamePluginPage>::EnableReset()) { return true; }
    return false;
}

void CGamePluginPage::ResetPage()
{
    CSettingsPageImpl<CGamePluginPage>::ResetPage();
}

void CGamePluginPage::ApplyComboBoxes(void)
{
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        CModifiedComboBox * ComboBox = cb_iter->second;
        if (ComboBox->IsChanged())
        {
            int index = ComboBox->GetCurSel();
            if (index == CB_ERR)
            {
                return;
            }

            const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox->GetItemDataPtr(index);
            if (PluginPtr == NULL)
            {
                return;
            }

            const CPluginList::PLUGIN * Plugin = *PluginPtr;

            if (Plugin)
            {
                if (g_Settings->LoadStringVal(cb_iter->first) != Plugin->FileName.c_str())
                {
                    g_Settings->SaveString(cb_iter->first, Plugin->FileName.c_str());
                }
            }
            else {
                g_Settings->DeleteSetting(cb_iter->first);
            }
        }
        if (ComboBox->IsReset())
        {
            g_Settings->DeleteSetting(cb_iter->first);
            ComboBox->SetReset(false);
        }
    }
}

bool CGamePluginPage::ResetComboBox(CModifiedComboBox & ComboBox, SettingID /*Type*/)
{
    if (!ComboBox.IsChanged())
    {
        return false;
    }

    ComboBox.SetReset(true);
    for (int i = 0, n = ComboBox.GetCount(); i < n; i++)
    {
        const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox.GetItemDataPtr(i);
        if (PluginPtr == NULL)
        {
            continue;
        }
        if (*PluginPtr != NULL)
        {
            continue;
        }
        ComboBox.SetCurSel(i);
        return true;
    }
    return false;
}

void CGamePluginPage::HleGfxChanged(UINT /*Code*/, int id, HWND /*ctl*/)
{
    for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter++)
    {
        CModifiedButton * Button = iter->second;
        if ((int)Button->GetMenu() != id)
        {
            continue;
        }
        if ((Button->GetCheck() & BST_CHECKED) == 0)
        {
            if (!g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SET_LLE_GFX_MSG).c_str()))
            {
                Button->SetCheck(BST_CHECKED);
                return;
            }
        }
        Button->SetChanged(true);
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
        break;
    }
}

void CGamePluginPage::HleAudioChanged(UINT /*Code*/, int id, HWND /*ctl*/)
{
    for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter++)
    {
        CModifiedButton * Button = iter->second;
        if ((int)Button->GetMenu() != id)
        {
            continue;
        }
        if ((Button->GetCheck() & BST_CHECKED) != 0)
        {          
            if (!g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SET_HLE_AUD_MSG).c_str()))
            {
                Button->SetCheck(BST_UNCHECKED);
                return;
            }
        }
        Button->SetChanged(true);
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
        break;
    }
}