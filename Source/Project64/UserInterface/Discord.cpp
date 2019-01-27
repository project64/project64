#include "stdafx.h"
#include <3rdParty\discord-rpc\include\discord_rpc.h>

#include "Discord.h"

#define UNIX_TIME_START    0x019DB1DED53E8000
#define TICKS_PER_SECOND   10000000
#define PJ64_DISCORD_APPID "359086208862650378"

// void CDiscord::OnReady(const DiscordUser* /*request*/) {}
// void CDiscord::OnError(int /*errorCode*/, const char* /*message*/) {}
// void CDiscord::OnDisconnected(int /*errorCode*/, const char* /*message*/) {}
// void CDiscord::OnJoinGame(const char* /*joinSecret*/) {}
// void CDiscord::OnSpectateGame(const char* /*spectateSecret*/) {}
// void CDiscord::OnJoinRequest(const DiscordUser* /*request*/) {}

void CDiscord::Init()
{
    DiscordEventHandlers handlers = {};

    // handlers.ready = OnReady;
    // handlers.errored = OnError;
    // handlers.disconnected = OnDisconnected;
    // handlers.joinGame = OnJoinGame;
    // handlers.spectateGame = OnSpectateGame;
    // handlers.joinRequest = OnJoinRequest;

    Discord_Initialize(PJ64_DISCORD_APPID, &handlers, 1, NULL);
}

void CDiscord::Shutdown()
{
    Discord_Shutdown();
}

void CDiscord::Update(bool bHaveGame)
{
    DiscordRichPresence discordPresence = {};

    if (bHaveGame && g_Settings->LoadStringVal(Game_File).length() != 0)
    {
        char szState[256];
        sprintf(szState, "Playing %s", g_Settings->LoadStringVal(Rdb_GoodName).c_str());

        discordPresence.state = szState;
        discordPresence.startTimestamp = Timestamp();
    }

    discordPresence.largeImageKey = "icon";
    discordPresence.instance = 1;

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
