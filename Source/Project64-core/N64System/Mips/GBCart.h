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

    int rtc_data[5];
    int rtc_latch_data[5];
    time_t   rtc_last_time;

	void(*read_gb_cart)(struct gb_cart* gb_cart, uint16_t address, uint8_t* data);
	void(*write_gb_cart)(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data);
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



