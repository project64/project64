#include "../../User Interface.h"
#include "Settings Page.h"

COptionPluginPage::COptionPluginPage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}
	
	//Set the text for all gui Items
	SetDlgItemText(RSP_ABOUT,GS(PLUG_ABOUT));
	SetDlgItemText(GFX_ABOUT,GS(PLUG_ABOUT));
	SetDlgItemText(AUDIO_ABOUT,GS(PLUG_ABOUT));
	SetDlgItemText(CONT_ABOUT,GS(PLUG_ABOUT));

	SetDlgItemText(IDC_RSP_NAME,GS(PLUG_RSP));
	SetDlgItemText(IDC_GFX_NAME,GS(PLUG_GFX));
	SetDlgItemText(IDC_AUDIO_NAME,GS(PLUG_AUDIO));
	SetDlgItemText(IDC_CONT_NAME,GS(PLUG_CTRL));		
	
	SetDlgItemText(IDC_HLE_GFX,GS(PLUG_HLE_GFX));
	SetDlgItemText(IDC_HLE_AUDIO,GS(PLUG_HLE_AUDIO));		

	m_GfxGroup.Attach(GetDlgItem(IDC_GFX_NAME));
	m_AudioGroup.Attach(GetDlgItem(IDC_AUDIO_NAME));
	m_ControlGroup.Attach(GetDlgItem(IDC_CONT_NAME));
	m_RspGroup.Attach(GetDlgItem(IDC_RSP_NAME));

	AddPlugins(GFX_LIST,Plugin_GFX_Current,PLUGIN_TYPE_GFX);
	AddPlugins(AUDIO_LIST,Plugin_AUDIO_Current,PLUGIN_TYPE_AUDIO);
	AddPlugins(CONT_LIST,Plugin_CONT_Current,PLUGIN_TYPE_CONTROLLER);
	AddPlugins(RSP_LIST,Plugin_RSP_Current,PLUGIN_TYPE_RSP);

	AddModCheckBox(GetDlgItem(IDC_HLE_GFX),Plugin_UseHleGfx);
	AddModCheckBox(GetDlgItem(IDC_HLE_AUDIO),Plugin_UseHleAudio);
	
	UpdatePageSettings();
}

void COptionPluginPage::AddPlugins (int ListId,SettingID Type, PLUGIN_TYPE PluginType )
{
	stdstr Default = _Settings->LoadString(Type);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(ListId),Type);
	for (int i = 0, n = m_PluginList.GetPluginCount(); i < n; i++ )
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
		if (_stricmp(Default.c_str(),Plugin->FileName.c_str()) == 0)
		{
			ComboBox->SetDefault((WPARAM)Plugin);
		}
		ComboBox->AddItem(Plugin->Info.Name, (WPARAM)Plugin);
	}
}

void COptionPluginPage::ShowAboutButton ( int id )
{
	CModifiedComboBox * ComboBox = NULL;
	for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
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
	
	const CPluginList::PLUGIN * Plugin = (const CPluginList::PLUGIN *)ComboBox->GetItemDataPtr(index);
	if (Plugin == NULL)
	{
		return;
	}
	
	//Load the plugin
	UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	HMODULE hLib = LoadLibrary(Plugin->FullPath);		
	SetErrorMode(LastErrorMode);
	if (hLib == NULL)
	{
		return;
	}
	
	//Get DLL about
	void (__cdecl *DllAbout) ( HWND hWnd );
	DllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
	
	//call the function from the dll
	DllAbout(m_hWnd);

	FreeLibrary(hLib);
}

void COptionPluginPage::PluginItemChanged ( int id, int AboutID, bool bSetChanged )
{
	CModifiedComboBox * ComboBox = NULL;
	for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
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
	const CPluginList::PLUGIN * Plugin = (const CPluginList::PLUGIN *)ComboBox->GetItemDataPtr(index);
	if (Plugin)
	{
		::EnableWindow(GetDlgItem(AboutID),Plugin->AboutFunction);
	}
	if (bSetChanged)
	{
		ComboBox->SetChanged(true);
		SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
	}
}

void COptionPluginPage::UpdatePageSettings ( void )
{
	UpdateCheckBoxes();
	for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
	{
		CModifiedComboBox * ComboBox = cb_iter->second;
		stdstr SelectedValue;
		
		ComboBox->SetChanged(_Settings->LoadString(cb_iter->first,SelectedValue));
		for (int i = 0, n = m_PluginList.GetPluginCount(); i < n; i++ )
		{
			const CPluginList::PLUGIN * Plugin = m_PluginList.GetPluginInfo(i);
			if (Plugin == NULL)
			{
				continue;
			}
			if (_stricmp(SelectedValue.c_str(),Plugin->FileName.c_str()) != 0)
			{
				continue;
			}
			ComboBox->SetDefault((WPARAM)Plugin);
		}
	}
	PluginItemChanged(GFX_LIST,GFX_ABOUT,false);
	PluginItemChanged(AUDIO_LIST,AUDIO_ABOUT,false);
	PluginItemChanged(CONT_LIST,CONT_ABOUT,false);
	PluginItemChanged(RSP_LIST,RSP_ABOUT,false);
}

void COptionPluginPage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void COptionPluginPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void COptionPluginPage::ApplySettings( bool UpdateScreen )
{
	CSettingsPageImpl<COptionPluginPage>::ApplySettings(UpdateScreen);
}

bool COptionPluginPage::EnableReset ( void )
{
	if (CSettingsPageImpl<COptionPluginPage>::EnableReset()) { return true; }
	return false;
}

void COptionPluginPage::ResetPage()
{
	CSettingsPageImpl<COptionPluginPage>::ResetPage();
}

void COptionPluginPage::ApplyComboBoxes ( void )
{
	for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
	{
		CModifiedComboBox * ComboBox = cb_iter->second;
		if (ComboBox->IsChanged())
		{
			int index = ComboBox->GetCurSel();
			if (index == CB_ERR) 
			{
				return; 
			}
			const CPluginList::PLUGIN * Plugin = (const CPluginList::PLUGIN *)ComboBox->GetItemDataPtr(index);

			_Settings->SaveString(cb_iter->first,Plugin->FileName.c_str());
			switch (Plugin->Info.Type)
			{
			case PLUGIN_TYPE_RSP:        _Settings->SaveBool(Plugin_RSP_Changed,true); break;
			case PLUGIN_TYPE_GFX:        _Settings->SaveBool(Plugin_GFX_Changed,true); break;
			case PLUGIN_TYPE_AUDIO:      _Settings->SaveBool(Plugin_AUDIO_Changed,true); break;
			case PLUGIN_TYPE_CONTROLLER: _Settings->SaveBool(Plugin_CONT_Changed,true); break;
			default:
				Notify().BreakPoint(__FILE__,__LINE__);
			}
		}
		if (ComboBox->IsReset())
		{
			_Settings->DeleteSetting(cb_iter->first);
			ComboBox->SetReset(false);
		}
	}
}

bool COptionPluginPage::ResetComboBox ( CModifiedComboBox & ComboBox, SettingID Type )
{
	if (!ComboBox.IsChanged())
	{
		return false;
	}

	ComboBox.SetReset(true);
	stdstr Value = _Settings->LoadDefaultString(Type);
	for (int i = 0, n = ComboBox.GetCount(); i < n; i++)
	{
		const CPluginList::PLUGIN * Plugin = (const CPluginList::PLUGIN *)ComboBox.GetItemDataPtr(i);
		if (Plugin->FileName != Value)
		{
			continue;
		}
		ComboBox.SetCurSel(i);
		return true;
	}
	return false;
}

void COptionPluginPage::HleGfxChanged ( UINT Code, int id, HWND ctl )
{
	for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
	{
		CModifiedButton * Button = iter->second;
		if ((int)Button->GetMenu() != id)
		{
			continue;
		}			
		if ((Button->GetCheck() & BST_CHECKED) == 0)
		{
			int res = MessageBox(GS(MSG_SET_LLE_GFX_MSG),GS(MSG_SET_LLE_GFX_TITLE),MB_YESNO|MB_ICONWARNING);
			if (res != IDYES)
			{
				Button->SetCheck(BST_CHECKED);
				return;
			}
		}
		Button->SetChanged(true);
		SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
		break;
	}
}

void COptionPluginPage::HleAudioChanged ( UINT Code, int id, HWND ctl )
{
	for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
	{
		CModifiedButton * Button = iter->second;
		if ((int)Button->GetMenu() != id)
		{
			continue;
		}			
		if ((Button->GetCheck() & BST_CHECKED) != 0)
		{
			int res = MessageBox(GS(MSG_SET_HLE_AUD_MSG),GS(MSG_SET_HLE_AUD_TITLE),MB_ICONWARNING|MB_YESNO);
			if (res != IDYES)
			{
				Button->SetCheck(BST_UNCHECKED);
				return;
			}
		}
		Button->SetChanged(true);
		SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
		break;
	}
}
