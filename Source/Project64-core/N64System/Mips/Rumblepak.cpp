#include "stdafx.h"
#include "Rumblepak.h"

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Project64-core/Plugins/ControllerPlugin.h>

void Rumblepak::ReadFrom(uint32_t address, uint8_t * data)
{
	if ((address >= 0x8000) && (address < 0x9000))
	{
		memset(data, 0x80, 0x20);
	}
	else
	{
		memset(data, 0x00, 0x20);
	}
}

void Rumblepak::WriteTo(int32_t Control, uint32_t address, uint8_t * data)
{
	if ((address) == 0xC000)
	{
		if (g_Plugins->Control()->RumbleCommand != NULL)
		{
			g_Plugins->Control()->RumbleCommand(Control, *(int *)data);
		}
	}
}