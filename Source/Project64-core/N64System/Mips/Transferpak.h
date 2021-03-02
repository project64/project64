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
