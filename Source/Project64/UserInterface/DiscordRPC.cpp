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
#include <3rdParty\discord-rpc\include\discord_rpc.h>
#include "DiscordRPC.h"

#define UNIX_TIME_START    0x019DB1DED53E8000
#define TICKS_PER_SECOND   10000000

//Discord Project64 App ID
#define PJ64_DISCORD_APPID "704794684387491891"

void CDiscord::Init()
{
	DiscordEventHandlers handlers = {};

	Discord_Initialize(PJ64_DISCORD_APPID, &handlers, 1, NULL);
}

void CDiscord::Shutdown()
{
	Discord_ClearPresence();
	Discord_Shutdown();
}

static stdstr GetTitle() 
{
	stdstr Default = "";
	bool existsInRdb = g_Settings->LoadStringVal(Rdb_GoodName, Default);
	if (existsInRdb)
		return g_Settings->LoadStringVal(Rdb_GoodName);
	else {
		Default = CPath(g_Settings->LoadStringVal(Game_File)).GetName().c_str();
		if (strstr(const_cast<char*>(Default.c_str()), "?") != NULL) {
			return Default.substr(Default.find("?") + 1);
		}
		return Default;
	}
}

void CDiscord::Update(bool bHaveGame)
{
	//Variables we use later
	//title uses the Rdb_GoodName to display a proper game name over DiscordRPC
	//artwork uses the Header of the rom to easily add game pictures through the discord developer panel using the ID above
	stdstr title = bHaveGame ? GetTitle() : "";
	stdstr artwork = bHaveGame ? g_Settings->LoadStringVal(Rdb_RPCKey) : "";

	//Load Game Into DiscordRPC
	DiscordRichPresence discordPresence = {}; //activates DiscordRPC
	if (artwork.empty())
	{
		discordPresence.largeImageKey = "pj64_icon";
		discordPresence.largeImageText = "Project64";
	}
	else
	{
		discordPresence.largeImageKey = artwork.c_str();
		discordPresence.largeImageText = title.c_str();
		discordPresence.smallImageKey = "pj64_icon";
		discordPresence.smallImageText = "Project64";
	}
	discordPresence.details = title.empty() ? "Not in-game" : title.c_str();
	discordPresence.startTimestamp = Timestamp();

	Discord_UpdatePresence(&discordPresence); //end DiscordRPC
}

int64_t CDiscord::Timestamp()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
}