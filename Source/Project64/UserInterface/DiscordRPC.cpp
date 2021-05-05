#include "stdafx.h"
#include <3rdParty\discord-rpc\include\discord_rpc.h>
#include "DiscordRPC.h"

#define UNIX_TIME_START    0x019DB1DED53E8000
#define TICKS_PER_SECOND   10000000

// Discord Project64 app ID
#define PJ64_DISCORD_APPID "704794684387491891"

void CDiscord::Init()
{
	DiscordEventHandlers handlers = {};

	Discord_Initialize(PJ64_DISCORD_APPID, &handlers, 1, nullptr);
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
		if (strstr(const_cast<char*>(Default.c_str()), "?") != nullptr) {
			return Default.substr(Default.find("?") + 1);
		}
		return Default;
	}
}

void CDiscord::Update(bool bHaveGame)
{
	// Variables we use later
	// Title uses the Rdb_GoodName to display a proper game name over DiscordRPC
	// Artwork uses the header of the ROM to easily add game images through the Discord developer panel using the ID above
	stdstr title = bHaveGame ? GetTitle() : "";
	stdstr artwork = bHaveGame ? g_Settings->LoadStringVal(Rdb_RPCKey) : "";

	// Load game into DiscordRPC
	DiscordRichPresence discordPresence = {}; // Activates DiscordRPC
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

	Discord_UpdatePresence(&discordPresence); // End DiscordRPC
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
