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
#pragma once

#include <stddef.h>
#include <stdint.h>

struct gb_cart
{
	uint8_t* rom;
	uint8_t* ram;

	size_t rom_size;
	size_t ram_size;

	unsigned int rom_bank;
	unsigned int ram_bank;

	bool has_rtc;
	bool ram_bank_mode;
	bool ram_enabled;

	void(*read_gb_cart)(struct gb_cart* gb_cart, uint16_t address, uint8_t* data);
	void(*write_gb_cart)(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data);
};

class GBCart
{
public:
	static int init_gb_cart(struct gb_cart* gb_cart, const char* gb_file);
	static void release_gb_cart(struct gb_cart* gb_cart);

	static void read_gb_cart(struct gb_cart* gb_cart, uint16_t address, uint8_t* data);
	static void write_gb_cart(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data);
};



