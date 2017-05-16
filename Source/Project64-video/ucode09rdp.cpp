/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include <Project64-video/trace.h>
#include <Project64-video/ucode.h>

void uc9_rpdcmd()
{
    uint32_t a = segoffset(rdp.cmd1) >> 2;
    WriteTrace(TraceRDP, TraceDebug, "uc9:rdpcmd addr: %08lx", a);
    if (a)
    {
        rdp.LLE = 1;
        uint32_t cmd = 0;
        while (1)
        {
            rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a++];
            cmd = rdp.cmd0 >> 24;
            if (cmd == 0xDF)
                break;
            rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[a++];
            if (cmd == 0xE4 || cmd == 0xE5)
            {
                a++;
                rdp.cmd2 = ((uint32_t*)gfx.RDRAM)[a++];
                a++;
                rdp.cmd3 = ((uint32_t*)gfx.RDRAM)[a++];
            }
            gfx_instruction[CSettings::ucode_zSort][cmd]();
        };
        rdp.LLE = 0;
    }
}
