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

COptionsGameBrowserPage::COptionsGameBrowserPage(HWND hParent, const RECT & rcDispay) :
m_OrderChanged(false),
m_OrderReset(false)
{
	if (!Create(hParent, rcDispay))
	{
		return;
	}

	SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT2, GS(RB_ROMS));
	SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT4, GS(RB_DIRS));
	SetDlgItemTextW(m_hWnd, IDC_USE_ROMBROWSER, GS(RB_USE));
	SetDlgItemTextW(m_hWnd, IDC_RECURSION, GS(RB_DIR_RECURSION));
	SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT5, GS(RB_AVALIABLE_FIELDS));
	SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT6, GS(RB_SHOW_FIELDS));
	SetDlgItemTextW(m_hWnd, IDC_ADD, GS(RB_ADD));
	SetDlgItemTextW(m_hWnd, IDC_REMOVE, GS(RB_REMOVE));
	SetDlgItemTextW(m_hWnd, IDC_UP, GS(RB_UP));
	SetDlgItemTextW(m_hWnd, IDC_DOWN, GS(RB_DOWN));

	AddModCheckBox(GetDlgItem(IDC_USE_ROMBROWSER), RomBrowser_Enabled);
	AddModCheckBox(GetDlgItem(IDC_RECURSION), RomBrowser_Recursive);

	m_Avaliable.Attach(GetDlgItem(IDC_AVALIABLE));
	m_Using.Attach(GetDlgItem(IDC_USING));

	CRomBrowser::GetFieldInfo(m_Fields);

	UpdatePageSettings();
}

void COptionsGameBrowserPage::UpdateFieldList(const ROMBROWSER_FIELDS_LIST & Fields)
{
	m_Avaliable.ResetContent();
	m_Using.ResetContent();

	m_OrderChanged = false;
	for (int i = 0, n = Fields.size(); i < n; i++)
	{
		if (Fields[i].PosChanged())
		{
			m_OrderChanged = true;
		}
		int Pos = Fields[i].Pos();
		if (Pos < 0)
		{
			m_Avaliable.SetItemData(m_Avaliable.AddStringW(GS(Fields[i].LangID())), i);
			continue;
		}
		int listCount = m_Using.GetCount();
		if (Pos > listCount) { Pos = listCount; }
		m_Using.SetItemData(m_Using.InsertStringW(Pos, GS(Fields[i].LangID())), i);
	}
}

void COptionsGameBrowserPage::UpdatePageSettings(void)
{
	UpdateFieldList(m_Fields);
	CSettingsPageImpl<COptionsGameBrowserPage>::UpdatePageSettings();
	FixCtrlState();
}

void  COptionsGameBrowserPage::UseRomBrowserChanged(UINT Code, int id, HWND ctl)
{
	CheckBoxChanged(Code, id, ctl);
	FixCtrlState();
}

void  COptionsGameBrowserPage::FixCtrlState(void)
{
	bool bEnabled = (SendDlgItemMessage(IDC_USE_ROMBROWSER, BM_GETCHECK, 0, 0) == BST_CHECKED);
	::EnableWindow(GetDlgItem(IDC_ROMSEL_TEXT5), bEnabled);
	::EnableWindow(GetDlgItem(IDC_ROMSEL_TEXT6), bEnabled);
	::EnableWindow(GetDlgItem(IDC_AVALIABLE), bEnabled);
	::EnableWindow(GetDlgItem(IDC_ADD), bEnabled);
	::EnableWindow(GetDlgItem(IDC_REMOVE), bEnabled);
	::EnableWindow(GetDlgItem(IDC_USING), bEnabled);
	::EnableWindow(GetDlgItem(IDC_UP), bEnabled);
	::EnableWindow(GetDlgItem(IDC_DOWN), bEnabled);
	::EnableWindow(GetDlgItem(IDC_RECURSION), bEnabled);
}

void COptionsGameBrowserPage::AddFieldClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
	int index = m_Avaliable.GetCurSel();
	if (index < 0)
	{
		return;
	}
	//remove from list
	int i = m_Avaliable.GetItemData(index);
	m_Avaliable.DeleteString(index);

	//select next in list
	int listCount = m_Avaliable.GetCount();
	if (index >= listCount) { index -= 1; }
	m_Avaliable.SetCurSel(index);

	//Add to list
	index = m_Using.AddStringW(GS(m_Fields[i].LangID()));
	m_Using.SetItemData(index, i);
	m_Using.SetCurSel(index);

	m_OrderChanged = true;
	m_OrderReset = false;
	SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsGameBrowserPage::RemoveFieldClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
	int index = m_Using.GetCurSel();
	if (index < 0)
	{
		return;
	}
	//remove from list
	int i = m_Using.GetItemData(index);
	m_Using.DeleteString(index);

	//select next in list
	int listCount = m_Using.GetCount();
	if (index >= listCount) { index -= 1; }
	m_Using.SetCurSel(index);

	//Add to list
	index = m_Avaliable.AddStringW(GS(m_Fields[i].LangID()));
	m_Avaliable.SetItemData(index, i);
	m_Avaliable.SetCurSel(index);

	m_OrderChanged = true;
	m_OrderReset = false;
	SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsGameBrowserPage::MoveFieldUpClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
	int index = m_Using.GetCurSel();
	if (index <= 0)
	{
		return;
	}
	int i = m_Using.GetItemData(index);
	m_Using.DeleteString(index);

	index = m_Using.InsertStringW(index - 1, GS(m_Fields[i].LangID()));
	m_Using.SetItemData(index, i);
	m_Using.SetCurSel(index);

	m_OrderChanged = true;
	m_OrderReset = false;
	SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsGameBrowserPage::MoveFieldDownClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
	int index = m_Using.GetCurSel();
	if (index < 0 || index >= (m_Using.GetCount() - 1))
	{
		return;
	}
	int i = m_Using.GetItemData(index);
	m_Using.DeleteString(index);

	index = m_Using.InsertStringW(index + 1, GS(m_Fields[i].LangID()));
	m_Using.SetItemData(index, i);
	m_Using.SetCurSel(index);

	m_OrderChanged = true;
	m_OrderReset = false;
	SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsGameBrowserPage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void COptionsGameBrowserPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void COptionsGameBrowserPage::ApplySettings(bool UpdateScreen)
{
	bool bColChanged = false;
	if (m_OrderReset)
	{
		for (size_t i = 0; i < m_Fields.size(); i++)
		{
			m_Fields[i].ResetPos();
		}
		bColChanged = true;
	}
	else
	{
		size_t Item, listCount = m_Using.GetCount();
		for (Item = 0; Item < listCount; Item++)
		{
			int Pos = m_Using.GetItemData(Item);
			if (m_OrderReset || m_Fields[Pos].Pos() != Item)
			{
				m_Fields[Pos].SetColPos(Item);
				bColChanged = true;
			}
		}

		listCount = m_Avaliable.GetCount();
		for (Item = 0; Item < listCount; Item++)
		{
			int Pos = m_Avaliable.GetItemData(Item);
			if (m_OrderReset || m_Fields[Pos].Pos() != -1)
			{
				m_Fields[Pos].SetColPos(-1);
				bColChanged = true;
			}
		}
	}
	if (bColChanged)
	{
		g_Settings->SaveBool(RomBrowser_ColoumnsChanged, !g_Settings->LoadBool(RomBrowser_ColoumnsChanged));
	}

	CSettingsPageImpl<COptionsGameBrowserPage>::ApplySettings(UpdateScreen);
}

bool COptionsGameBrowserPage::EnableReset(void)
{
	if (m_OrderChanged) { return true; }
	return CSettingsPageImpl<COptionsGameBrowserPage>::EnableReset();
}

void COptionsGameBrowserPage::ResetPage()
{
	if (m_OrderChanged)
	{
		ROMBROWSER_FIELDS_LIST Fields;
		CRomBrowser::GetFieldInfo(Fields, true);
		UpdateFieldList(Fields);
		m_OrderReset = true;
		SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
	}
	CSettingsPageImpl<COptionsGameBrowserPage>::ResetPage();
}
