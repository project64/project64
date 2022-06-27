#include "stdafx.h"

#include "SettingsPage.h"

COptionPluginPage::COptionPluginPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    // Set the text for all GUI items
    SetDlgItemText(RSP_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemText(GFX_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemText(AUDIO_ABOUT, wGS(PLUG_ABOUT).c_str());
    SetDlgItemText(CONT_ABOUT, wGS(PLUG_ABOUT).c_str());

    SetDlgItemText(IDC_RSP_NAME, wGS(PLUG_RSP).c_str());
    SetDlgItemText(IDC_GFX_NAME, wGS(PLUG_GFX).c_str());
    SetDlgItemText(IDC_AUDIO_NAME, wGS(PLUG_AUDIO).c_str());
    SetDlgItemText(IDC_CONT_NAME, wGS(PLUG_CTRL).c_str());

    SetDlgItemText(IDC_HLE_GFX, wGS(PLUG_HLE_GFX).c_str());
    SetDlgItemText(IDC_HLE_AUDIO, wGS(PLUG_HLE_AUDIO).c_str());

    m_GfxGroup.Attach(GetDlgItem(IDC_GFX_NAME));
    m_AudioGroup.Attach(GetDlgItem(IDC_AUDIO_NAME));
    m_ControlGroup.Attach(GetDlgItem(IDC_CONT_NAME));
    m_RspGroup.Attach(GetDlgItem(IDC_RSP_NAME));

    AddPlugins(GFX_LIST, Plugin_GFX_Current, PLUGIN_TYPE_VIDEO);
    AddPlugins(AUDIO_LIST, Plugin_AUDIO_Current, PLUGIN_TYPE_AUDIO);
    AddPlugins(CONT_LIST, Plugin_CONT_Current, PLUGIN_TYPE_CONTROLLER);
    AddPlugins(RSP_LIST, Plugin_RSP_Current, PLUGIN_TYPE_RSP);

    AddModCheckBox(GetDlgItem(IDC_HLE_GFX), Plugin_UseHleGfx);
    AddModCheckBox(GetDlgItem(IDC_HLE_AUDIO), Plugin_UseHleAudio);

    UpdatePageSettings();
}

void COptionPluginPage::AddPlugins(int ListId, SettingID Type, PLUGIN_TYPE PluginType)
{
    stdstr Default = g_Settings->LoadStringVal(Type);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(ListId), Type);
    for (int i = 0, n = m_PluginList.GetPluginCount(); i < n; i++)
    {
        const CPluginList::PLUGIN * Plugin = m_PluginList.GetPluginInfo(i);
        if (Plugin == nullptr)
        {
            continue;
        }
        if (Plugin->Info.Type != PluginType)
        {
            continue;
        }
        if (_stricmp(Default.c_str(), Plugin->FileName.c_str()) == 0)
        {
            ComboBox->SetDefault((WPARAM)Plugin);
        }
        ComboBox->AddItem(stdstr(Plugin->Info.Name).ToUTF16().c_str(), (WPARAM)Plugin);
    }
}

void COptionPluginPage::ShowAboutButton(int id)
{
    CModifiedComboBox * ComboBox = nullptr;
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        if ((int)(cb_iter->second->GetMenu()) != id)
        {
            continue;
        }
        ComboBox = cb_iter->second;
        break;
    }
    if (ComboBox == nullptr)
    {
        return;
    }
    int index = ComboBox->GetCurSel();
    if (index == CB_ERR)
    {
        return;
    }

    const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox->GetItemDataPtr(index);
    if (PluginPtr == nullptr)
    {
        return;
    }

    const CPluginList::PLUGIN * Plugin = *PluginPtr;
    if (Plugin == nullptr)
    {
        return;
    }

    // Load the plugin
    UINT LastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HMODULE hLib = LoadLibrary(stdstr((const char *)(Plugin->FullPath)).ToUTF16().c_str());
    SetErrorMode(LastErrorMode);
    if (hLib == nullptr)
    {
        return;
    }

    // Get DLL about
    void(CALL *DllAbout) (HWND hWnd);
    DllAbout = (void(CALL *)(HWND))GetProcAddress(hLib, "DllAbout");

    // Call the function from the DLL
    DllAbout(m_hWnd);

    FreeLibrary(hLib);
}

void COptionPluginPage::PluginItemChanged(int id, int AboutID, bool bSetChanged)
{
    CModifiedComboBox * ComboBox = nullptr;
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        if ((int)(cb_iter->second->GetMenu()) != id)
        {
            continue;
        }
        ComboBox = cb_iter->second;
        break;
    }
    if (ComboBox == nullptr)
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

void COptionPluginPage::UpdatePageSettings(void)
{
    UpdateCheckBoxes();
    for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter++)
    {
        CModifiedComboBox * ComboBox = cb_iter->second;
        stdstr SelectedValue;

        ComboBox->SetChanged(g_Settings->LoadStringVal(cb_iter->first, SelectedValue));
        for (int i = 0, n = ComboBox->GetCount(); i < n; i++)
        {
            const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox->GetItemDataPtr(i);
            if (PluginPtr == nullptr)
            {
                continue;
            }
            const CPluginList::PLUGIN * Plugin = *PluginPtr;
            if (Plugin == nullptr)
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
    PluginItemChanged(GFX_LIST, GFX_ABOUT, false);
    PluginItemChanged(AUDIO_LIST, AUDIO_ABOUT, false);
    PluginItemChanged(CONT_LIST, CONT_ABOUT, false);
    PluginItemChanged(RSP_LIST, RSP_ABOUT, false);
}

void COptionPluginPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void COptionPluginPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void COptionPluginPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<COptionPluginPage>::ApplySettings(UpdateScreen);
}

bool COptionPluginPage::EnableReset(void)
{
    if (CSettingsPageImpl<COptionPluginPage>::EnableReset()) { return true; }
    return false;
}

void COptionPluginPage::ResetPage()
{
    CSettingsPageImpl<COptionPluginPage>::ResetPage();
}

void COptionPluginPage::ApplyComboBoxes(void)
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
            if (PluginPtr == nullptr)
            {
                return;
            }

            const CPluginList::PLUGIN * Plugin = *PluginPtr;

            g_Settings->SaveString(cb_iter->first, Plugin->FileName.c_str());
        }
        if (ComboBox->IsReset())
        {
            g_Settings->DeleteSetting(cb_iter->first);
            ComboBox->SetReset(false);
        }
    }
}

bool COptionPluginPage::ResetComboBox(CModifiedComboBox & ComboBox, SettingID Type)
{
    if (!ComboBox.IsChanged())
    {
        return false;
    }

    ComboBox.SetReset(true);
    stdstr Value = g_Settings->LoadDefaultString(Type);
    for (int i = 0, n = ComboBox.GetCount(); i < n; i++)
    {
        const CPluginList::PLUGIN ** PluginPtr = (const CPluginList::PLUGIN **)ComboBox.GetItemDataPtr(i);
        if (PluginPtr == nullptr)
        {
            continue;
        }

        const CPluginList::PLUGIN * Plugin = *PluginPtr;
        if (Plugin->FileName != Value)
        {
            continue;
        }
        ComboBox.SetCurSel(i);
        return true;
    }
    return false;
}

void COptionPluginPage::HleGfxChanged(UINT /*Code*/, int id, HWND /*ctl*/)
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

void COptionPluginPage::HleAudioChanged(UINT /*Code*/, int id, HWND /*ctl*/)
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
