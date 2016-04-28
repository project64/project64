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
#include "GBCart.h"
enum cart_access_mode
{
	CART_NOT_INSERTED = 0x40,
	CART_ACCESS_MODE_0 = 0x80,
	CART_ACCESS_MODE_1 = 0x89
};

struct transferpak
{
	unsigned int enabled;
	unsigned int bank;
	unsigned int access_mode;
	unsigned int access_mode_changed;
	struct gb_cart gb_cart;
};

class Transferpak
{
public:

    static void Release();
    static void Init();
	static void ReadFrom(uint16_t address, uint8_t * command);
	static void WriteTo(uint16_t address, uint8_t * command);
};
