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
#include "Rumblepak.h"

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Project64-core/Plugins/ControllerPlugin.h>

void Rumblepak::ReadFrom(uint8_t * command)
{
	uint32_t address = (command[3] << 8) | (command[4] & 0xE0);

	if ((address >= 0x8000) && (address < 0x9000))
	{
		memset(&command[5], 0x80, 0x20);
	}
	else
	{
		memset(&command[5], 0x00, 0x20);
	}
}

void Rumblepak::WriteTo(int32_t Control, uint8_t * command)
{
	uint32_t address = (command[3] << 8) | (command[4] & 0xE0);

	if ((address) == 0xC000)
	{
		if (g_Plugins->Control()->RumbleCommand != NULL)
		{
			g_Plugins->Control()->RumbleCommand(Control, *(int *)(&command[5]));
		}
	}
}