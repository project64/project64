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
#define PJ64_DISCORD_APPID "359086208862650378"

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
	DiscordRichPresence discordPresence = {};


	if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "MarioParty3") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Mario Party 3");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "mp3";
		discordPresence.largeImageText = "Mario Party 3";
		discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "MarioParty2") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Mario Party 2");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "mp2";
		discordPresence.largeImageText = "Mario Party 2";
		discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "MarioParty") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Mario Party 1");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "mp1";
		discordPresence.largeImageText = "Mario Party 1";
		discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "SUPER MARIO 64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Super Mario 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "sm64";
		discordPresence.largeImageText = "Super Mario 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "MARIOKART64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Mario Kart 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "mk64";
		discordPresence.largeImageText = "Mario Kart 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "SMASH BROTHERS") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Super Smash Bros.");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "ssb";
		discordPresence.largeImageText = "Super Smash Bros.",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "THE LEGEND OF ZELDA") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing The Legend Of Zelda: Ocarina Of Time");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "loz-ocarina";
		discordPresence.largeImageText = "The Legend Of Zelda: Ocarina Of Time",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "ZELDA MAJORA'S MASK") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing The Legend Of Zelda: Majora's Mask");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "loz-majora";
		discordPresence.largeImageText = "The Legend Of Zelda: Majora's Mask",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "Banjo-Kazooie") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Banjo Kazooie");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "banjo-kazooie";
		discordPresence.largeImageText = "Banjo Kazooie",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "BANJO TOOIE") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Banjo Tootie");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "banjo-tootie";
		discordPresence.largeImageText = "Banjo Tootie",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "WAVE RACE 64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Wave Race 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "waverace";
		discordPresence.largeImageText = "Waverace 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}


	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "F-ZERO X") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing F-Zero X");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "fzero";
		discordPresence.largeImageText = "F-Zero X",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "DONKEY KONG 64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Donkey Kong 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "dk64";
		discordPresence.largeImageText = "Donkey Kong 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "PAPER MARIO") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Paper Mario");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "papermario";
		discordPresence.largeImageText = "Paper Mario",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "BOMBERMAN64U") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Bomberman 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "bomberman";
		discordPresence.largeImageText = "Bomberman 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "POKEMON STADIUM") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Pokemon Stadium");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "pokemon-stadium";
		discordPresence.largeImageText = "Pokemon Stadium",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "POKEMON STADIUM 2") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Pokemon Stadium 2");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "pokemon-stadium-2";
		discordPresence.largeImageText = "Pokemon Stadium 2",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "GOLDENEYE") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Goldeneye 007");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "goldeneye";
		discordPresence.largeImageText = "Goldeneye 007",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "Perfect Dark") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Perfect Dark");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "perfect-dark";
		discordPresence.largeImageText = "Perfect Dark",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "STARFOX64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Starfox 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "starfox";
		discordPresence.largeImageText = "Starfox 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else if (strstr(g_Settings->LoadStringVal(Game_GameName).c_str(), "Pilot Wings64") != NULL)
	{
		char szState[256];
		sprintf(szState, "Playing Pilot Wings 64");
		discordPresence.state = szState;
		discordPresence.startTimestamp = Timestamp();
		discordPresence.largeImageKey = "pilotwings";
		discordPresence.largeImageText = "Pilotwings 64",
			discordPresence.smallImageKey = "icon";
		discordPresence.smallImageText = "Project64";
		discordPresence.instance = 1;
	}

	else
	{
	discordPresence.startTimestamp = Timestamp();
	discordPresence.largeImageKey = "icon";
	discordPresence.instance = 1;
	}
	Discord_UpdatePresence(&discordPresence);
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