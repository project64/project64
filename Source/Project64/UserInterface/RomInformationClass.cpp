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
#include "RomInformationClass.h"

RomInformation::RomInformation(const char * RomFile) :
m_DeleteRomInfo(true),
m_FileName(RomFile ? RomFile : ""),
m_pRomInfo(NULL)
{
    if (m_FileName.length() == 0)  { return; }
    m_pRomInfo = new CN64Rom;
    if (!m_pRomInfo->LoadN64Image(m_FileName.c_str())) {
        delete m_pRomInfo;
        m_pRomInfo = NULL;
        return;
    }
}

RomInformation::RomInformation(CN64Rom * RomInfo) :
m_DeleteRomInfo(false),
m_FileName(RomInfo ? RomInfo->GetFileName().c_str() : ""),
m_pRomInfo(RomInfo)
{
}

RomInformation::~RomInformation()
{
    if (m_DeleteRomInfo)
        delete m_pRomInfo;
}

#include <windows.h>
void RomInformation::DisplayInformation(HWND hParent) const {
    if (m_FileName.length() == 0) { return; }

    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Rom_Information),
        (HWND)hParent, (DLGPROC)RomInfoProc, (DWORD)this);
}

DWORD CALLBACK RomInfoProc(HWND hDlg, DWORD uMsg, DWORD wParam, DWORD lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
    {
        //record class for future usage
        SetProp(hDlg, "this", (RomInformation *)lParam);
        RomInformation * _this = (RomInformation *)lParam;

        LONG_PTR originalWndProc = GetWindowLongPtrW(hDlg, GWLP_WNDPROC);
        SetWindowLongPtrW(hDlg, GWLP_WNDPROC, (LONG_PTR)DefWindowProcW);
        SetWindowTextW(hDlg, wGS(INFO_TITLE).c_str());
        SetWindowLongPtrW(hDlg, GWLP_WNDPROC, originalWndProc);

        SetDlgItemTextW(hDlg, IDC_ROM_NAME, wGS(INFO_ROM_NAME_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_FILE_NAME, wGS(INFO_FILE_NAME_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_LOCATION, wGS(INFO_LOCATION_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_ROM_MD5, wGS(INFO_MD5_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_ROM_SIZE, wGS(INFO_SIZE_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CART_ID, wGS(INFO_CART_ID_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_MANUFACTURER, wGS(INFO_MANUFACTURER_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_COUNTRY, wGS(INFO_COUNTRY_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CRC1, wGS(INFO_CRC1_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CRC2, wGS(INFO_CRC2_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CIC_CHIP, wGS(INFO_CIC_CHIP_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CLOSE_BUTTON, wGS(BOTTOM_CLOSE).c_str());

        SetDlgItemTextW(hDlg, IDC_INFO_ROMNAME, _this->m_pRomInfo->GetRomName().ToUTF16(stdstr::CODEPAGE_932).c_str());

        char path[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
        _splitpath(_this->m_pRomInfo->GetFileName().c_str(), drive, dir, fname, ext);
        _makepath(path, drive, dir, "", "");

        SetDlgItemText(hDlg, IDC_INFO_FILENAME, stdstr_f("%s%s", fname, ext).c_str());
        SetDlgItemText(hDlg, IDC_INFO_LOCATION, path);

        SetDlgItemText(hDlg, IDC_INFO_MD5, _this->m_pRomInfo->GetRomMD5().c_str());

        char String[255] = " ";
        sprintf(&String[1], "%.1f MBit", (float)_this->m_pRomInfo->GetRomSize() / 0x20000);
        SetDlgItemText(hDlg, IDC_INFO_ROMSIZE, String);

        BYTE * RomHeader = _this->m_pRomInfo->GetRomAddress();
        String[1] = RomHeader[0x3F];
        String[2] = RomHeader[0x3E];
        String[3] = '\0';
        SetDlgItemText(hDlg, IDC_INFO_CARTID, String);

        switch (RomHeader[0x38]) {
        case 'N': SetDlgItemText(hDlg, IDC_INFO_MANUFACTURER, " Nintendo"); break;
        case 0:   SetDlgItemText(hDlg, IDC_INFO_MANUFACTURER, " None"); break;
        default:  SetDlgItemText(hDlg, IDC_INFO_MANUFACTURER, " (Unknown)"); break;
        }

        switch (RomHeader[0x3D]) {
        case NTSC_BETA: SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Beta"); break;
        case X_NTSC:    SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " NTSC"); break;
        case Germany:   SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Germany"); break;
        case USA:       SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " America"); break;
        case french:    SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " France"); break;
        case Italian:   SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Italy"); break;
        case Japan:     SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Japan"); break;
        case Europe:    SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Europe"); break;
        case Spanish:   SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Spain"); break;
        case Australia: SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " Australia"); break;
        case X_PAL:     SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " PAL"); break;
        case Y_PAL:     SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " PAL"); break;
        case 0: SetDlgItemText(hDlg, IDC_INFO_COUNTRY, " None"); break;
        default:
            sprintf(&String[1], " Unknown %c (%02X)", RomHeader[0x3D], RomHeader[0x3D]);
            SetDlgItemText(hDlg, IDC_INFO_COUNTRY, String);
        }

        sprintf(&String[1], "0x%08X", *(DWORD *)(RomHeader + 0x10));
        SetDlgItemText(hDlg, IDC_INFO_CRC1, String);

        sprintf(&String[1], "0x%08X", *(DWORD *)(RomHeader + 0x14));
        SetDlgItemText(hDlg, IDC_INFO_CRC2, String);

        if (_this->m_pRomInfo->CicChipID() == CIC_UNKNOWN) {
            sprintf(&String[1], "Unknown");
        }
        else if (_this->m_pRomInfo->CicChipID() == CIC_NUS_8303) {
            sprintf(&String[1], "CIC-NUS-8303");
        }
        else if (_this->m_pRomInfo->CicChipID() == CIC_NUS_5167) {
            sprintf(&String[1], "CIC-NUS-5167");
        }
        else {
            sprintf(&String[1], "CIC-NUS-610%d", _this->m_pRomInfo->CicChipID());
        }
        SetDlgItemText(hDlg, IDC_INFO_CIC, String);
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            RemoveProp(hDlg, "this");
            EndDialog(hDlg, 0);
            break;
        case IDC_CLOSE_BUTTON:
            RemoveProp(hDlg, "this");
            EndDialog(hDlg, 0);
            break;
        }
    default:
        return false;
    }
    return true;
}