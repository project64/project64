#pragma once

#include "Project64-core\Plugins\ControllerPlugin.h"
#include "Common\path.h"


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

#define PACKET_TYPE_INPUT 1
#define PACKET_TYPE_CHEAT 2

// Automatic retransmit request
#define NETPLAY_ARQ_REQ		0x80000000
#define NETPLAY_ARQ_REPLY	0x40000000
#define NETPLAY_ARQ_MASK	0x3FFFFFFF
#define NETPLAY_ARQ_BOTH	0xC0000000

#define CODE_LENGTH 14 //8 for code, 1 for a space, 4 for value, 1 for trailing null

struct CKailleraPacket
{
	BYTE Type;
	union
	{
		char code[CODE_LENGTH];
		DWORD input;
	};
};

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
	CKaillera();
	~CKaillera();

	bool SetRomName(char* path);
	void clearGameList();
	void addGame(char *gameName, char *szFullFileName);
	void terminateGameList();
	void selectServerDialog(HWND hWnd);
	void modifyPlayValues(DWORD values);
	void GetPlayerKeyValuesFor1Player(BUTTONS &FKeys, int player);
	void UpdatePlayerKeyValues();
	void GetPlayerKeyValues(kPlayerEntry keyvalues[4], int player);
	//void modifyPlayValues(CODES c);
	void setInfos();

	int numberOfGames;
	char szKailleraNamedRoms[MAX_NUMBER_OF_GAMES * 2000];
	char szFullFileNames[MAX_NUMBER_OF_GAMES][2000];
	bool isPlayingKailleraGame;

	int playerNumber;
	int numberOfPlayers;

	DWORD getValues(int player);
	void StopKailleraThread();
	void endGame();

	void addCode(LPCSTR str);
	void delCode(LPCSTR str);
	LPCSTR getCode(int i);
	void sendDmaToSram(uint8_t * Source, int32_t StartOffset, int32_t len);
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
	void clearCodes();
	void sendCodes();
	int numCodes();
	void OnChatReceived(char *nick, char *text);
	void SetLagness(int lag);
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
	BOOL			kailleraAutoApplyCheat;

	BUTTONS Keys;

	bool DisableGameFixes;
	std::vector<stdstr>CheatList;
	std::vector<stdstr>ExtensionList;

	const char *GetGameName() const { return GameName; }
	stdstr GetGameIniKey() const { return GameIniKey; }
	void SetGameName(const char* name) { strcpy(GameName, name); }
	void SetGameIniKey(stdstr name) { GameIniKey = name; }

private:

	void sendResetCode();
	void sendLoadCode();
	void sendConfirmCode();

	kailleraInfos   kInfos;
	HMODULE KailleraHandle;
	int LoadKailleraFuncs();
	void processResult(CKailleraPacket ckp[]);
	char *pszKailleraNamedRoms;
	char *sAppName;
	//char *sAppName = "Project 64k 0.13 (01 Aug 2003)"; // CHANGE THIS
	DWORD values[4]; // for a maximum of 4 players
	std::vector<char*> codes;
	int playValuesLength;

	char *eepromBuf;
	char *sramBuf;
	char *mempakBuf;
	char *flashramBuf;

	char GameName[MAX_PATH];
	stdstr GameIniKey;
	uint32_t randomizer_seed;

	KailleraStateType KailleraState;
};