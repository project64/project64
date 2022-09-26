#include "stdafx.h"
#include <3rdParty\discord-rpc\include\discord_rpc.h>

class CDiscord
{
private:
    static int64_t Timestamp(void);

public:
    static void Init(void);
    static void Shutdown(void);
    static void Update(bool bHaveGame = true);
};
