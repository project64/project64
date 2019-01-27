#include "stdafx.h"
#include <3rdParty\discord-rpc\include\discord_rpc.h>

class CDiscord
{
private:
    // static void OnReady(const DiscordUser* request);
    // static void OnError(int errorCode, const char* message);
    // static void OnDisconnected(int errorCode, const char* message);
    // static void OnJoinGame(const char* joinSecret);
    // static void OnSpectateGame(const char* spectateSecret);
    // static void OnJoinRequest(const DiscordUser* request);

    static int64_t Timestamp(void);

public:
    static void Init(void);
    static void Shutdown(void);
    static void Update(bool bHaveGame = true);
};
