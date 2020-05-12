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
#include <Project64-core/N64System/N64DiskClass.h>

RomInformation::RomInformation(const char * RomFile) :
    m_DeleteRomInfo(true),
    m_DeleteDiskInfo(true),
    m_FileName(RomFile ? RomFile : ""),
    m_pRomInfo(NULL),
    m_pDiskInfo(NULL)
{
    if (m_FileName.length() == 0)  { return; }
    if ((CPath(m_FileName).GetExtension() != "ndd") && (CPath(m_FileName).GetExtension() != "d64"))
    {
        m_pRomInfo = new CN64Rom;
        if (!m_pRomInfo->LoadN64Image(m_FileName.c_str()))
        {
            delete m_pRomInfo;
            m_pRomInfo = NULL;
            return;
        }
    }
    else
    {
        m_pDiskInfo = new CN64Disk;
        if (!m_pDiskInfo->LoadDiskImage(m_FileName.c_str()))
        {
            delete m_pDiskInfo;
            m_pDiskInfo = NULL;
            return;
        }
    }
}

RomInformation::RomInformation(CN64Rom * RomInfo) :
    m_DeleteRomInfo(false),
    m_DeleteDiskInfo(false),
    m_FileName(RomInfo ? RomInfo->GetFileName().c_str() : ""),
    m_pRomInfo(RomInfo),
    m_pDiskInfo(NULL)
{
}

RomInformation::RomInformation(CN64Disk * DiskInfo) :
    m_DeleteRomInfo(false),
    m_DeleteDiskInfo(false),
    m_FileName(DiskInfo ? DiskInfo->GetFileName().c_str() : ""),
    m_pRomInfo(NULL),
    m_pDiskInfo(DiskInfo)
{
}

RomInformation::~RomInformation()
{
    if (m_DeleteRomInfo)
        delete m_pRomInfo;
    if (m_DeleteDiskInfo)
        delete m_pDiskInfo;
}

#include <windows.h>
void RomInformation::DisplayInformation(HWND hParent) const
{
    if (m_FileName.length() == 0) { return; }

    DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Rom_Information), hParent, (DLGPROC)RomInfoProc, (DWORD)this);
}

DWORD CALLBACK RomInfoProc(HWND hDlg, DWORD uMsg, DWORD wParam, DWORD lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        //record class for future usage
        SetProp(hDlg, L"this", (RomInformation *)lParam);
        RomInformation * _this = (RomInformation *)lParam;

        if (_this->m_pDiskInfo == NULL)
        {
            SetWindowTextW(hDlg, wGS(INFO_TITLE).c_str());

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

            SetDlgItemTextW(hDlg, IDC_INFO_FILENAME, stdstr(CPath(_this->m_pRomInfo->GetFileName()).GetNameExtension()).ToUTF16(CP_ACP).c_str());
            SetDlgItemTextW(hDlg, IDC_INFO_LOCATION, stdstr(CPath(_this->m_pRomInfo->GetFileName()).GetDriveDirectory()).ToUTF16(CP_ACP).c_str());

            SetDlgItemTextW(hDlg, IDC_INFO_MD5, _this->m_pRomInfo->GetRomMD5().ToUTF16().c_str());
            SetDlgItemTextW(hDlg, IDC_INFO_ROMSIZE, stdstr_f("%.1f MBit", (float)_this->m_pRomInfo->GetRomSize() / 0x20000).ToUTF16().c_str());

            BYTE * RomHeader = _this->m_pRomInfo->GetRomAddress();
            SetDlgItemTextW(hDlg, IDC_INFO_CARTID, stdstr_f("%c%c", RomHeader[0x3F], RomHeader[0x3E]).ToUTF16().c_str());

            switch (RomHeader[0x38])
            {
            case 'N': SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"Nintendo"); break;
            case 0:   SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"None"); break;
            default:  SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"(Unknown)"); break;
            }

            switch (RomHeader[0x3D])
            {
            case NTSC_BETA: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Beta"); break;
            case X_NTSC:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"NTSC"); break;
            case Germany:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Germany"); break;
            case USA:       SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"America"); break;
            case french:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"France"); break;
            case Italian:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Italy"); break;
            case Japan:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Japan"); break;
            case Europe:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Europe"); break;
            case Spanish:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Spain"); break;
            case Australia: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Australia"); break;
            case X_PAL:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"PAL"); break;
            case Y_PAL:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"PAL"); break;
            case 0: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"None"); break;
            default:
                SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, stdstr_f(" Unknown %c (%02X)", RomHeader[0x3D], RomHeader[0x3D]).ToUTF16().c_str());
            }

            switch (_this->m_pRomInfo->CicChipID())
            {
            case CIC_NUS_8303:
            case CIC_NUS_DDUS:
            case CIC_NUS_DDTL:
                SetDlgItemTextW(hDlg, IDC_INFO_CRC1, stdstr_f("0x%08X", (*(uint16_t *)(RomHeader + 0x608) << 16) | *(uint16_t *)(RomHeader + 0x60C)).ToUTF16().c_str());
                SetDlgItemTextW(hDlg, IDC_INFO_CRC2, stdstr_f("0x%08X", (*(uint16_t *)(RomHeader + 0x638) << 16) | *(uint16_t *)(RomHeader + 0x63C)).ToUTF16().c_str());
                break;
            default:
                SetDlgItemTextW(hDlg, IDC_INFO_CRC1, stdstr_f("0x%08X", *(uint32_t *)(RomHeader + 0x10)).ToUTF16().c_str());
                SetDlgItemTextW(hDlg, IDC_INFO_CRC2, stdstr_f("0x%08X", *(DWORD *)(RomHeader + 0x14)).ToUTF16().c_str());
                break;
            }

            std::wstring CicChip;
            switch (_this->m_pRomInfo->CicChipID())
            {
            case CIC_UNKNOWN: CicChip = L"Unknown"; break;
            case CIC_NUS_8303: CicChip = L"CIC-NUS-8303"; break;
            case CIC_NUS_5167: CicChip = L"CIC-NUS-5167"; break;
            case CIC_NUS_DDUS: CicChip = L"CIC-NUS-????"; break;
            case CIC_NUS_DDTL: CicChip = L"CIC-NUS-????"; break;
            default: CicChip = stdstr_f("CIC-NUS-610%d", _this->m_pRomInfo->CicChipID()).ToUTF16(); break;
            }
            SetDlgItemTextW(hDlg, IDC_INFO_CIC, CicChip.c_str());
        }
        else
        {
            SetWindowTextW(hDlg, wGS(INFO_TITLE).c_str());

            SetDlgItemTextW(hDlg, IDC_ROM_NAME, wGS(INFO_ROM_NAME_TEXT).c_str());
            SetDlgItemTextW(hDlg, IDC_FILE_NAME, wGS(INFO_FILE_NAME_TEXT).c_str());
            SetDlgItemTextW(hDlg, IDC_LOCATION, wGS(INFO_LOCATION_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_ROM_MD5, wGS(INFO_MD5_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_ROM_SIZE, wGS(INFO_SIZE_TEXT).c_str());
            SetDlgItemTextW(hDlg, IDC_CART_ID, wGS(INFO_CART_ID_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_MANUFACTURER, wGS(INFO_MANUFACTURER_TEXT).c_str());
            SetDlgItemTextW(hDlg, IDC_COUNTRY, wGS(INFO_COUNTRY_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_CRC1, wGS(INFO_CRC1_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_CRC2, wGS(INFO_CRC2_TEXT).c_str());
            //SetDlgItemTextW(hDlg, IDC_CIC_CHIP, wGS(INFO_CIC_CHIP_TEXT).c_str());
            SetDlgItemTextW(hDlg, IDC_CLOSE_BUTTON, wGS(BOTTOM_CLOSE).c_str());

            SetDlgItemTextW(hDlg, IDC_INFO_ROMNAME, _this->m_pDiskInfo->GetRomName().ToUTF16(stdstr::CODEPAGE_932).c_str());

            SetDlgItemTextW(hDlg, IDC_INFO_FILENAME, stdstr(CPath(_this->m_pDiskInfo->GetFileName()).GetNameExtension()).ToUTF16(CP_ACP).c_str());
            SetDlgItemTextW(hDlg, IDC_INFO_LOCATION, stdstr(CPath(_this->m_pDiskInfo->GetFileName()).GetDriveDirectory()).ToUTF16(CP_ACP).c_str());

            //SetDlgItemTextW(hDlg, IDC_INFO_MD5, _this->m_pRomInfo->GetRomMD5().ToUTF16().c_str());
            //SetDlgItemTextW(hDlg, IDC_INFO_ROMSIZE, stdstr_f("%.1f MBit", (float)_this->m_pDiskInfo->GetRomSize() / 0x20000).ToUTF16().c_str());

            BYTE * DiskHeader = _this->m_pDiskInfo->GetDiskAddressID();
            SetDlgItemTextW(hDlg, IDC_INFO_CARTID, stdstr_f("%c%c", DiskHeader[0x02], DiskHeader[0x01]).ToUTF16().c_str());

            /*switch (DiskHeader[0x00])
            {
            case 'N': SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"Nintendo"); break;
            case 0:   SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"None"); break;
            default:  SetDlgItemTextW(hDlg, IDC_INFO_MANUFACTURER, L"(Unknown)"); break;
            }*/

            switch (DiskHeader[0x00])
            {
            case NTSC_BETA: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Beta"); break;
            case X_NTSC:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"NTSC"); break;
            case Germany:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Germany"); break;
            case USA:       SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"America"); break;
            case french:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"France"); break;
            case Italian:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Italy"); break;
            case Japan:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Japan"); break;
            case Europe:    SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Europe"); break;
            case Spanish:   SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Spain"); break;
            case Australia: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"Australia"); break;
            case X_PAL:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"PAL"); break;
            case Y_PAL:     SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"PAL"); break;
            case 0: SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, L"None"); break;
            default:
                SetDlgItemTextW(hDlg, IDC_INFO_COUNTRY, stdstr_f(" Unknown %c (%02X)", DiskHeader[0x03], DiskHeader[0x03]).ToUTF16().c_str());
            }
            SetDlgItemTextW(hDlg, IDC_INFO_CRC1, stdstr_f("0x%08X", (_this->m_pDiskInfo->CalculateCrc())).ToUTF16().c_str());
            SetDlgItemTextW(hDlg, IDC_INFO_CRC2, stdstr_f("0x%08X", (~_this->m_pDiskInfo->CalculateCrc())).ToUTF16().c_str());
            /*
            std::wstring CicChip;
            switch (_this->m_pRomInfo->CicChipID())
            {
            case CIC_UNKNOWN: CicChip = L"Unknown"; break;
            case CIC_NUS_8303: CicChip = L"CIC-NUS-8303"; break;
            case CIC_NUS_5167: CicChip = L"CIC-NUS-5167"; break;
            case CIC_NUS_DDUS: CicChip = L"CIC-NUS-????"; break;
            default: CicChip = stdstr_f("CIC-NUS-610%d", _this->m_pRomInfo->CicChipID()).ToUTF16(); break;
            }
            SetDlgItemTextW(hDlg, IDC_INFO_CIC, CicChip.c_str());
            */
        }
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            RemoveProp(hDlg, L"this");
            EndDialog(hDlg, 0);
            break;
        case IDC_CLOSE_BUTTON:
            RemoveProp(hDlg, L"this");
            EndDialog(hDlg, 0);
            break;
        }
    default:
        return false;
    }
    return true;
}
