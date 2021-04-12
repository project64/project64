// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2015 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#include "stdafx.h"
#include "GBCart.h"

#include <time.h>
#include <memory>

static void read_gb_cart_normal(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    uint16_t offset;

    if ((address >= 0x0000) && (address <= 0x7FFF))
    {
        //Read GB Cart
        if (address >= gb_cart->rom_size)
        {
            //If address is larger then our rome size, bail out
            return;
        }
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF))
    {
        //Read from RAM
        if (gb_cart->ram == NULL)
        {
            //No RAM to write to
            return;
        }

        offset = address - 0xA000;
        if (offset >= gb_cart->ram_size)
        {
            //Offset is larger then our ram size
            return;
        }

        memcpy(&gb_cart->ram[offset], data, 0x20);
        memcpy(data, &gb_cart->ram[offset], 0x20);
    }
}

static void write_gb_cart_normal(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    uint16_t offset;
    if ((address >= 0xA000) && (address <= 0xBFFF))
    {
        //Write to RAM
        if (gb_cart->ram == NULL)
        {
            //No RAM to write to
            return;
        }

        offset = address - 0xa000;
        if (offset >= gb_cart->ram_size)
        {
            //Offset is larger then our ram size
            return;
        }

        memcpy(&gb_cart->ram[offset], data, 0x20);
    }
}

static void read_gb_cart_mbc1(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x3FFF)) //No nbanked memory
    {
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0x4000) && (address <= 0x7FFF)) //Read from ROM
    {
        offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
        if (offset < gb_cart->rom_size)
        {
            memcpy(data, &gb_cart->rom[offset], 0x20);
        }
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) //Read from RAM
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(data, &gb_cart->ram[offset], 0x20);
            }
        }
    }
}

static void write_gb_cart_mbc1(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x1FFF)) // RAM enable
    {
        //Enable/disable RAM
        gb_cart->ram_enabled = (data[0] & 0x0F) == 0x0A;
    }
    else if ((address >= 0x2000) && (address <= 0x3FFF)) // ROM bank select
    {
        gb_cart->rom_bank &= 0x60;	// keep MSB
        gb_cart->rom_bank |= data[0] & 0x1F;

        // emulate quirk: 0x00 -> 0x01, 0x20 -> 0x21, 0x40->0x41, 0x60 -> 0x61
        if ((gb_cart->rom_bank & 0x1F) == 0)
        {
            gb_cart->rom_bank |= 0x01;
        }
    }
    else if ((address >= 0x4000) && (address <= 0x5FFF)) // RAM bank select
    {
        if (gb_cart->ram_bank_mode)
        {
            gb_cart->ram_bank = data[0] & 0x03;
        }
        else
        {
            gb_cart->rom_bank &= 0x1F;
            gb_cart->rom_bank |= ((data[0] & 0x03) << 5); // set bits 5 and 6 of ROM bank
        }
    }
    else if ((address >= 0x6000) && (address <= 0x7FFF)) // MBC1 mode select
    {
        // this is overly complicated, but it keeps us from having to do bitwise math later
        // Basically we shuffle the 2 "magic bits" between rom_bank and ram_bank as necessary.
        if (gb_cart->ram_bank_mode != (data[0] & 0x01))
        {
            // we should only alter the ROM and RAM bank numbers if we have changed modes
            gb_cart->ram_bank_mode = data[0] & 0x01;
            if (gb_cart->ram_bank_mode)
            {
                gb_cart->ram_bank = gb_cart->rom_bank >> 5;	// set the ram bank to the "magic bits"
                gb_cart->rom_bank &= 0x1F; // zero out bits 5 and 6 to keep consistency
            }
            else
            {
                gb_cart->rom_bank &= 0x1F;
                gb_cart->rom_bank |= (gb_cart->ram_bank << 5);
                gb_cart->ram_bank = 0x00;	// we can only reach RAM page 0
            }
        }
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(&gb_cart->ram[offset], data, 0x20);
            }
        }
    }
}

static void read_gb_cart_mbc2(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    size_t offset;

    if ((address < 0x4000)) //Rom Bank 0
    {
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0x4000) && (address < 0x8000)) //Switchable Rom Bank
    {
        offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
        if (offset < gb_cart->rom_size)
        {
            memcpy(data, &gb_cart->rom[offset], 0x20);
        }
    }
    else if ((address >= 0xA000) && (address <= 0xC000)) //Upper Bounds of memory map
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(data, &gb_cart->ram[offset], 0x20);
            }
        }
    }
}

static void write_gb_cart_mbc2(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        gb_cart->ram_enabled = (data[0] & 0x0F) == 0x0A;
    }
    else if ((address >= 0x2000) && (address <= 0x3FFF)) // ROM bank select
    {
        gb_cart->rom_bank = data[0] & 0x0F;
        if (gb_cart->rom_bank == 0)
        {
            gb_cart->rom_bank = 1;
        }
    }
    else if ((address >= 0x4000) && (address <= 0x5FFF)) // RAM bank select
    {
        if (gb_cart->ram != NULL)
        {
            gb_cart->ram_bank = data[0] & 0x07;
        }
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(&gb_cart->ram[offset], data, 0x20);
            }
        }
    }
}

void memoryUpdateMBC3Clock(struct gb_cart* gb_cart)
{
    time_t now = time(NULL);
    time_t diff = now - gb_cart->rtc_last_time;
    if (diff > 0) {
        // update the clock according to the last update time
        gb_cart->rtc_data[0] += (int)(diff % 60);
        if (gb_cart->rtc_data[0] > 59) {
            gb_cart->rtc_data[0] -= 60;
            gb_cart->rtc_data[1]++;
        }
        diff /= 60;

        gb_cart->rtc_data[1] += (int)(diff % 60);
        if (gb_cart->rtc_data[1] > 59) {
            gb_cart->rtc_data[1] -= 60;
            gb_cart->rtc_data[2]++;
        }
        diff /= 60;

        gb_cart->rtc_data[2] += (int)(diff % 24);
        if (gb_cart->rtc_data[2] > 23) {
            gb_cart->rtc_data[2] -= 24;
            gb_cart->rtc_data[3]++;
        }
        diff /= 24;

        gb_cart->rtc_data[3] += (int)(diff & 0xFFFFFFFF);
        if (gb_cart->rtc_data[3] > 255) {
            if (gb_cart->rtc_data[3] > 511) {
                gb_cart->rtc_data[3] %= 512;
                gb_cart->rtc_data[3] |= 0x80;
            }
            gb_cart->rtc_data[4] = (gb_cart->rtc_data[4] & 0xFE) | (gb_cart->rtc_data[3] > 255 ? 1 : 0);
        }
    }
    gb_cart->rtc_last_time = now;
}

static void read_gb_cart_mbc3(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    size_t offset;

    if ((address < 0x4000)) //Rom Bank 0
    {
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0x4000) && (address < 0x8000)) //Switchable Rom Bank
    {
        offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
        if (offset < gb_cart->rom_size)
        {
            memcpy(data, &gb_cart->rom[offset], 0x20);
        }
    }
    else if ((address >= 0xA000) && (address <= 0xC000)) //Upper Bounds of memory map
    {
        if (gb_cart->ram != NULL)
        {
            if (gb_cart->ram_bank <= 0x03)
            {
                offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
                if (offset < gb_cart->ram_size)
                {
                    memcpy(data, &gb_cart->ram[offset], 0x20);
                }
            }
            else if (gb_cart->has_rtc)
            {
                if (gb_cart->rtc_latch)
                {
                    for (int i = 0; i < 32; i++)
                    {
                        data[i] = (uint8_t)(gb_cart->rtc_latch_data[gb_cart->ram_bank - 0x08]);
                    }
                }
                else
                {
                    memoryUpdateMBC3Clock(gb_cart);
                    for (int i = 0; i < 32; i++)
                    {
                        data[i] = (uint8_t)(gb_cart->rtc_data[gb_cart->ram_bank - 0x08]);
                    }
                }
            }
        }
    }
}

static void write_gb_cart_mbc3(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    uint8_t bank;
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        //Enable / Disable RAM -- NOT WORKING -- FIXME
        gb_cart->ram_enabled = (data[0] & 0x0F) == 0x0A;
    }
    else if ((address >= 0x2000) && (address <= 0x3FFF)) // ROM bank select
    {
        bank = data[0] & 0x7F;
        gb_cart->rom_bank = (bank == 0) ? 1 : bank;
    }
    else if ((address >= 0x4000) && (address <= 0x5FFF)) // RAM/Clock bank select
    {
        if (gb_cart->ram != NULL)
        {
            bank = data[0];
            if (gb_cart->has_rtc && (bank >= 0x8 && bank <= 0xc))
            {
                //Set the bank for the timer
                gb_cart->ram_bank = bank;
            }
            else
            {
                gb_cart->ram_bank = bank & 0x03;
            }
        }
    }
    else if ((address >= 0x6000) && (address <= 0x7FFF)) // Latch timer data
    {
        //Implement RTC timer / latch
        if (gb_cart->rtc_latch == 0 && data[0] == 1)
        {
            //Update time
            memoryUpdateMBC3Clock(gb_cart);
            for (int i = 0; i < 4; i++)
            {
                gb_cart->rtc_latch_data[i] = gb_cart->rtc_data[i];
            }
        }
        gb_cart->rtc_latch = data[0];
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
    {
        if (gb_cart->ram != NULL)
        {
            if (gb_cart->ram_bank <= 0x03)
            {
                offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
                if (offset < gb_cart->ram_size)
                {
                    memcpy(&gb_cart->ram[offset], data, 0x20);
                }
            }
            else if (gb_cart->has_rtc)
            {
                /* RTC write */
                gb_cart->rtc_data[gb_cart->ram_bank - 0x08] = data[0];
            }
        }
    }
}

static void read_gb_cart_mbc4(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_mbc4(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, const uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_mbc5(struct gb_cart * gb_cart, uint16_t address, uint8_t * data)
{
    size_t offset;

    if ((address < 0x4000)) //Rom Bank 0
    {
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0x4000) && (address < 0x8000)) //Switchable ROM BANK
    {
        offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
        if (offset < gb_cart->rom_size)
        {
            memcpy(data, &gb_cart->rom[offset], 0x20);
        }
    }
    else if ((address >= 0xA000) && (address <= 0xC000)) //Upper bounds of memory map
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(data, &gb_cart->ram[offset], 0x20);
            }
        }
    }
}

static void write_gb_cart_mbc5(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x1FFF))  // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        //Enable / Disable RAM -- NOT WORKING -- CHECK ME
        gb_cart->ram_enabled = (data[0] & 0x0F) == 0x0A;
    }
    else if ((address >= 0x2000) && (address <= 0x2FFF)) // ROM bank select, low bits
    {
        gb_cart->rom_bank &= 0xff00;
        gb_cart->rom_bank |= data[0];
    }
    else if ((address >= 0x3000) && (address <= 0x3FFF)) // ROM bank select, high bit
    {
        gb_cart->rom_bank &= 0x00ff;
        gb_cart->rom_bank |= (data[0] & 0x01) << 8;
    }
    else if ((address >= 0x4000) && (address <= 0x5FFF)) // RAM bank select
    {
        if (gb_cart->ram != NULL)
        {
            gb_cart->ram_bank = data[0] & 0x0f;
        }
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
    {
        if (gb_cart->ram != NULL)
        {
            offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
                memcpy(&gb_cart->ram[offset], data, 0x20);
            }
        }
    }
}

static void read_gb_cart_mmm01(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_mmm01(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, const uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_pocket_cam(struct gb_cart * gb_cart, uint16_t address, uint8_t * data)
{
    size_t offset;

    if ((address < 0x4000)) //Rom Bank 0
    {
        memcpy(data, &gb_cart->rom[address], 0x20);
    }
    else if ((address >= 0x4000) && (address < 0x8000)) //Switchable ROM BANK
    {
        offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
        if (offset < gb_cart->rom_size)
        {
            memcpy(data, &gb_cart->rom[offset], 0x20);
        }
    }
    else if ((address >= 0xA000) && (address <= 0xC000)) //Upper bounds of memory map
    {
        //Check to see if where currently in register mode
        if (gb_cart->ram != NULL)
        {
            if (gb_cart->ram_bank & 0x10)
            {
                //Where in register mode, based off NRAGE we just fill the memory with Zeroes.
                //Seems to be incorrect behaviour but need to find more doccumentation
                memset(data, 0x00, 0x20);
            }
            else
            {
                //Read RAM normally
                offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
                if (offset < gb_cart->ram_size)
                {
                    memcpy(data, &gb_cart->ram[offset], 0x20);
                }
            }
        }
    }
}

static void write_gb_cart_pocket_cam(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    size_t offset;

    if ((address >= 0x0000) && (address <= 0x1FFF))  // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        //Enable / Disable RAM
        gb_cart->ram_enabled = (data[0] & 0x0F) == 0x0A;
    }
    else if ((address >= 0x2000) && (address <= 0x2FFF)) // ROM bank select, low bits
    {
        gb_cart->rom_bank &= 0xFF00;
        gb_cart->rom_bank |= data[0];
    }
    else if ((address >= 0x4000) && (address <= 0x4FFF)) // Camera Register & RAM bank select
    {
        if (gb_cart->ram != NULL)
        {
            if (data[0] & 0x10)
            {
                //REGISTER MODE
                gb_cart->ram_bank = data[0];
            }
            else
            {
                //RAM MODE
                gb_cart->ram_bank = data[0] & 0x0F;
            }
        }
    }
    else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
    {
        if (gb_cart->ram != NULL)
        {
            if (gb_cart->ram_bank & 0x10)
            {
                //REGISTER MODE (DO NOTHING)
            }
            else
            {
                //RAM MODE
                offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
                if (offset < gb_cart->ram_size)
                {
                    memcpy(&gb_cart->ram[offset], data, 0x20);
                }
            }
        }
    }
}

static void read_gb_cart_bandai_tama5(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_bandai_tama5(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, const uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_huc1(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_huc1(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, const uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_huc3(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_huc3(struct gb_cart * /*gb_cart*/, uint16_t /*address*/, const uint8_t * /*data*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

enum gbcart_extra_devices
{
    GED_NONE = 0x00,
    GED_RAM = 0x01,
    GED_BATTERY = 0x02,
    GED_RTC = 0x04,
    GED_RUMBLE = 0x08
};

struct parsed_cart_type
{
    void(*read_gb_cart)(struct gb_cart*, uint16_t, uint8_t*);
    void(*write_gb_cart)(struct gb_cart*, uint16_t, const uint8_t*);
    unsigned int extra_devices;
};

static const struct parsed_cart_type* parse_cart_type(uint8_t cart_type)
{
#define MBC(x) read_gb_cart_ ## x, write_gb_cart_ ## x
    static const struct parsed_cart_type GB_CART_TYPES[] =
    {
        { MBC(normal), GED_NONE },
        { MBC(mbc1), GED_NONE },
        { MBC(mbc1), GED_RAM },
        { MBC(mbc1), GED_RAM | GED_BATTERY },
        { MBC(mbc2), GED_NONE },
        { MBC(mbc2), GED_BATTERY },
        { MBC(normal), GED_RAM },
        { MBC(normal), GED_RAM | GED_BATTERY },
        { MBC(mmm01), GED_NONE },
        { MBC(mmm01), GED_RAM },
        { MBC(mmm01), GED_RAM | GED_BATTERY },
        { MBC(mbc3), GED_BATTERY | GED_RTC },
        { MBC(mbc3), GED_RAM | GED_BATTERY | GED_RTC },
        { MBC(mbc3), GED_NONE },
        { MBC(mbc3), GED_RAM },
        { MBC(mbc3), GED_RAM | GED_BATTERY },
        { MBC(mbc4), GED_NONE },
        { MBC(mbc4), GED_RAM },
        { MBC(mbc4), GED_RAM | GED_BATTERY },
        { MBC(mbc5), GED_NONE },
        { MBC(mbc5), GED_RAM },
        { MBC(mbc5), GED_RAM | GED_BATTERY },
        { MBC(mbc5), GED_RUMBLE },
        { MBC(mbc5), GED_RAM | GED_RUMBLE },
        { MBC(mbc5), GED_RAM | GED_BATTERY | GED_RUMBLE },
        { MBC(pocket_cam), GED_NONE },
        { MBC(bandai_tama5), GED_NONE },
        { MBC(huc3), GED_NONE },
        { MBC(huc1), GED_RAM | GED_BATTERY }
    };
#undef MBC

    switch (cart_type)
    {
    case 0x00: return &GB_CART_TYPES[0];
    case 0x01: return &GB_CART_TYPES[1];
    case 0x02: return &GB_CART_TYPES[2];
    case 0x03: return &GB_CART_TYPES[3];
    case 0x05: return &GB_CART_TYPES[4];
    case 0x06: return &GB_CART_TYPES[5];
    case 0x08: return &GB_CART_TYPES[6];
    case 0x09: return &GB_CART_TYPES[7];
    case 0x0B: return &GB_CART_TYPES[8];
    case 0x0C: return &GB_CART_TYPES[9];
    case 0x0D: return &GB_CART_TYPES[10];
    case 0x0F: return &GB_CART_TYPES[11];
    case 0x10: return &GB_CART_TYPES[12];
    case 0x11: return &GB_CART_TYPES[13];
    case 0x12: return &GB_CART_TYPES[14];
    case 0x13: return &GB_CART_TYPES[15];
    case 0x15: return &GB_CART_TYPES[16];
    case 0x16: return &GB_CART_TYPES[17];
    case 0x17: return &GB_CART_TYPES[18];
    case 0x19: return &GB_CART_TYPES[19];
    case 0x1A: return &GB_CART_TYPES[20];
    case 0x1B: return &GB_CART_TYPES[21];
    case 0x1C: return &GB_CART_TYPES[22];
    case 0x1D: return &GB_CART_TYPES[23];
    case 0x1E: return &GB_CART_TYPES[24];
    case 0xFC: return &GB_CART_TYPES[25];
    case 0xFD: return &GB_CART_TYPES[26];
    case 0xFE: return &GB_CART_TYPES[27];
    case 0xFF: return &GB_CART_TYPES[28];
    default:   return NULL;
    }
}

bool GBCart::init_gb_cart(struct gb_cart* gb_cart, const char* gb_file)
{
    const struct parsed_cart_type* type;
    std::unique_ptr<uint8_t> rom;
    size_t rom_size = 0;
    std::unique_ptr<uint8_t> ram;
    size_t ram_size = 0;
    CFile tempFile;

    /* load GB cart ROM */
    if (!tempFile.Open(gb_file, CFileBase::modeRead))
    {
        g_Notify->DisplayError("Failed to open Transferpak ROM");
        return false;
    }

    rom_size = tempFile.GetLength();
    rom.reset(new uint8_t[rom_size]);

    tempFile.Read(rom.get(), rom_size);
    tempFile.Close();

    if (rom_size < 0x8000)
    {
        return false;
    }

    /* get and parse cart type */
    uint8_t cart_type = rom.get()[0x147];
    type = parse_cart_type(cart_type);
    if (type == NULL)
    {
        return false;
    }

    /* load ram (if present) */
    if (type->extra_devices & GED_RAM)
    {
        ram_size = 0;
        switch (rom.get()[0x149])
        {
        case 0x01: ram_size = 1 * 0x800; break;
        case 0x02: ram_size = 4 * 0x800; break;
        case 0x03: ram_size = 16 * 0x800; break;
        case 0x04: ram_size = 64 * 0x800; break;
        case 0x05: ram_size = 32 * 0x800; break;
        }

        if (ram_size != 0)
        {
            ram.reset(new uint8_t[ram_size]);
            if (ram.get() == NULL)
            {
                return false;
            }

            if (!tempFile.Open(g_Settings->LoadStringVal(Game_Transferpak_Sav).c_str(), CFileBase::modeRead))
            {
                g_Notify->DisplayError("Failed to open Transferpak SAV File");
                return false;
            }

            tempFile.Read(ram.get(), ram_size);
        }

        //If we have RTC we need to load in the data, we assume the save will use the VBA-M format
        if (type->extra_devices & GED_RTC)
        {
            tempFile.Read(&gb_cart->rtc_data[0], 4);
            tempFile.Read(&gb_cart->rtc_data[1], 4);
            tempFile.Read(&gb_cart->rtc_data[2], 4);
            tempFile.Read(&gb_cart->rtc_data[3], 4);
            tempFile.Read(&gb_cart->rtc_data[4], 4);
            tempFile.Read(&gb_cart->rtc_latch_data[0], 4);
            tempFile.Read(&gb_cart->rtc_latch_data[1], 4);
            tempFile.Read(&gb_cart->rtc_latch_data[2], 4);
            tempFile.Read(&gb_cart->rtc_latch_data[3], 4);
            tempFile.Read(&gb_cart->rtc_latch_data[4], 4);
            tempFile.Read(&gb_cart->rtc_last_time, 8);
            memoryUpdateMBC3Clock(gb_cart);
        }
        tempFile.Close();
    }

    /* update gb_cart */
    gb_cart->rom = rom.release();
    gb_cart->ram = ram.release();
    gb_cart->rom_size = rom_size;
    gb_cart->ram_size = ram_size;
    gb_cart->rom_bank = 1;
    gb_cart->ram_bank = 0;
    gb_cart->has_rtc = (type->extra_devices & GED_RTC) ? 1 : 0;
    gb_cart->read_gb_cart = type->read_gb_cart;
    gb_cart->write_gb_cart = type->write_gb_cart;
    return true;
}

void GBCart::save_gb_cart(struct gb_cart* gb_cart)
{
    CFile ramFile;
    ramFile.Open(g_Settings->LoadStringVal(Game_Transferpak_Sav).c_str(), CFileBase::modeWrite | CFileBase::modeCreate);
    ramFile.Write(gb_cart->ram, gb_cart->ram_size);

    if (gb_cart->has_rtc)
    {
        ramFile.Write(&gb_cart->rtc_data[0], 4);
        ramFile.Write(&gb_cart->rtc_data[1], 4);
        ramFile.Write(&gb_cart->rtc_data[2], 4);
        ramFile.Write(&gb_cart->rtc_data[3], 4);
        ramFile.Write(&gb_cart->rtc_data[4], 4);
        ramFile.Write(&gb_cart->rtc_latch_data[0], 4);
        ramFile.Write(&gb_cart->rtc_latch_data[1], 4);
        ramFile.Write(&gb_cart->rtc_latch_data[2], 4);
        ramFile.Write(&gb_cart->rtc_latch_data[3], 4);
        ramFile.Write(&gb_cart->rtc_latch_data[4], 4);
        ramFile.Write(&gb_cart->rtc_last_time, 8);
    }

    ramFile.Close();
}

void GBCart::release_gb_cart(struct gb_cart* gb_cart)
{
    if (gb_cart->rom != NULL)
        delete gb_cart->rom;

    if (gb_cart->ram != NULL)
        delete gb_cart->ram;

    memset(gb_cart, 0, sizeof(*gb_cart));
}

void GBCart::read_gb_cart(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    gb_cart->read_gb_cart(gb_cart, address, data);
}

void GBCart::write_gb_cart(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    gb_cart->write_gb_cart(gb_cart, address, data);
}