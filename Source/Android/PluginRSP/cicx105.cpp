/****************************************************************************
 *                                                                          *
 * Project64 - A Nintendo 64 emulator.                                      *
 * http://www.pj64-emu.com/                                                 *
 * Copyright (C) 2016 Project64. All rights reserved.                       *
 * Copyright (C) 2012 Bobby Smiles                                          *
 * Copyright (C) 2009 Richard Goedeken                                      *
 * Copyright (C) 2002 Hacktarux                                             *
 *                                                                          *
 * License:                                                                 *
 * GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
 * version 2 of the License, or (at your option) any later version.         *
 *                                                                          *
 ****************************************************************************/
#include "stdafx.h"
#include <string.h>

/**
* During IPL3 stage of CIC x105 games, the RSP performs some checks and transactions
* necessary for booting the game.
*
* We only implement the needed DMA transactions for booting.
*
* Found in Banjo-Tooie, Zelda, Perfect Dark, ...)
**/
void cicx105_ucode(CHle * hle)
{
    /* memcpy is okay to use because access constrains are met (alignment, size) */
    unsigned int i;
    unsigned char *dst = hle->dram() + 0x2fb1f0;
    unsigned char *src = hle->imem() + 0x120;

    /* dma_read(0x1120, 0x1e8, 0x1e8) */
    memcpy(hle->imem() + 0x120, hle->dram() + 0x1e8, 0x1f0);

    /* dma_write(0x1120, 0x2fb1f0, 0xfe817000) */
    for (i = 0; i < 24; ++i)
    {
        memcpy(dst, src, 8);
        dst += 0xff0;
        src += 0x8;
    }
}