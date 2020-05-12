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
#include <Project64-core/Logging.h>
#include <Project64-core/Settings/LoggingSettings.h>
#include <prsht.h>

LRESULT CALLBACK LogGeneralProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LogPifProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LogRegProc(HWND, UINT, WPARAM, LPARAM);

void EnterLogOptions(HWND hwndOwner)
{
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;
    CLogSettings logSettings;

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandle(NULL);
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_Logging_Registers);
    psp[0].pfnDlgProc = (DLGPROC)LogRegProc;
    psp[0].pszTitle = L"Registers";
    psp[0].lParam = (LPARAM)&logSettings;
    psp[0].pfnCallback = NULL;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = GetModuleHandle(NULL);
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_Logging_PifRam);
    psp[1].pfnDlgProc = (DLGPROC)LogPifProc;
    psp[1].pszTitle = L"Pif Ram";
    psp[1].lParam = (LPARAM)&logSettings;
    psp[1].pfnCallback = NULL;

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = GetModuleHandle(NULL);
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_Logging_General);
    psp[2].pfnDlgProc = (DLGPROC)LogGeneralProc;
    psp[2].pszTitle = L"General";
    psp[2].lParam = (LPARAM)&logSettings;
    psp[2].pfnCallback = NULL;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszCaption = (LPTSTR)L"Log Options";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);
    return;
}

LRESULT CALLBACK LogGeneralProc(HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (CLogSettings::LogCP0changes())   { CheckDlgButton(hDlg, IDC_CP0_WRITE, BST_CHECKED); }
        if (CLogSettings::LogCP0reads())     { CheckDlgButton(hDlg, IDC_CP0_READ, BST_CHECKED); }
        if (CLogSettings::LogCache())        { CheckDlgButton(hDlg, IDC_CACHE, BST_CHECKED); }
        if (CLogSettings::LogExceptions())   { CheckDlgButton(hDlg, IDC_EXCEPTIONS, BST_CHECKED); }
        if (CLogSettings::LogNoInterrupts()) { CheckDlgButton(hDlg, IDC_INTERRUPTS, BST_CHECKED); }
        if (CLogSettings::LogTLB())          { CheckDlgButton(hDlg, IDC_TLB, BST_CHECKED); }
        if (CLogSettings::LogRomHeader())    { CheckDlgButton(hDlg, IDC_ROM_HEADER, BST_CHECKED); }
        if (CLogSettings::LogUnknown())      { CheckDlgButton(hDlg, IDC_UNKOWN, BST_CHECKED); }
    }
    break;
    case WM_NOTIFY:
        if (((NMHDR FAR *) lParam)->code != PSN_APPLY) { break; }
        g_Settings->SaveBool(Logging_LogCP0changes, IsDlgButtonChecked(hDlg, IDC_CP0_WRITE) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogCP0reads, IsDlgButtonChecked(hDlg, IDC_CP0_READ) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogCache, IsDlgButtonChecked(hDlg, IDC_CACHE) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogExceptions, IsDlgButtonChecked(hDlg, IDC_EXCEPTIONS) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_NoInterrupts, IsDlgButtonChecked(hDlg, IDC_INTERRUPTS) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogTLB, IsDlgButtonChecked(hDlg, IDC_TLB) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogRomHeader, IsDlgButtonChecked(hDlg, IDC_ROM_HEADER) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogUnknown, IsDlgButtonChecked(hDlg, IDC_UNKOWN) == BST_CHECKED ? true : false);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK LogPifProc(HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (CLogSettings::LogPRDMAOperations())   { CheckDlgButton(hDlg, IDC_SI_DMA, BST_CHECKED); }
        if (CLogSettings::LogPRDirectMemLoads())  { CheckDlgButton(hDlg, IDC_DIRECT_WRITE, BST_CHECKED); }
        if (CLogSettings::LogPRDMAMemLoads())     { CheckDlgButton(hDlg, IDC_DMA_WRITE, BST_CHECKED); }
        if (CLogSettings::LogPRDirectMemStores()) { CheckDlgButton(hDlg, IDC_DIRECT_READ, BST_CHECKED); }
        if (CLogSettings::LogPRDMAMemStores())    { CheckDlgButton(hDlg, IDC_DMA_READ, BST_CHECKED); }
        if (CLogSettings::LogControllerPak())     { CheckDlgButton(hDlg, IDC_CONT_PAK, BST_CHECKED); }
    }
    break;
    case WM_NOTIFY:
        if (((NMHDR FAR *) lParam)->code != PSN_APPLY)
        {
            break;
        }
        g_Settings->SaveBool(Logging_LogPRDMAOperations, IsDlgButtonChecked(hDlg, IDC_SI_DMA) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogPRDirectMemLoads, IsDlgButtonChecked(hDlg, IDC_DIRECT_WRITE) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogPRDMAMemLoads, IsDlgButtonChecked(hDlg, IDC_DMA_WRITE) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogPRDirectMemStores, IsDlgButtonChecked(hDlg, IDC_DIRECT_READ) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogPRDMAMemStores, IsDlgButtonChecked(hDlg, IDC_DMA_READ) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogControllerPak, IsDlgButtonChecked(hDlg, IDC_CONT_PAK) == BST_CHECKED ? true : false);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK LogRegProc(HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (CLogSettings::LogRDRamRegisters())  { CheckDlgButton(hDlg, IDC_RDRAM, BST_CHECKED); }
        if (CLogSettings::LogSPRegisters())     { CheckDlgButton(hDlg, IDC_SP_REG, BST_CHECKED); }
        if (CLogSettings::LogDPCRegisters())    { CheckDlgButton(hDlg, IDC_DPC_REG, BST_CHECKED); }
        if (CLogSettings::LogDPSRegisters())    { CheckDlgButton(hDlg, IDC_DPS_REG, BST_CHECKED); }
        if (CLogSettings::LogMIPSInterface())   { CheckDlgButton(hDlg, IDC_MI_REG, BST_CHECKED); }
        if (CLogSettings::LogVideoInterface())  { CheckDlgButton(hDlg, IDC_VI_REG, BST_CHECKED); }
        if (CLogSettings::LogAudioInterface())  { CheckDlgButton(hDlg, IDC_AI_REG, BST_CHECKED); }
        if (CLogSettings::LogPerInterface())    { CheckDlgButton(hDlg, IDC_PI_REG, BST_CHECKED); }
        if (CLogSettings::LogRDRAMInterface())  { CheckDlgButton(hDlg, IDC_RI_REG, BST_CHECKED); }
        if (CLogSettings::LogSerialInterface()) { CheckDlgButton(hDlg, IDC_SI_REG, BST_CHECKED); }
    }
    break;
    case WM_NOTIFY:
        if (((NMHDR FAR *) lParam)->code != PSN_APPLY)
        {
            break;
        }
        g_Settings->SaveBool(Logging_LogRDRamRegisters, IsDlgButtonChecked(hDlg, IDC_RDRAM) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogSPRegisters, IsDlgButtonChecked(hDlg, IDC_SP_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogDPCRegisters, IsDlgButtonChecked(hDlg, IDC_DPC_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogDPSRegisters, IsDlgButtonChecked(hDlg, IDC_DPS_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogMIPSInterface, IsDlgButtonChecked(hDlg, IDC_MI_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogVideoInterface, IsDlgButtonChecked(hDlg, IDC_VI_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogAudioInterface, IsDlgButtonChecked(hDlg, IDC_AI_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogPerInterface, IsDlgButtonChecked(hDlg, IDC_PI_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogRDRAMInterface, IsDlgButtonChecked(hDlg, IDC_RI_REG) == BST_CHECKED ? true : false);
        g_Settings->SaveBool(Logging_LogSerialInterface, IsDlgButtonChecked(hDlg, IDC_SI_REG) == BST_CHECKED ? true : false);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}