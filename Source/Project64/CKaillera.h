#pragma once

#include "Project64-core\Plugins\ControllerPlugin.h"
#include "Common\path.h"

class CMainMenu;

#define KAILLERA_CLIENT_API_VERSION "0.8"

#define MAX_NUMBER_OF_GAMES 1024

struct kailleraInfos
{
	char *appName;
	char *gameList;

	int (WINAPI *gameCallback)(char *game, int player, int numplayers);

	void (WINAPI *chatReceivedCallback)(char *nick, char *text);
	void (WINAPI *clientDroppedCallback)(char *nick, int playernb);

	void (WINAPI *moreInfosCallback)(char *gamename);
};

// Automatic retransmit request
#define NETPLAY_ARQ_REQ		0x80000000
#define NETPLAY_ARQ_REPLY	0x40000000
#define NETPLAY_ARQ_MASK	0x3FFFFFFF
#define NETPLAY_ARQ_BOTH	0xC0000000

typedef struct
{
	BUTTONS			b;
	uint32_t			viCount;
	unsigned char	optimalDelay;
} kPlayerEntry;

typedef enum
{
	DLL_NOT_LOADED,		// DLL is not loaded
	GAME_IDLE,			// Is idling
	TRANSFER_DATA,		// Game is starting, transfering data between players
	INPLAY,				// Game is playing
}KailleraStateType;

class CKaillera
{
public:
	CKaillera(CMainMenu* menu);
	~CKaillera();

	bool SetRomName(char* path);
	void clearGameList();
	void addGame(char *gameName, char *szFullFileName);
	void terminateGameList();
	void selectServerDialog(HWND hWnd);
	void GetPlayerKeyValuesFor1Player(BUTTONS &FKeys, int player);
	void UpdatePlayerKeyValues();
	void GetPlayerKeyValues(kPlayerEntry keyvalues[4], int player);
	void setInfos();

	int numberOfGames;
	char szKailleraNamedRoms[MAX_NUMBER_OF_GAMES * 2000];
	char szFullFileNames[MAX_NUMBER_OF_GAMES][2000];
	bool isPlayingKailleraGame;

	int playerNumber;
	int numberOfPlayers;

	void StopKailleraThread();
	void endGame();
	void ResetSaveFiles();
	void OnRomOpen();
	void GetFileName(CPath &filename, char* ext);
	void KailleraUploadFile(const char* filename, char* type);
	void DownloadFiles_SaveStrings(char *line);
	void UploadSaveFiles();
	void UploadRandomizerSeed();
	void UploadCheatCodes();
	template<class T> void UploadSetting(const char* format, T value);
	void UploadGameSettings();
	void OnChatReceived(char *nick, char *text);
	void SetLagness(int lag);
	void UploadLagness(int lag);
	void QueueLagness(int lag);
	int GetLagness() const { return kailleraLagness; }
	uint32_t GetRandomizerSeed() const { return randomizer_seed; }
	void SetRandomizerSeed(uint32_t seed) { randomizer_seed = seed; }

	volatile int	Kaillera_Thread_Keep_Running;
	volatile BOOL	Kaillera_Thread_Is_Running;
	HANDLE	kailleraThreadStopEvent;
	HANDLE	kailleraThreadEvent;
	HANDLE	kailleraThreadHandle;


#define KAILLERA_KEY_VALUE_QUETE_LEN	100
	kPlayerEntry	kailleraKeyQueues[4][KAILLERA_KEY_VALUE_QUETE_LEN];
	kPlayerEntry	kailleraLocalSendKeys[KAILLERA_KEY_VALUE_QUETE_LEN];
	BOOL			kailleraKeyValid[4][KAILLERA_KEY_VALUE_QUETE_LEN];
	BOOL			kailleraLocalSendKeysValid[KAILLERA_KEY_VALUE_QUETE_LEN];
	//int				kailleraKeyQueueIndex[4];
	kPlayerEntry	kBuffers[8];
	kPlayerEntry	savedKeyValues[4];
	BOOL			kailleraClientStatus[4];

	int				kailleraLagness;
	unsigned int	kVIdelay;
	unsigned int	kVIdelayToUse;
	unsigned int	kLastKeyUpdateVI;
	int				kKeyUpdateStep;

	DWORD			kailleraLastUpdateKeysAtVI;
	BUTTONS			kailleraKeyValuesToUpdate;

	BUTTONS Keys;

	bool DisableGameFixes;
	std::vector<stdstr>CheatList;
	std::vector<stdstr>ExtensionList;

	const char *GetGameName() const { return GameName; }
	stdstr GetGameIniKey() const { return GameIniKey; }
	void SetGameName(const char* name) { strcpy(GameName, name); }
	void SetGameIniKey(stdstr name) { GameIniKey = name; }

private:
	CMainMenu* mainmenu;

	kailleraInfos   kInfos;
	HMODULE KailleraHandle;
	int LoadKailleraFuncs();
	char *pszKailleraNamedRoms;
	char *sAppName;
	DWORD values[4]; // for a maximum of 4 players
	std::vector<char*> codes;
	int playValuesLength;
	int lagqueue;

	char *eepromBuf;
	char *sramBuf;
	char *mempakBuf;
	char *flashramBuf;

	char GameName[MAX_PATH];
	stdstr GameIniKey;
	uint32_t randomizer_seed;

	KailleraStateType KailleraState;
};

extern void(__stdcall* kailleraChatSend)  (char *text);