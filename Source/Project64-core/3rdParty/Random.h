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
 * Defines the CRandom class.
 *
 * This class implements the Lehmer Random Number Generator.
 *
 */

#include <Common/stdtypes.h>

class CRandom
{
public:
    CRandom();
    CRandom(uint32_t seed_value);
    uint32_t next();
    void seed(uint32_t seed_value);

protected:
    uint32_t randomizer(uint32_t val);
    uint32_t state;
};
