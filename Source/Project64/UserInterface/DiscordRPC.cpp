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
#define PJ64_DISCORD_APPID "594955067208105985"

void CDiscord::Init()
{
	DiscordEventHandlers handlers = {};

	Discord_Initialize(PJ64_DISCORD_APPID, &handlers, 1, NULL);
}

void CDiscord::Shutdown()
{
	Discord_Shutdown();
}

void CDiscord::Update(bool bHaveGame)
{
	//Variables we use later
	//szState uses the Rdb_GoodName to display a proper game name over DiscordRPC
	//keyState uses the Header of the rom to easily add game pictures through the discord developer panel using the ID above
	char szState[256];
	char keyState[256];
	sprintf(szState, "Playing %s", g_Settings->LoadStringVal(Rdb_GoodName).c_str()); //rdb_GoodName in a variable for use later
	sprintf(keyState, "%s", g_Settings->LoadStringVal(Game_GameName).c_str()); //Rom Header in a variable for use later

	//Load Game Into DiscordRPC
	DiscordRichPresence discordPresence = {}; //activates DiscordRPC
	if (bHaveGame && g_Settings->LoadStringVal(Game_File).length() != 0)
	{

		//DiscordRPC Game Name - Playing <game>
		discordPresence.state = szState; //sets the state of the DiscordRPC to the Rdb_GoodName file

		//Play Time over DiscordRPC
		discordPresence.startTimestamp = Timestamp(); //sets the time on the DiscordRPC

		//Large Image Text over DiscordRPC
		discordPresence.largeImageText = szState; //sets the RDB_GoodName Variable as the large image text

		//Large Image File Name over DiscordRPC
		discordPresence.largeImageText = keyState; //sets the Rom Header Variable as the large image key (the file you upload to discord)

		//Small Image over DiscordRPC
		discordPresence.smallImageKey = "icon"; //Project 64 Logo in bottom right corner
		discordPresence.smallImageText = "Project64"; //Name of the Project64 Logo
		discordPresence.instance = 1; //Instance of Active DiscordRPC
	}
	else
	{

		//Show when you are not playing a game over DiscordRPC.
		// This is not perfect due to Project64's method of loading 
		// ROM's into the filesystem before emulation starts.
		sprintf(szState, "Not in-game"); //shows "Not in-game" on the active DiscordRPC text
		discordPresence.largeImageKey = "icon"; //Shows the Project64 logo on the large image box
		discordPresence.largeImageText = "Project64"; //Name of the Project64 Logo
		discordPresence.smallImageKey = NULL; //Safety Measure to force unload the smallImageKey
		discordPresence.smallImageText = NULL; //Safety Measure to force unload the smallImageText
		discordPresence.endTimestamp = NULL; //Safety Measure to force unload the TimeStamp

	}

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