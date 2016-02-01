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
#include "GBCart.h"

#include <time.h>
#include <Project64-core/N64System/SystemGlobals.h>

//--------------------------------------------------------------------------------------
bool read_from_file(const char *filename, void *data, size_t size)
{
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        return false;
    }

    if (fread(data, 1, size, f) != size)
    {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

bool load_file(const char* filename, void** buffer, size_t* size)
{
    FILE* fd;
    size_t l_size;
    void* l_buffer;

    /* open file */
    fd = fopen(filename, "rb");
    if (fd == NULL)
    {
        return false;
    }

    /* obtain file size */
    if (fseek(fd, 0, SEEK_END) != 0)
    {
        fclose(fd);
        return false;
    }

    l_size = (size_t)ftell(fd);
    if (l_size == -1)
    {
        fclose(fd);
        return false;
    }

    if (fseek(fd, 0, SEEK_SET) != 0)
    {
        fclose(fd);
        return false;
    }

    /* allocate buffer */
    l_buffer = malloc(l_size);
    if (l_buffer == NULL)
    {
        fclose(fd);
        return false;
    }

    /* copy file content to buffer */
    if (fread(l_buffer, 1, l_size, fd) != l_size)
    {
        free(l_buffer);
        fclose(fd);
        return false;
    }

    /* commit buffer,size */
    *buffer = l_buffer;
    *size = l_size;

    /* close file */
    fclose(fd);
    return true;
}
//--------------------------------------------------------------------------------------

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
			if (gb_cart->has_rtc && (gb_cart->ram_bank >= 0x08 && gb_cart->ram_bank <= 0x0c))
			{
                switch (gb_cart->ram_bank)
                {
                case 0x08:
                    data[0] = gb_cart->rtc_latch_second;
                    break;
                case 0x09:
                    data[0] = gb_cart->rtc_latch_minute;
                    break;
                case 0x0A:
                    data[0] = gb_cart->rtc_latch_hour;
                    break;
                case 0x0B:
                    data[0] = gb_cart->rtc_latch_day;
                    break;
                case 0x0C:
                    data[0] = (gb_cart->rtc_latch_day_carry << 7) | (gb_cart->rtc_latch_day >> 8);
                    break;
                }
			}
			else
			{
				offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
				if (offset < gb_cart->ram_size)
				{
					memcpy(data, &gb_cart->ram[offset], 0x20);
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
            gb_cart->rtc_latch_second    = gb_cart->rtc_second;
            gb_cart->rtc_latch_minute    = gb_cart->rtc_minute;
            gb_cart->rtc_latch_hour      = gb_cart->rtc_hour;
            gb_cart->rtc_latch_day       = gb_cart->rtc_day;
            gb_cart->rtc_latch_day_carry = gb_cart->rtc_day_carry;
        }
        gb_cart->rtc_latch = data[0];
	}
	else if ((address >= 0xA000) && (address <= 0xBFFF)) // Write to RAM
	{
        if (gb_cart->ram != NULL)
        {
            if (gb_cart->has_rtc && (gb_cart->ram_bank >= 0x8 && gb_cart->ram_bank <= 0xC))
            {
                bank = data[0];

                /* RTC write */
                switch (gb_cart->ram_bank)
                {
                case 0x08:
                    if (bank >= 60)
                        bank = 0;
                    gb_cart->rtc_second = bank;
                    break;
                case 0x09:
                    if (bank >= 60)
                        bank = 0;
                    gb_cart->rtc_minute = bank;
                    break;
                case 0x0A:
                    if (bank >= 24)
                        bank = 0;
                    gb_cart->rtc_hour = bank;
                    break;
                case 0x0B:
                    gb_cart->rtc_day = (gb_cart->rtc_day & 0x0100) | bank;
                    break;
                case 0x0C:
                    gb_cart->rtc_day = ((bank & 1) << 8) | (gb_cart->rtc_day & 0xFF);
                    gb_cart->rtc_day_carry = bank & 0x80;
                    break;
                }
            }
            else
            {
                offset = (address - 0xA000) + (gb_cart->ram_bank * 0x2000);
                if (offset < gb_cart->ram_size)
                {
                    memcpy(&gb_cart->ram[offset], data, 0x20);
                }
            }
        }
	}
}

static void read_gb_cart_mbc4(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_mbc4(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_mbc5(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
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

static void read_gb_cart_mmm01(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_mmm01(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_pocket_cam(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
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

static void read_gb_cart_bandai_tama5(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_bandai_tama5(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_huc1(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_huc1(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void read_gb_cart_huc3(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
	g_Notify->BreakPoint(__FILE__, __LINE__);
}

static void write_gb_cart_huc3(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
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
		{ MBC(normal),          GED_NONE },
		{ MBC(mbc1),            GED_NONE },
		{ MBC(mbc1),            GED_RAM },
		{ MBC(mbc1),            GED_RAM | GED_BATTERY },
		{ MBC(mbc2),            GED_NONE },
		{ MBC(mbc2),            GED_BATTERY },
		{ MBC(normal),          GED_RAM },
		{ MBC(normal),          GED_RAM | GED_BATTERY },
		{ MBC(mmm01),           GED_NONE },
		{ MBC(mmm01),           GED_RAM },
		{ MBC(mmm01),           GED_RAM | GED_BATTERY },
		{ MBC(mbc3),            GED_BATTERY | GED_RTC },
		{ MBC(mbc3),            GED_RAM | GED_BATTERY | GED_RTC },
		{ MBC(mbc3),            GED_NONE },
		{ MBC(mbc3),            GED_RAM },
		{ MBC(mbc3),            GED_RAM | GED_BATTERY },
		{ MBC(mbc4),            GED_NONE },
		{ MBC(mbc4),            GED_RAM },
		{ MBC(mbc4),            GED_RAM | GED_BATTERY },
		{ MBC(mbc5),            GED_NONE },
		{ MBC(mbc5),            GED_RAM },
		{ MBC(mbc5),            GED_RAM | GED_BATTERY },
		{ MBC(mbc5),            GED_RUMBLE },
		{ MBC(mbc5),            GED_RAM | GED_RUMBLE },
		{ MBC(mbc5),            GED_RAM | GED_BATTERY | GED_RUMBLE },
		{ MBC(pocket_cam),      GED_NONE },
		{ MBC(bandai_tama5),    GED_NONE },
		{ MBC(huc3),            GED_NONE },
		{ MBC(huc1),            GED_RAM | GED_BATTERY }
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
	uint8_t* rom = NULL;
	size_t rom_size = 0;
	uint8_t* ram = NULL;
	size_t ram_size = 0;

	/* load GB cart ROM */
	if (!load_file(gb_file, (void**)&rom, &rom_size))
	{
		return false;
	}

	if (rom_size < 0x8000)
	{
        free(rom);
        return false;
	}

	/* get and parse cart type */
	uint8_t cart_type = rom[0x147];
	type = parse_cart_type(cart_type);
	if (type == NULL)
	{
        free(rom);
        return false;
	}

	/* load ram (if present) */
	if (type->extra_devices & GED_RAM)
	{
		ram_size = 0;
		switch (rom[0x149])
		{
		case 0x01: ram_size =  1 * 0x800; break;
		case 0x02: ram_size =  4 * 0x800; break;
		case 0x03: ram_size = 16 * 0x800; break;
		case 0x04: ram_size = 64 * 0x800; break;
		case 0x05: ram_size = 32 * 0x800; break;
		}

		if (ram_size != 0)
		{
            if (type->extra_devices & GED_RTC)
            {
                ram_size += 0x30;
            }
           
			ram = (uint8_t*)malloc(ram_size );
			if (ram == NULL)
			{
                free(rom);
                return false;
			}

			read_from_file("C:/Users/death/Desktop/pokemonsilver.sav", ram, ram_size );
		}
        
        //If we have RTC we need to load in the data, we assume the save will use the VBA-M format
        if (type->extra_devices & GED_RTC)
        {
            gbCartRTC rtc;
            memcpy(&rtc, &ram[ram_size-0x30], 0x30);
           
            gb_cart->rtc_second          = rtc.second;
            gb_cart->rtc_minute          = rtc.minute;
            gb_cart->rtc_hour            = rtc.hour;
            gb_cart->rtc_day             = rtc.day;
            gb_cart->rtc_day_carry       = rtc.day_carry;
            gb_cart->rtc_latch_second    = rtc.latch_second;
            gb_cart->rtc_latch_minute    = rtc.latch_minute;
            gb_cart->rtc_latch_hour      = rtc.latch_hour;
            gb_cart->rtc_latch_day       = rtc.latch_day;
            gb_cart->rtc_latch_day_carry = rtc.latch_day_carry;
            gb_cart->rtc_last_time       = rtc.mapperLastTime;
        }
	}

	/* update gb_cart */
    gb_cart->ram = ram;
	gb_cart->rom = rom;
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
    FILE *fRAM = fopen("C:/Users/death/Desktop/pokemonsilver.sav", "wb");

    if (gb_cart->has_rtc)
    {
        fwrite(gb_cart->ram, 1, gb_cart->ram_size-0x30, fRAM);

        gbCartRTC rtc;
        rtc.second = gb_cart->rtc_second;
        rtc.minute = gb_cart->rtc_minute;
        rtc.hour = gb_cart->rtc_hour;
        rtc.day = gb_cart->rtc_day;
        rtc.day_carry = gb_cart->rtc_day_carry;
        rtc.latch_second = gb_cart->rtc_latch_second;
        rtc.latch_minute = gb_cart->rtc_latch_minute;
        rtc.latch_hour = gb_cart->rtc_latch_hour;
        rtc.latch_day = gb_cart->rtc_latch_day;
        rtc.latch_day_carry = gb_cart->rtc_latch_day_carry;
        rtc.mapperLastTime = gb_cart->rtc_last_time;
        fwrite(&rtc, 1, 0x30, fRAM);
    }
    else
    {
        fwrite(gb_cart->ram, 1, gb_cart->ram_size, fRAM);
    }

    fclose(fRAM);
}

void GBCart::release_gb_cart(struct gb_cart* gb_cart)
{
	if (gb_cart->rom != NULL)
		free(gb_cart->rom);

    if (gb_cart->ram != NULL)
        free(gb_cart->ram);

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
