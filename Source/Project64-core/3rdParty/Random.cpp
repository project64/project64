/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
/*
 * Implements the CRandom class.
 *
 * This class implements the Lehmer Random Number Generator.
 * 
 */

#include "Random.h"
#include <time.h>

CRandom::CRandom()
{
    state = (uint32_t)time(NULL);
}

CRandom::CRandom(uint32_t seed_value)
{
    state = seed_value;
}

uint32_t randomizer(uint32_t val)
{
    return ((uint64_t)val * 279470273UL) % 4294967291UL;
}

uint32_t CRandom::next()
{
    state = randomizer(state);
    return state;
}

void CRandom::seed(uint32_t seed_value)
{
    if (seed_value == 0)
        state == 1;
    else
        state = seed_value;
}
