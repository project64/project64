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

    uint32_t rtc_latch;

    uint32_t rtc_second;
    uint32_t rtc_minute;
    uint32_t rtc_hour;
    uint32_t rtc_day;
    uint32_t rtc_day_carry;
    uint32_t rtc_latch_second;
    uint32_t rtc_latch_minute;
    uint32_t rtc_latch_hour;
    uint32_t rtc_latch_day;
    uint32_t rtc_latch_day_carry;
    time_t   rtc_last_time;

	void(*read_gb_cart)(struct gb_cart* gb_cart, uint16_t address, uint8_t* data);
	void(*write_gb_cart)(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data);
};

struct gbCartRTC {
    uint32_t second;
    uint32_t minute;
    uint32_t hour;
    uint32_t day;
    uint32_t day_carry;
    uint32_t latch_second;
    uint32_t latch_minute;
    uint32_t latch_hour;
    uint32_t latch_day;
    uint32_t latch_day_carry;
    time_t mapperLastTime;
};

class GBCart
{
public:
	static bool init_gb_cart(struct gb_cart* gb_cart, const char* gb_file);
	static void release_gb_cart(struct gb_cart* gb_cart);
    static void save_gb_cart(struct gb_cart* gb_cart);

	static void read_gb_cart(struct gb_cart* gb_cart, uint16_t address, uint8_t* data);
	static void write_gb_cart(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data);
};



