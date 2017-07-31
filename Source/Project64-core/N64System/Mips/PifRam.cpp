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
#include <stdio.h>

#include <Project64-core/N64System/Mips/PifRam.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/ControllerPlugin.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Mips/Transferpak.h>
#include <Project64-core/N64System/Mips/Rumblepak.h>
#include <Project64-core/N64System/Mips/Mempak.h>
#include <Project64-core/Logging.h>

int32_t CPifRamSettings::m_RefCount = 0;
bool CPifRamSettings::m_bShowPifRamErrors = false;

CPifRamSettings::CPifRamSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(Debugger_ShowPifErrors, NULL, RefreshSettings);
        RefreshSettings(NULL);
    }
}

CPifRamSettings::~CPifRamSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(Debugger_ShowPifErrors, NULL, RefreshSettings);
    }
}

void CPifRamSettings::RefreshSettings(void *)
{
    m_bShowPifRamErrors = g_Settings->LoadBool(Debugger_ShowPifErrors);
}

CPifRam::CPifRam(bool SavesReadOnly) :
CEeprom(SavesReadOnly)
{
    Reset();
}

CPifRam::~CPifRam()
{
}

void CPifRam::Reset()
{
    memset(m_PifRam, 0, sizeof(m_PifRam));
    memset(m_PifRom, 0, sizeof(m_PifRom));
}

void CPifRam::n64_cic_nus_6105(char challenge[], char respone[], int32_t length)
{
    static char lut0[0x10] = {
        0x4, 0x7, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
        0xC, 0xF, 0x8, 0xF, 0x6, 0x3, 0x6, 0x9
    };
    static char lut1[0x10] = {
        0x4, 0x1, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
        0xC, 0x9, 0x8, 0x5, 0x6, 0x3, 0xC, 0x9
    };
    char key, *lut;
    int32_t i, sgn, mag, mod;

    for (key = 0xB, lut = lut0, i = 0; i < length; i++)
    {
        respone[i] = (key + 5 * challenge[i]) & 0xF;
        key = lut[respone[i]];
        sgn = (respone[i] >> 3) & 0x1;
        mag = ((sgn == 1) ? ~respone[i] : respone[i]) & 0x7;
        mod = (mag % 3 == 1) ? sgn : 1 - sgn;
        if (lut == lut1 && (respone[i] == 0x1 || respone[i] == 0x9))
        {
            mod = 1;
        }
        if (lut == lut1 && (respone[i] == 0xB || respone[i] == 0xE))
        {
            mod = 0;
        }
        lut = (mod == 1) ? lut1 : lut0;
    }
}

void CPifRam::PifRamRead()
{
    if (m_PifRam[0x3F] == 0x2)
    {
        return;
    }

    CONTROL * Controllers = g_Plugins->Control()->PluginControllers();

    int32_t Channel = 0;
    for (int32_t CurPos = 0; CurPos < 0x40; CurPos++)
    {
        switch (m_PifRam[CurPos])
        {
        case 0x00:
            Channel += 1;
            if (Channel > 6)
            {
                CurPos = 0x40;
            }
            break;
        case 0xFD: CurPos = 0x40; break;
        case 0xFE: CurPos = 0x40; break;
        case 0xFF: break;
        case 0xB4: case 0x56: case 0xB8: break; /* ??? */
        default:
            if ((m_PifRam[CurPos] & 0xC0) == 0)
            {
                if (Channel < 4)
                {
                    if (Controllers[Channel].Present && Controllers[Channel].RawData)
                    {
                        if (g_Plugins->Control()->ReadController)
                        {
                            g_Plugins->Control()->ReadController(Channel, &m_PifRam[CurPos]);
                        }
                    }
                    else
                    {
                        ReadControllerCommand(Channel, &m_PifRam[CurPos]);
                    }
                }
                CurPos += m_PifRam[CurPos] + (m_PifRam[CurPos + 1] & 0x3F) + 1;
                Channel += 1;
            }
            else
            {
                if (CurPos != 0x27 && bShowPifRamErrors())
                {
                    g_Notify->DisplayError(stdstr_f("Unknown Command in PifRamRead(%X)", m_PifRam[CurPos]).c_str());
                }
                CurPos = 0x40;
            }
            break;
        }
    }
    if (g_Plugins->Control()->ReadController)
    {
        g_Plugins->Control()->ReadController(-1, NULL);
    }
}

void CPifRam::PifRamWrite()
{
    CONTROL * Controllers = g_Plugins->Control()->PluginControllers();
    int32_t Channel = 0, CurPos;

    if (m_PifRam[0x3F] > 0x1)
    {
        switch (m_PifRam[0x3F])
        {
        case 0x02:
            // format the 'challenge' message into 30 nibbles for X-Scale's CIC code
            {
                char Challenge[30], Response[30];
                for (int32_t i = 0; i < 15; i++)
                {
                    Challenge[i * 2] = (m_PifRam[48 + i] >> 4) & 0x0f;
                    Challenge[i * 2 + 1] = m_PifRam[48 + i] & 0x0f;
                }
                n64_cic_nus_6105(Challenge, Response, CHALLENGE_LENGTH - 2);
                uint64_t ResponseValue = 0;
                m_PifRam[46] = m_PifRam[47] = 0x00;
                for (int32_t z = 8; z > 0; z--)
                {
                    ResponseValue = (ResponseValue << 8) | ((Response[(z - 1) * 2] << 4) + Response[(z - 1) * 2 + 1]);
                }
                memcpy(&m_PifRam[48], &ResponseValue, sizeof(uint64_t));
                ResponseValue = 0;
                for (int32_t z = 7; z > 0; z--)
                {
                    ResponseValue = (ResponseValue << 8) | ((Response[((z + 8) - 1) * 2] << 4) + Response[((z + 8) - 1) * 2 + 1]);
                }
                memcpy(&m_PifRam[56], &ResponseValue, sizeof(uint64_t));
            }
            break;
        case 0x08:
            m_PifRam[0x3F] = 0;
            g_Reg->MI_INTR_REG |= MI_INTR_SI;
            g_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
            g_Reg->CheckInterrupts();
            break;
        case 0x10:
            memset(m_PifRom, 0, 0x7C0);
            break;
        case 0x30:
            m_PifRam[0x3F] = 0x80;
            break;
        case 0xC0:
            memset(m_PifRam, 0, 0x40);
            break;
        default:
            if (bShowPifRamErrors())
            {
                g_Notify->DisplayError(stdstr_f("Unkown PifRam control: %d", m_PifRam[0x3F]).c_str());
            }
        }
        return;
    }

    for (CurPos = 0; CurPos < 0x40; CurPos++)
    {
        switch (m_PifRam[CurPos])
        {
        case 0x00:
            Channel += 1;
            if (Channel > 6)
            {
                CurPos = 0x40;
            }
            break;
        case 0xFD: CurPos = 0x40; break;
        case 0xFE: CurPos = 0x40; break;
        case 0xFF: break;
        case 0xB4: case 0x56: case 0xB8: break; /* ??? */
        default:
            if ((m_PifRam[CurPos] & 0xC0) == 0)
            {
                if (Channel < 4)
                {
                    if (Controllers[Channel].Present && Controllers[Channel].RawData)
                    {
                        if (g_Plugins->Control()->ControllerCommand)
                        {
                            g_Plugins->Control()->ControllerCommand(Channel, &m_PifRam[CurPos]);
                        }
                    }
                    else
                    {
                        ProcessControllerCommand(Channel, &m_PifRam[CurPos]);
                    }
                }
                else if (Channel == 4)
                {
                    EepromCommand(&m_PifRam[CurPos]);
                }
                else
                {
                    if (bShowPifRamErrors())
                    {
                        g_Notify->DisplayError("Command on channel 5?");
                    }
                }
                CurPos += m_PifRam[CurPos] + (m_PifRam[CurPos + 1] & 0x3F) + 1;
                Channel += 1;
            }
            else
            {
                if (CurPos != 0x27 && bShowPifRamErrors())
                {
                    g_Notify->DisplayError(stdstr_f("Unknown Command in PifRamWrite(%X)", m_PifRam[CurPos]).c_str());
                }
                CurPos = 0x40;
            }
            break;
        }
    }
    m_PifRam[0x3F] = 0;
    if (g_Plugins->Control()->ControllerCommand)
    {
        g_Plugins->Control()->ControllerCommand(-1, NULL);
    }
}

void CPifRam::SI_DMA_READ()
{
    uint8_t * PifRamPos = m_PifRam;
    uint8_t * RDRAM = g_MMU->Rdram();

    uint32_t & SI_DRAM_ADDR_REG = (uint32_t &)g_Reg->SI_DRAM_ADDR_REG;
    if ((int32_t)SI_DRAM_ADDR_REG > (int32_t)g_System->RdramSize())
    {
        if (bShowPifRamErrors())
        {
            g_Notify->DisplayError(stdstr_f("%s\nSI_DRAM_ADDR_REG not in RDRam space", __FUNCTION__).c_str());
        }
        return;
    }

    PifRamRead();
    SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
    if ((int32_t)SI_DRAM_ADDR_REG < 0)
    {
        int32_t count, RdramPos;

        RdramPos = (int32_t)SI_DRAM_ADDR_REG;
        for (count = 0; count < 0x40; count++, RdramPos++)
        {
            if (RdramPos < 0)
            {
                continue;
            }
            RDRAM[RdramPos ^ 3] = m_PifRam[count];
        }
    }
    else
    {
        for (size_t i = 0; i < 64; i++)
        {
            RDRAM[(SI_DRAM_ADDR_REG + i) ^ 3] = PifRamPos[i];
        }
    }

    if (LogPRDMAMemStores())
    {
        int32_t count;
        char HexData[100], AsciiData[100], Addon[20];
        LogMessage("\tData DMAed to RDRAM:");
        LogMessage("\t--------------------");
        for (count = 0; count < 16; count++)
        {
            if ((count % 4) == 0)
            {
                HexData[0] = '\0';
                AsciiData[0] = '\0';
            }
            sprintf(Addon, "%02X %02X %02X %02X",
                m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1],
                m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3]);
            strcat(HexData, Addon);
            if (((count + 1) % 4) != 0)
            {
                sprintf(Addon, "-");
                strcat(HexData, Addon);
            }

            sprintf(Addon, "%c%c%c%c",
                m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1],
                m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3]);
            strcat(AsciiData, Addon);

            if (((count + 1) % 4) == 0)
            {
                LogMessage("\t%s %s", HexData, AsciiData);
            }
        }
        LogMessage("");
    }

    if (g_System->bDelaySI())
    {
        g_SystemTimer->SetTimer(CSystemTimer::SiTimer, 0x900, false);
    }
    else
    {
        g_Reg->MI_INTR_REG |= MI_INTR_SI;
        g_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
        g_Reg->CheckInterrupts();
    }
}

void CPifRam::SI_DMA_WRITE()
{
    uint8_t * PifRamPos = m_PifRam;

    uint32_t & SI_DRAM_ADDR_REG = (uint32_t &)g_Reg->SI_DRAM_ADDR_REG;
    if ((int32_t)SI_DRAM_ADDR_REG > (int32_t)g_System->RdramSize())
    {
        if (bShowPifRamErrors())
        {
            g_Notify->DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
        }
        return;
    }

    SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
    uint8_t * RDRAM = g_MMU->Rdram();

    if ((int32_t)SI_DRAM_ADDR_REG < 0)
    {
        int32_t RdramPos = (int32_t)SI_DRAM_ADDR_REG;

        for (int32_t count = 0; count < 0x40; count++, RdramPos++)
        {
            if (RdramPos < 0)
            {
                m_PifRam[count] = 0; continue;
            }
            m_PifRam[count] = RDRAM[RdramPos ^ 3];
        }
    }
    else
    {
        for (size_t i = 0; i < 64; i++)
        {
            PifRamPos[i] = RDRAM[(SI_DRAM_ADDR_REG + i) ^ 3];
        }
    }

    if (LogPRDMAMemLoads())
    {
        int32_t count;
        char HexData[100], AsciiData[100], Addon[20];
        LogMessage("");
        LogMessage("\tData DMAed to the Pif Ram:");
        LogMessage("\t--------------------------");
        for (count = 0; count < 16; count++)
        {
            if ((count % 4) == 0)
            {
                HexData[0] = '\0';
                AsciiData[0] = '\0';
            }
            sprintf(Addon, "%02X %02X %02X %02X",
                m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1],
                m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3]);
            strcat(HexData, Addon);
            if (((count + 1) % 4) != 0)
            {
                sprintf(Addon, "-");
                strcat(HexData, Addon);
            }

            sprintf(Addon, "%c%c%c%c",
                m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1],
                m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3]);
            strcat(AsciiData, Addon);

            if (((count + 1) % 4) == 0)
            {
                LogMessage("\t%s %s", HexData, AsciiData);
            }
        }
        LogMessage("");
    }

    PifRamWrite();

    if (g_System->bDelaySI())
    {
        g_SystemTimer->SetTimer(CSystemTimer::SiTimer, 0x900, false);
    }
    else
    {
        g_Reg->MI_INTR_REG |= MI_INTR_SI;
        g_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
        g_Reg->CheckInterrupts();
    }
}

void CPifRam::ProcessControllerCommand(int32_t Control, uint8_t * Command)
{
    CONTROL * Controllers = g_Plugins->Control()->PluginControllers();

    switch (Command[2])
    {
    case 0x00: // check
    case 0xFF: // reset & check ?
        if ((Command[1] & 0x80) != 0)
        {
            break;
        }
        if (bShowPifRamErrors())
        {
            if (Command[0] != 1 || Command[1] != 3)
            {
                g_Notify->DisplayError("What am I meant to do with this Controller Command");
            }
        }
        if (Controllers[Control].Present != 0)
        {
            Command[3] = 0x05;
            Command[4] = 0x00;
            switch (Controllers[Control].Plugin)
            {
            case PLUGIN_TANSFER_PAK:
            case PLUGIN_RUMBLE_PAK:
            case PLUGIN_MEMPAK:
            case PLUGIN_RAW:
                Command[5] = 1; break;
            default: Command[5] = 0; break;
            }
        }
        else
        {
            Command[1] |= 0x80;
        }
        break;
    case 0x01: // read controller
        if (bShowPifRamErrors())
        {
            if (Command[0] != 1 || Command[1] != 4)
            {
                g_Notify->DisplayError("What am I meant to do with this Controller Command");
            }
        }
        if (Controllers[Control].Present == false)
        {
            Command[1] |= 0x80;
        }
        break;
    case 0x02: //read from controller pack
        if (LogControllerPak())
        {
            LogControllerPakData("Read: Before Gettting Results");
        }
        if (bShowPifRamErrors())
        {
            if (Command[0] != 3 || Command[1] != 33)
            {
                g_Notify->DisplayError("What am I meant to do with this Controller Command");
            }
        }
        if (Controllers[Control].Present != 0)
        {
            uint32_t address = (Command[3] << 8) | (Command[4] & 0xE0);
            uint8_t* data = &Command[5];

            switch (Controllers[Control].Plugin)
            {
            case PLUGIN_RUMBLE_PAK: Rumblepak::ReadFrom(address, data); break;
            case PLUGIN_MEMPAK: g_Mempak->ReadFrom(Control, address, data); break;
            case PLUGIN_TANSFER_PAK: Transferpak::ReadFrom(address, data); break;
            case PLUGIN_RAW: if (g_Plugins->Control()->ControllerCommand) { g_Plugins->Control()->ControllerCommand(Control, Command); } break;
            default:
                memset(&Command[5], 0, 0x20);
            }

            if (Controllers[Control].Plugin != PLUGIN_RAW)
            {
                Command[0x25] = CMempak::CalculateCrc(data);
            }
        }
        else
        {
            Command[1] |= 0x80;
        }
        if (LogControllerPak())
        {
            LogControllerPakData("Read: After Gettting Results");
        }
        break;
    case 0x03: //write controller pak
        if (LogControllerPak())
        {
            LogControllerPakData("Write: Before Processing");
        }
        if (bShowPifRamErrors())
        {
            if (Command[0] != 35 || Command[1] != 1)
            {
                g_Notify->DisplayError("What am I meant to do with this Controller Command");
            }
        }
        if (Controllers[Control].Present == true)
        {
            uint32_t address = (Command[3] << 8) | (Command[4] & 0xE0);
            uint8_t* data = &Command[5];

            switch (Controllers[Control].Plugin)
            {
            case PLUGIN_MEMPAK: g_Mempak->WriteTo(Control, address, data); break;
            case PLUGIN_RUMBLE_PAK: Rumblepak::WriteTo(Control, address, data); break;
            case PLUGIN_TANSFER_PAK: Transferpak::WriteTo(address, data); break;
            case PLUGIN_RAW: if (g_Plugins->Control()->ControllerCommand) { g_Plugins->Control()->ControllerCommand(Control, Command); } break;
            }

            if (Controllers[Control].Plugin != PLUGIN_RAW)
            {
                Command[0x25] = CMempak::CalculateCrc(data);
            }
        }
        else
        {
            Command[1] |= 0x80;
        }
        if (LogControllerPak())
        {
            LogControllerPakData("Write: After Processing");
        }
        break;
    default:
        if (bShowPifRamErrors())
        {
            g_Notify->DisplayError(stdstr_f("Unknown ControllerCommand %d", Command[2]).c_str());
        }
    }
}

void CPifRam::ReadControllerCommand(int32_t Control, uint8_t * Command)
{
    CONTROL * Controllers = g_Plugins->Control()->PluginControllers();

    switch (Command[2])
    {
    case 0x01: // read controller
        if (Controllers[Control].Present != 0)
        {
            if (bShowPifRamErrors())
            {
                if (Command[0] != 1 || Command[1] != 4) { g_Notify->DisplayError("What am I meant to do with this Controller Command"); }
            }

            const uint32_t buttons = g_BaseSystem->GetButtons(Control);
            memcpy(&Command[3], &buttons, sizeof(uint32_t));
        }
        break;
    case 0x02: //read from controller pack
        if (Controllers[Control].Present != 0)
        {
            switch (Controllers[Control].Plugin)
            {
            case PLUGIN_RAW: if (g_Plugins->Control()->ReadController) { g_Plugins->Control()->ReadController(Control, Command); } break;
            }
        }
        break;
    case 0x03: //write controller pak
        if (Controllers[Control].Present != 0)
        {
            switch (Controllers[Control].Plugin)
            {
            case PLUGIN_RAW: if (g_Plugins->Control()->ReadController) { g_Plugins->Control()->ReadController(Control, Command); } break;
            }
        }
        break;
    }
}

void CPifRam::LogControllerPakData(const char * Description)
{
    uint8_t * PIF_Ram = g_MMU->PifRam();

    int32_t count, count2;
    char HexData[100], AsciiData[100], Addon[20];
    LogMessage("\t%s:", Description);
    LogMessage("\t------------------------------");
    for (count = 0; count < 16; count++)
    {
        if ((count % 4) == 0)
        {
            HexData[0] = '\0';
            AsciiData[0] = '\0';
        }
        sprintf(Addon, "%02X %02X %02X %02X",
            PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1],
            PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3]);
        strcat(HexData, Addon);
        if (((count + 1) % 4) != 0)
        {
            sprintf(Addon, "-");
            strcat(HexData, Addon);
        }

        Addon[0] = 0;
        for (count2 = 0; count2 < 4; count2++)
        {
            if (PIF_Ram[(count << 2) + count2] < 30)
            {
                strcat(Addon, ".");
            }
            else
            {
                sprintf(Addon, "%s%c", Addon, PIF_Ram[(count << 2) + count2]);
            }
        }
        strcat(AsciiData, Addon);

        if (((count + 1) % 4) == 0)
        {
            LogMessage("\t%s %s", HexData, AsciiData);
        }
    }
    LogMessage("");
}