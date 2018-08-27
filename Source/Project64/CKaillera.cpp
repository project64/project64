#include "stdafx.h"
#include <time.h>
#include "Common\MemoryManagement.h"

const char* RESET = "00000000 0001";
const char* LOAD = "00000000 0002";
const char* CONFIRM = "00000000 0000";

void(__stdcall* kailleraGetVersion) (char *version);
void(__stdcall* kailleraInit) ();
void(__stdcall* kailleraShutdown) ();
void(__stdcall* kailleraSetInfos) (kailleraInfos *infos);
void(__stdcall* kailleraSelectServerDialog) (HWND parent);
int(__stdcall* kailleraModifyPlayValues)  (void *values, int size);
void(__stdcall* kailleraChatSend)  (char *text);
void(__stdcall* kailleraEndGame) ();

extern CKaillera *ck;

void(__stdcall StartKailleraThread)(CKaillera *pKaillera);

void ShowError(char* str)
{
	MessageBox(NULL, str, str, NULL);
}

void NotificationCB(const char * Status, CN64Rom * /*_this*/)
{
	g_Notify->DisplayMessage(5, stdstr_f("%s", Status).c_str());
}

bool CKaillera::SetRomName(char* path)
{
	char* FileLoc = path;
	stdstr ext = CPath(FileLoc).GetExtension();
	bool Loaded7zFile = false;
	CN64Rom *rom = new CN64Rom;
	rom->SetErrorPublic(EMPTY_STRING);
	const bool LoadBootCodeOnly = true;

#ifdef _WIN32
	if (strstr(FileLoc, "?") != NULL || _stricmp(ext.c_str(), "7z") == 0)
	{
		stdstr FullPath = FileLoc;

		//this should be a 7zip file
		char * SubFile = strstr(const_cast<char*>(FullPath.c_str()), "?");
		if (SubFile == NULL)
		{
			return false;
		}
		else
		{
			*SubFile = '\0';
			SubFile += 1;
		}

		C7zip ZipFile(FullPath.c_str());
		ZipFile.SetNotificationCallback((C7zip::LP7ZNOTIFICATION)NotificationCB, this);
		for (int i = 0; i < ZipFile.NumFiles(); i++)
		{
			CSzFileItem * f = ZipFile.FileItem(i);
			if (f->IsDir)
			{
				continue;
			}

			stdstr ZipFileName;
			ZipFileName.FromUTF16(ZipFile.FileNameIndex(i).c_str());
			if (SubFile != NULL)
			{
				if (_stricmp(ZipFileName.c_str(), SubFile) != 0)
				{
					continue;
				}
			}

			//Get the size of the rom and try to allocate the memory needed.
			uint32_t RomFileSize = (uint32_t)f->Size;
			//if loading boot code then just load the first 0x1000 bytes

			if (LoadBootCodeOnly) { RomFileSize = 0x1000; }

			if (!rom->AllocateRomImagePublic(RomFileSize))
			{
				return false;
			}

			//Load the n64 rom to the allocated memory
			g_Notify->DisplayMessage(5, MSG_LOADING);
			if (!ZipFile.GetFile(i, rom->GetRomAddress(), RomFileSize))
			{
				return false;
			}

			if (!rom->IsValidRomImage(rom->GetRomAddress()))
			{
				return false;
			}
			g_Notify->DisplayMessage(5, MSG_BYTESWAP);
			rom->ByteSwapRomPublic();

			//Protect the memory so that it can not be written to.
			ProtectMemory(rom->GetRomAddress(), rom->GetRomSize(), MEM_READONLY);
			Loaded7zFile = true;
			break;
		}
		if (!Loaded7zFile)
		{
			return false;
		}
	}
#endif

	//Try to open the file as a zip file
	if (!Loaded7zFile)
	{
		if (!rom->AllocateAndLoadZipImagePublic(FileLoc, LoadBootCodeOnly))
		{
			if (rom->GetError() != EMPTY_STRING)
			{
				return false;
			}
			if (!rom->AllocateAndLoadN64ImagePublic(FileLoc, LoadBootCodeOnly))
			{
				return false;
			}
		}
	}

	char RomName[260];
	//Get the header from the rom image
	memcpy(&RomName[0], (void *)(rom->GetRomAddress() + 0x20), 20);
	CN64Rom::CleanRomName(RomName);

	if (strlen(RomName) == 0)
	{
		strcpy(RomName, CPath(FileLoc).GetName().c_str());
		CN64Rom::CleanRomName(RomName, false);
	}

	SetGameName(RomName);
	stdstr m_RomIdent;
	m_RomIdent.Format("%08X-%08X-C:%X", *(uint32_t *)(&rom->GetRomAddress()[0x10]), *(uint32_t *)(&rom->GetRomAddress()[0x14]), rom->GetRomAddress()[0x3D]);
	SetGameIniKey(m_RomIdent);

	delete rom;
	return true;
}

int WINAPI kailleraGameCallback(char *game, int player, int numplayers)
{
	ck->playerNumber = player-1; // since our player #1 is in index zero
	ck->numberOfPlayers = numplayers;

	// find game in local list and run it based off of the full path
	char *temp = ck->szKailleraNamedRoms;

	for (int x = 0; x < ck->numberOfGames; x++)
	{
		if (strncmp(temp, game, strlen(temp)) == 0)
		{
			temp = ck->szFullFileNames[x];
			break;
		}
		temp += strlen(temp) + 1;
	}

	Notify().BringToTop();
	//g_Notify->BringToTop();

	ck->isPlayingKailleraGame = true;
	ck->clearCodes();
	if (!ck->SetRomName(temp))
	{
		MessageBoxA(NULL, "Kaillera failed to set rom name", "Project64", MB_OK);
		return 1;
	}

	if (numplayers>4)
		numplayers = 4;	//N64 supports up to 4 players

	ck->kailleraClientStatus[0] = ck->kailleraClientStatus[1] = ck->kailleraClientStatus[2] = ck->kailleraClientStatus[3] = FALSE;
	for (int i = 0; i< numplayers; i++)
	{
		ck->kailleraClientStatus[i] = TRUE;
	}

	ck->OnRomOpen();

	g_BaseSystem->RunFileImage(temp);

	return 0;
}

void CKaillera::SetLagness(int lag)
{
	kailleraLagness = lag;
	kVIdelay = lag * 3;
}

/************************************************************************/
/* We are using ChatSend and ChatReceiveCallBack to transfer data       */
/* between players. See the KailleraRomOpen() function comment about    */
/* data to be transferred.                                              */
/************************************************************************/
void WINAPI kailleraChatReceivedCallback(char *nick, char *text)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : <%s> : %s", nick, text);

	ck->OnChatReceived(nick, text);
}

void WINAPI kailleraClientDroppedCallback(char *nick, int playernb)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : <%s> dropped (%d)", nick, playernb);
	if ((playernb - 1) == ck->playerNumber) // that means we dropped, so stop emulation!
	{
		ck->endGame();
		g_BaseSystem->CloseSystem();
		ck->isPlayingKailleraGame = false;
	}
}

void CKaillera::OnChatReceived(char *nick, char *text)
{
	if (KailleraState == TRANSFER_DATA || (KailleraState == INPLAY && g_System->GetVITotalCount() == 0))
	{
		/************************************************************************/
		/* Data format:                                                         */
		/*    !Setting=3,....    (CF,and then other setting)                    */
		/*    !Cheats=10                                                        */
		/*    !Cheat#1=803233230320,809832331023,....                           */
		/*    !Cheat#2=.....                                                    */
		/*    !SaveType=eep(or sra, fla, mpk)                                   */
		/*    !SaveLines=15                                                     */
		/*    !Save#1=xkdksjeidj;adjdjs;akljdks (encoded string)                */
		/************************************************************************/

		if (text[0] == '!' && playerNumber != 0)
		{
			FILE *stream = NULL;
			char *line = text + 1;

			if (strncmp(line, "cf=", 3) == 0)
			{
				/*
				INI_ENTRY *c = &currentromoptions;
				sscanf(line, "cf=%d,32bit=%d,reg=%d,fpu=%d,rdram=%d,save=%d,tlb=%d,eeprom=%d,2pass=%d,rsp=%d,link=%d",
					&(c->Counter_Factor), &(c->Assume_32bit), &(c->Use_Register_Caching), &(c->FPU_Hack), &(c->RDRAM_Size), &(c->Save_Type),
					&(c->Use_TLB), &(c->Eeprom_size), &(c->Advanced_Block_Analysis), &(c->RSP_RDP_Timing), &(c->Link_4KB_Blocks));
				CounterFactor = currentromoptions.Counter_Factor;
#ifdef _DEBUG
				currentromoptions.Assume_32bit = ASSUME_32BIT_NO;
#endif
				*/
			}
			else if (strncmp(line, "EnableGameFixes", 15) == 0)
			{
				DisableGameFixes = false;
			}
			else if (strncmp(line, "DisableGameFixes", 16) == 0)
			{
				DisableGameFixes = true;
			}
			else if (strncmp(line, "CheatStart", 10) == 0)
			{
				CheatList.clear();
			}
			else if (strncmp(line, "CheatEnd", 8) == 0)
			{
				//if (playerNumber != 0)
				//	kailleraAutoApplyCheat = TRUE;
			}
			else if (strncmp(line, "ExtensionStart", 14) == 0)
			{
				ExtensionList.clear();
			}
			else if (strncmp(line, "ExtensionEnd", 12) == 0)
			{

			}
			else if (strncmp(line, "Ext", 3) == 0)
			{
				char* extension = line + 3;
				stdstr ext;
				if (strncmp(extension, "BLANK", 5) == 0)
					ext.clear();
				else
					ext = stdstr(extension);

				ExtensionList.push_back(ext);
			}
			else if (strncmp(line, "Cheat=", 6) == 0)
			{
				CheatList.push_back(line);
			}
			else if (strncmp(line, "!!StartGame!!!", 14) == 0)
			{
				KailleraState = INPLAY;
			}
			else if (strncmp(line, "RandomizerSeed=", 15) == 0)
			{
				char* seed = line + 15;
				uint32_t received_seed = strtoul(seed, NULL, 10);
				char received[20];
				sprintf(received, "Received seed: %u", received_seed);
				SetRandomizerSeed(received_seed);
				kailleraChatSend(received);
			}
			else if (strncmp(text, "!EEPROM", 7) == 0)
			{
				DownloadFiles_SaveStrings(text);
			}
			else if (strncmp(text, "!SRAM", 5) == 0)
			{
				DownloadFiles_SaveStrings(text);
			}
			else if (strncmp(text, "!FLASH", 6) == 0)
			{
				DownloadFiles_SaveStrings(text);
			}
			else if (strncmp(text, "!MEMPAK", 7) == 0)
			{
				DownloadFiles_SaveStrings(text);
			}
		}
		else if (strncmp(text, "!!!StartGame!!!", 15) == 0)
		{
			KailleraState = INPLAY;
		}
	}
	else
	{
		if (strncmp(text, "* Player 1", 10) == 0 && strstr(text, "dropped from the current game.") != 0)
		{
			kailleraClientDroppedCallback("Player 1", 1);
		}

	}
}

void WINAPI kailleraMoreInfosCallback(char *gamename)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : MoreInfosCallback %s ", gamename);
}

CKaillera::CKaillera()
{
	sAppName = "Project 64k Core 2.3";
	kInfos.appName = sAppName;
	kInfos.gameList = szKailleraNamedRoms;
	kInfos.gameCallback = kailleraGameCallback;
	kInfos.chatReceivedCallback = kailleraChatReceivedCallback;
	kInfos.clientDroppedCallback = kailleraClientDroppedCallback;
	kInfos.moreInfosCallback = kailleraMoreInfosCallback;

	LoadKailleraFuncs();

	kailleraInit();

	eepromBuf = (char*)malloc(256 * 1024);
	sramBuf = (char*)malloc(256 * 1024);
	mempakBuf = (char*)malloc(256 * 1024);
	flashramBuf = (char*)malloc(256 * 1024);

	isPlayingKailleraGame = false;
	numberOfGames = playerNumber = numberOfPlayers = 0;
	memset(values, 0, sizeof(values));
	playValuesLength = 0;
	kailleraLagness = 4;
	kVIdelay = 12;
	kVIdelayToUse = 12;
	kLastKeyUpdateVI = 0;
	kKeyUpdateStep = 3;
	KailleraState = DLL_NOT_LOADED;
	kailleraLastUpdateKeysAtVI = 0;
	kailleraAutoApplyCheat = FALSE;

	Kaillera_Thread_Keep_Running = FALSE;
	Kaillera_Thread_Is_Running = FALSE;
	kailleraThreadHandle = NULL;

	kailleraThreadStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	kailleraThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CKaillera::~CKaillera()
{
	kailleraShutdown();
	if (kailleraThreadStopEvent)
		CloseHandle(kailleraThreadStopEvent);
	if (kailleraThreadEvent)
		CloseHandle(kailleraThreadEvent);
}

void CKaillera::clearGameList()
{
	// clear the list
	pszKailleraNamedRoms = szKailleraNamedRoms;

	memset(szKailleraNamedRoms, 0, sizeof(szKailleraNamedRoms));
	memset(szFullFileNames, 0, sizeof(szFullFileNames));

	// seed it with the two basic rooms
	addGame("*Chat (not game)", " ");
	addGame("*Away (leave messages)", " ");

	numberOfGames = 2;
}

void CKaillera::addGame(char *gameName, char *szFullFileName)
{
	strncpy(pszKailleraNamedRoms, gameName, strlen(gameName) + 1);
	pszKailleraNamedRoms += strlen(gameName) + 1;

	strncpy(szFullFileNames[numberOfGames], szFullFileName, strlen(szFullFileName) + 1);

	numberOfGames++;
}

void CKaillera::terminateGameList()
{
	*(++pszKailleraNamedRoms) = '\0';
}

void CKaillera::setInfos()
{
	kailleraSetInfos(&kInfos);
}

void CKaillera::selectServerDialog(HWND hWnd)
{
	kailleraSelectServerDialog(hWnd);
}

/************************************************************************/
/* The emulator thread will call this function to read players key      */
/* values. If Kaillera is running, emulator thread should not use       */
/* controller plugin functions anymore, instead, Kaillera functions     */
/* should be used even for the local player. Kaillera functions will    */
/* manage all player control inputs - even for the local player         */
/************************************************************************/
void CKaillera::GetPlayerKeyValues(kPlayerEntry keyvalues[4], int player)
{
	// We have 4 queues
	// delay up to 10 VI

	//kPlayerEntry	kailleraKeyQueues[4][KAILLERA_KEY_VALUE_QUETE_LEN];
	//int			kailleraKeyQueueIndex[4];
	//kPlayerEntry	kBuffers[8];

	unsigned int VItouse;
	int idx;

	if (kailleraClientStatus[player] == FALSE)//&& (player == kailleraLocalPlayerNumber )
	{
		g_Plugins->Control()->GetKeys(0, &keyvalues[player].b); // Read key status of the local player
		return;
	}

	if (g_System->GetVITotalCount() < kVIdelay || kailleraClientStatus[player] == FALSE)
	{
		memset(&keyvalues[player].b, 0, sizeof(BUTTONS));
		return;
	}

	VItouse = (g_System->GetVITotalCount() - kVIdelayToUse) / kKeyUpdateStep*kKeyUpdateStep;
	//DEBUG_NETPLAY_MACRO(TRACE3("Read player %d at VI %d by syncVI(%d)", player, g_System->GetVITotalCount(), VItouse));

CheckKeyValuesAgain:
	if (Kaillera_Thread_Is_Running == FALSE || Kaillera_Thread_Keep_Running == FALSE)
	{
		if (player == playerNumber)
		{
			g_Plugins->Control()->GetKeys(0, &keyvalues[player].b);	// Read key status of the local player
		}
		else
		{
			memset(&keyvalues[player].b, 0, sizeof(BUTTONS));
		}
		return;
	}

	idx = VItouse / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;

	if ((kailleraKeyValid[player][idx] == FALSE || kailleraKeyQueues[player][idx].viCount != VItouse) && kailleraClientStatus[player])
	{
		//DEBUG_NETPLAY_MACRO(TRACE2("Wait for player %d to sync to key VI=%d", player, VItouse));
		Sleep(4);
		goto CheckKeyValuesAgain;
	}

	//DEBUG_NETPLAY_MACRO(TRACE4("Player %d get key=%X, VI (%d), syncVI (%d)", player, kailleraKeyQueues[player][idx].b.Value, viTotalCount, kailleraKeyQueues[player][idx].viCount));
	//KAILLERA_LOG(fprintf(ktracefile, "P%d get key %08X at VI %d by syncVI(%d), key VI count=%d\n", player, kailleraKeyQueues[player][idx].b.Value, viTotalCount, VItouse, kailleraKeyQueues[player][idx].viCount));
	memcpy(keyvalues + player, kailleraKeyQueues[player] + idx, sizeof(kPlayerEntry));
}

void CKaillera::GetPlayerKeyValuesFor1Player(BUTTONS& FKeys, int player)
{
	BUTTONS DummyKeys;

	if (Kaillera_Thread_Is_Running)
	{
		GetPlayerKeyValues(savedKeyValues, player);
	}

	memcpy(&FKeys, &savedKeyValues[player].b, sizeof(BUTTONS));

	// apparently if you don't have these calls, the emulator freaks out and ignores all input?
	//g_Plugins->Control()->GetKeys(1, &DummyKeys);
	//g_Plugins->Control()->GetKeys(2, &DummyKeys);
	//g_Plugins->Control()->GetKeys(3, &DummyKeys);
}

/************************************************************************/
/* This function is called by the Kaillera thread infinite loop to      */
/* perodically update the players values:                               */
/*    -> Update local player key values to Kaillera server              */
/*    -> Read the other players' latest key values from Kaillera server */
/*    -> Sync all players                                               */
/*    -> Lost packet detection and ARQ                                  */
/************************************************************************/
kPlayerEntry LastEntries[4];
void CKaillera::UpdatePlayerKeyValues()
{
	// Called in the Kaillera thread
	int		reclen;
	int		i;
	DWORD signal;
	unsigned int VItouse;
	kPlayerEntry *entry;
	int idx;
	BUTTONS DummyKeys;

	kLastKeyUpdateVI += kKeyUpdateStep;
	g_Plugins->Control()->GetKeys(0, &Keys);		// Read key status of the local player
	for (int i = 1; i < 4; i++)
	{
		g_Plugins->Control()->GetKeys(i, &DummyKeys);		// Read key status of other controllers
	}

	memcpy(&kBuffers[0].b, &Keys, sizeof(BUTTONS));

label_Jump:
	signal = WaitForSingleObject(kailleraThreadStopEvent, 0);
	if (signal == WAIT_OBJECT_0)
	{
		SetEvent(kailleraThreadStopEvent);
		return;
	}


	kBuffers[0].viCount = kLastKeyUpdateVI;
	kBuffers[0].optimalDelay = kVIdelay;
	idx = kLastKeyUpdateVI / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
	memcpy(&kailleraLocalSendKeys[idx], &kBuffers[0], sizeof(kPlayerEntry));
	kailleraLocalSendKeysValid[idx] = TRUE;

	/*
	if( rand() < 0x400 )
	{
	// to simulate package lose
	// Disable here in formal release
	int badplayer = rand()%4;
	idx = kBuffers[badplayer].viCount/kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
	idx--;
	if( idx < 0 ) idx=0;
	memcpy(&kBuffers[badplayer], &kailleraKeyQueues[badplayer][idx], sizeof(kPlayerEntry));
	}
	*/

	// Update local player key values to server and get key values of other players
	reclen = kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));

check_again:
	if (memcmp(&kBuffers, &LastEntries, sizeof(LastEntries)) == 0)
	{
		Sleep(2);
		goto label_Jump;
	}
	else
	{
		memcpy(&LastEntries, &kBuffers, sizeof(LastEntries));
	}

	if (reclen == -1)
	{
		//SetStatusBarText(0, TranslateStringByString("Kaillera timeout"));
		MessageBoxA(NULL, "Kaillera timeout", "Kaillera", MB_OK);
		//DEBUG_NETPLAY_TRACE0("Kaillera timeout");
		SetEvent(kailleraThreadStopEvent);
		Kaillera_Thread_Keep_Running = FALSE;
		return;
	}
	else if (reclen > 0)
	{
		DWORD minVI = 0xFFFFFFFF;
		BOOL  ARQRequest = FALSE;

		// Check optimal delay values
		kVIdelayToUse = kVIdelay;
		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i])
			{
				if (kBuffers[i].optimalDelay > kVIdelay)
				{
					kVIdelayToUse = kBuffers[i].optimalDelay;
				}
			}
		}

		VItouse = (g_System->GetVITotalCount() - kVIdelayToUse) / kKeyUpdateStep*kKeyUpdateStep;	// emulator thread will try to sync at this VIcount


																					// Store the keys in the queue
		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i] && (kBuffers[i].viCount&NETPLAY_ARQ_BOTH) == 0)
			{
				idx = kBuffers[i].viCount / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
				entry = &(kailleraKeyQueues[i][idx]);
				if (entry->viCount != kBuffers[i].viCount || entry->viCount == 0 || kailleraKeyValid[i][idx] == FALSE)
				{
					entry->b.Value = kBuffers[i].b.Value;
					entry->optimalDelay = kBuffers[i].optimalDelay;
					entry->viCount = kBuffers[i].viCount;
					kailleraKeyValid[i][idx] = TRUE;
				}

			}
		}


		// Check ARQ request
		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i])
			{
				// Check ARQ first

				// Is this player request an earlier packet
				if ((kBuffers[i].viCount&NETPLAY_ARQ_REQ) != 0 && playerNumber == kBuffers[i].b.Value)
				{
					// The ARQ request is for this local player

					int arq_vicount = kBuffers[i].viCount&NETPLAY_ARQ_MASK;
					idx = arq_vicount / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;

					if (arq_vicount == kailleraLocalSendKeys[idx].viCount && kailleraLocalSendKeysValid[idx])
					{
						memcpy(&kBuffers[0], &kailleraLocalSendKeys[idx], sizeof(kPlayerEntry));
						kBuffers[0].viCount |= NETPLAY_ARQ_REPLY;

						//DEBUG_NETPLAY_TRACE1("To reply ARQ for syncVI=%d", arq_vicount);
						//KAILLERA_LOG(fprintf(ktracefile, "P%d to reply ARQ for VI=%d, key=%08X\n", kailleraLocalPlayerNumber, arq_vicount, kBuffers[0].b.Value));
						kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
						Sleep(6);

						goto check_again;

						break;	// Ignore request from all other players
					}
					else
					{
						//DEBUG_NETPLAY_TRACE1("Error: ARQ for VI=%d, but haven't get there yet", arq_vicount);
					}
				}
			}
		}

		// Check ARQ reply
		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i])
			{
				if ((kBuffers[i].viCount & NETPLAY_ARQ_REPLY) != 0)	// Is this a ARQ reply 
				{
					uint32_t tempcount = (kBuffers[i].viCount&NETPLAY_ARQ_MASK);
					idx = tempcount / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;

					// save the ARQ reply
					entry = &(kailleraKeyQueues[i][idx]);
					if (entry->viCount != tempcount || entry->viCount == 0 || kailleraKeyValid[i][idx] == FALSE)
					{
						entry->b.Value = kBuffers[i].b.Value;
						entry->optimalDelay = kBuffers[i].optimalDelay;
						entry->viCount = tempcount;
						kailleraKeyValid[i][idx] = TRUE;
						//KAILLERA_LOG(fprintf(ktracefile, "P%d replied ARQ for VI=%d, key=%08X\n", i, tempcount, kBuffers[i].b.Value));
					}

					//DEBUG_NETPLAY_TRACE1("ARQ replied at syncVI = %d", tempcount);
				}
			}
		}

		// Check if we need to send ARQ
		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i] && (kBuffers[i].viCount & NETPLAY_ARQ_BOTH) == 0)
			{
				// Need to search backward 10 record to see if we have missed some keys values
				// while later values have been received successfully.
				{
					// ARQ check
					int k;
					int viToCheck = kBuffers[i].viCount;

					if (viToCheck - VItouse > 2 * kKeyUpdateStep)
						viToCheck -= kKeyUpdateStep;	// Don't check 1 step below, UDP packet may arrive slightly out of order

					for (k = 0; k<10; k++)
					{
						viToCheck -= kKeyUpdateStep;

						if (viToCheck < VItouse)	// Don't need to request key for VIs that we have passed
							break;

						if (viToCheck >= kKeyUpdateStep)
						{
							int idx2 = viToCheck / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
							if (kailleraKeyValid[i][idx2] == FALSE || kailleraKeyQueues[i][idx2].viCount != viToCheck)
							{
								if (i == playerNumber)
								{
									// lost packet from myself
									int idx3 = viToCheck / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
									memcpy(&kBuffers[0], &kailleraLocalSendKeys[idx3], sizeof(kPlayerEntry));
									kBuffers[0].viCount |= NETPLAY_ARQ_REPLY;
									//DEBUG_NETPLAY_TRACE1("To resend Keys for syncVI=%d", viToCheck);
									kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
									Sleep(3);
									goto check_again;
								}
								else
								{
									// we haven't receive key for VIcount = viToCheck;
									// Need to request ARQ from player #i
									kBuffers[0].b.Value = i;
									kBuffers[i].viCount = viToCheck | NETPLAY_ARQ_REQ;

									//DEBUG_NETPLAY_MACRO(TRACE3("ARQ P%d for syncVI = %d, Got (%08X)", i, viToCheck, kBuffers[i].viCount));
									//KAILLERA_LOG(fprintf(ktracefile, "P%d ARQ to P%d for syncVI=%d\n", kailleraLocalPlayerNumber, i, viToCheck));

									kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
									ARQRequest = TRUE;
									Sleep(50);
									goto check_again;
								}

							}
						}
					}
				}
			}
		}


		for (i = 0; i < numberOfPlayers; i++)
		{
			if (kailleraClientStatus[i] && (kBuffers[i].viCount&NETPLAY_ARQ_BOTH) == 0)
			{
				// This is normal reply
				if (kLastKeyUpdateVI > kVIdelayToUse + kKeyUpdateStep && kBuffers[i].viCount < (kLastKeyUpdateVI - kVIdelayToUse + kKeyUpdateStep) / kKeyUpdateStep*kKeyUpdateStep)
				{
					// Sync vi to at least "kLastKeyUpdateVI-kVIdelayToUse"
					Sleep(2);
					goto label_Jump;
				}

				if (VItouse > kVIdelayToUse + kKeyUpdateStep && VItouse < kLastKeyUpdateVI && kBuffers[i].viCount < VItouse)
				{
					// Sync vi to at least "kLastKeyUpdateVI-kVIdelayToUse"
					Sleep(2);
					goto label_Jump;
				}

				if (minVI>kBuffers[i].viCount)
					minVI = kBuffers[i].viCount;
			}
		}

		for (i = 0; i < numberOfPlayers; i++)
		{
			// Check again from the point of minVI
			if (kailleraClientStatus[i])
			{
				int k;
				int viToCheck = minVI;

				viToCheck -= kKeyUpdateStep;	// Don't check 1 step below, UDP packet may arrive slightly out of order

				for (k = 0; k<10; k++)
				{
					if (viToCheck >= 0 && viToCheck >= VItouse)
					{
						int idx3 = viToCheck / kKeyUpdateStep%KAILLERA_KEY_VALUE_QUETE_LEN;
						if (kailleraKeyValid[i][idx3] == FALSE || kailleraKeyQueues[i][idx3].viCount != viToCheck)
						{
							Sleep(4);
							goto label_Jump;
						}
					}
				}

			}
		}


		if (minVI == 0xFFFFFFFF)
		{
			Sleep(4);
			goto label_Jump;
		}

		// Update the latest key values from the server to my key value queues
		//DEBUG_NETPLAY_TRACE1("Kailler synced at syncVI = %d", minVI);
	}
	else	// reclen == 0
	{
		Sleep(2);
		goto label_Jump;
	}
}

void CKaillera::UploadRandomizerSeed()
{
	char seedstr[64];
	uint32_t seed = (uint32_t)time(NULL);
	sprintf(seedstr, "!RandomizerSeed=%u", seed);

	SetRandomizerSeed(seed);
	kailleraChatSend(seedstr);
}

void CKaillera::UploadCheatCodes()
{
	CCheats    cheats(CMipsMemoryVM(0));
	g_Settings->SaveString(Game_IniKey, GetGameIniKey());

	//Todo: set gamefix flag
	//Send gamefixes
	//Send cheats

	std::vector<stdstr>cheatlist;
	std::vector<stdstr>extensionlist;

	if (g_Settings->LoadBool(Debugger_DisableGameFixes))
		kailleraChatSend("!DisableGameFixes");
	else
		kailleraChatSend("!EnableGameFixes");

	bool DisableSelected = !g_Settings->LoadDword(Setting_RememberCheats);

	//if (DisableSelected)
	//	kailleraChatSend("!NoStartupCheats");

	for (int CheatNo = 0; CheatNo < cheats.MaxCheats; CheatNo++)
	{
		stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo);
		if (LineEntry.empty()) { break; }
		if (!g_Settings->LoadBoolIndex(Cheat_Active, CheatNo))
		{
			continue;
		}
		if (DisableSelected)
		{
			g_Settings->SaveBoolIndex(Cheat_Active, CheatNo, false);
			continue;
		}
		
		cheatlist.push_back(LineEntry);

		stdstr Extension;
		if (!g_Settings->LoadStringIndex(Cheat_Extension, CheatNo, Extension))
		{
			Extension.clear();
		}
		extensionlist.push_back(Extension);
		//LoadCode(LineEntry, Cheat_Extension, CheatNo);
	}

	kailleraChatSend("!CheatStart");

	for (auto&& cheat : cheatlist)
	{
		kailleraChatSend((char*)(stdstr("!") + cheat).c_str());
	}

	kailleraChatSend("!CheatEnd");

	kailleraChatSend("!ExtensionStart");

	for (auto&& extension : extensionlist)
	{
		stdstr extensionstr = !extension.empty() ? extension : "BLANK";
		kailleraChatSend((char*)(stdstr("!Ext") + extensionstr).c_str());
	}

	kailleraChatSend("!ExtensionEnd");
}

/************************************************************************/
/* If the local player is not the player 0 (the player in control)      */
/* game load/save will be done over dummy files, not the regular save   */
/* files for the game on this local computer. Otherwise game will be    */
/* out of sync if the save states are different across the netplay.     */
/* In the further, we want to automatically transfer the save file from */
/* the player #0 to all other players to overwrite the dummy save file. */
/************************************************************************/
void CKaillera::ResetSaveFiles()
{
	/************************************************************************/
	/* Need to delete (if exist) and create new dummy save files            */
	/* EEPROM-4KB, EEPROM-16KB, MEMPAK, SRAM, FlashRAM                      */
	/************************************************************************/

	CPath FileName(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), "kaillera.eep"/*stdstr_f("%s.eep", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str()*/);
	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}
	FileName.Delete();

	FileName = CPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), "kaillera.mpk"/*stdstr_f("%s.eep", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str()*/);
	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}
	FileName.Delete();

	FileName = CPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), "kaillera.sra"/*stdstr_f("%s.eep", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str()*/);
	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}
	FileName.Delete();

	FileName = CPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), "kaillera.fla"/*stdstr_f("%s.eep", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str()*/);
	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}
	FileName.Delete();
}

void CKaillera::GetFileName(CPath &filename, char* ext)
{
	// See comments for function ResetSaveFiles in CKaillera.cpp
	char* gamename = (!isPlayingKailleraGame || playerNumber == 0) ? GetGameName() : "kaillera";
	char file[MAX_PATH];
	sprintf(file, "%s.%s", gamename, ext);
	filename = CPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), file);
	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		filename.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}

	//char test[1024];
	//sprintf(test, "num %i %s", playerNumber, std::string(filename).c_str());
	//MessageBoxA(NULL, test, "", MB_OK);
}

char base64[64] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

int Decode64(char *input, char *output)
{
	int x;
	int y;
	int group = 0;
	int g = 18;
	char *savepos = output;

	for (x = 0; x < strlen(input); x++)
	{
		for (y = 0; y < 64; y++)
		{
			if (base64[y] == input[x])
				break;
		}

		if (y != 64)
			group = (group | (y << g));

		g -= 6;
		if (g  < 0)
		{
			*output++ = (char)((group & 0xff0000) >> 16);
			*output++ = (char)((group & 0xff00) >> 8);
			*output++ = (char)((group & 0xff));
			group = 0;
			g = 18;
		}
	}

	*output = 0;

	return output - savepos;
}


void Encode64(char *input, char *output, int len)
{
	int   state = 0;
	int   i, x, old, new1, new2;

	for (i = 0; i < len; i++)
	{
		state++;
		new2 = 32;
		switch (state)
		{
		case 1: x = input[i];
			new1 = base64[((x >> 2) & 0x3F)];
			*output++ = (char)new1;
			break;
		case 2: x = input[i];
			new1 = base64[(((old << 4) & 0x30) | (((x >> 4) & 0xF)))];
			*output++ = (char)new1;
			break;
		case 3: x = input[i];
			new1 = base64[(((old << 2) & 0x3C) | (((x >> 6) & 0x3)))];
			new2 = base64[(x & 0x3F)];
			*output++ = (char)new1;
			*output++ = (char)new2;
			state = 0;
			break;
		} /* end of switch statement */
		old = x;
	}

	switch (state)
	{
	case 0:
		break;
	case 1:
		new1 = base64[((old << 4) & 0x30)];
		*output++ = (char)new1;
		*output++ = '=';
		*output++ = '=';
		break;
	case 2: x = input[i];
		new1 = base64[((old << 2) & 0x3C)];
		*output++ = (char)new1;
		*output++ = '=';
		break;
	}

	*output = 0;
}

void CKaillera::KailleraUploadFile(const char* filename, char* type)
{
	char *buf = new char[256 * 1024];
	char *zipbuf = new char[256 * 1024];
	unsigned long ziplen;
	char *outbuf = new char[256 * 1024];
	FILE *fp = fopen(filename, "rb");

	if (fp)
	{
		LONG len;

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (len > 0)
		{
			fread(buf, len, 1, fp);
			ziplen = 256 * 1024;
			compress((Bytef*)zipbuf, &ziplen, (const Bytef*)buf, len);
			Encode64(zipbuf, outbuf, ziplen);
			len = strlen(outbuf);

			{
				int num = len / 100;
				int i;

				for (i = 0; i<num; i++)
				{
					sprintf(buf, "!%s%d=", type, i);
					strncat(buf, outbuf + i * 100, 100);
					kailleraChatSend(buf);
				}

				if (len % 100 != 0)
				{
					sprintf(buf, "!%s%d=", type, i);
					strcat(buf, outbuf + i * 100);
					kailleraChatSend(buf);
				}

				sprintf(buf, "!%sEND", type);
				kailleraChatSend(buf);
			}
		}
		fclose(fp);
	}
	delete buf;
	delete zipbuf;
	delete outbuf;
}

void CKaillera::UploadSaveFiles()
{
	if (playerNumber == 0)
	{
		CPath filename;
		GetFileName(filename, "eep");
		if (filename.Exists())
		{
			// Upload EEPROM file
			KailleraUploadFile(std::string(filename).c_str(), "EEPROM");
		}

		GetFileName(filename, "sra");
		if (filename.Exists())
		{
			// Upload SRAM file
			KailleraUploadFile(std::string(filename).c_str(), "SRAM");
		}

		GetFileName(filename, "fla");
		if (filename.Exists())
		{
			// Upload FlashRAM file
			KailleraUploadFile(std::string(filename).c_str(), "FLASH");
		}

		const char* gamename = (!isPlayingKailleraGame || playerNumber == 0) ? GetGameName() : "kaillera";
		char file[MAX_PATH];
		sprintf(file, "%s_Cont_1.mpk", gamename);
		filename = CPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), file);
		if (g_Settings->LoadBool(Setting_UniqueSaveDir))
		{
			filename.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
		}

		if (filename.Exists())
		{
			// Upload Mempak file
			KailleraUploadFile(std::string(filename).c_str(), "MEMPAK");
		}
	}
}

void CKaillera::OnRomOpen()
{
	int i;
	KailleraState = TRANSFER_DATA;

	ResetSaveFiles();
	//g_System->ResetVITotalCount();

	for (i = 0; i<20; i++)
	{
		// To clear the buffer stored in the Kaillera
		memset(&kBuffers[0], 0, sizeof(kPlayerEntry));
		kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
		//Sleep(40);
	}

	// Broadcast current rom settings to all other players
	if (playerNumber == 0)
	{
		UploadRandomizerSeed();
		UploadCheatCodes();
		UploadSaveFiles();
	
		//Sleep(200);
	}

	if (playerNumber == 0)
	{
		//SetStatusBarText(0, TranslateStringByString("Kaillera wait for 1 second"));
		//for( i=0; i<25; i++ )
		//{
		// To clear the buffer stored in the Kaillera
		//kailleraModifyPlayValues((void *) kBuffers, sizeof(kPlayerEntry));
		//Sleep(40);
		//}

		//SetStatusBarText(0, TranslateStringByString("Kaillera starts the game"));
		kailleraChatSend("!!!StartGame!!!");	// Tell other players to start
		kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
		//Sleep(100);

		//KailleraState = INPLAY;
	}

	{
		// Other players should wait to receive game setting and cheat codes
		// How can I know how long time to wait
		//SetStatusBarText(0,"Waiting and receiving data from player 0");
		while (KailleraState != INPLAY)
		{
			kailleraModifyPlayValues((void *)kBuffers, sizeof(kPlayerEntry));
			Sleep(10);
		}
	}

	DWORD ThreadID;
	kailleraThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartKailleraThread, this, 0, &ThreadID);
}

void CKaillera::DownloadFiles_SaveStrings(char *line)
{
	char **buf = 0;
	int pos = 0;
	int seq;
	char *startp;
	char *ext = "fal";

	if (strncmp(line, "!EEPROM", 7) == 0)
	{
		buf = &eepromBuf;
		pos = 7;
		ext = "eep";
	}
	else if (strncmp(line, "!SRAM", 5) == 0)
	{
		buf = &sramBuf;
		pos = 5;
		ext = "sra";
	}
	else if (strncmp(line, "!FLASH", 6) == 0)
	{
		buf = &flashramBuf;
		pos = 6;
		ext = "fla";
	}
	else if (strncmp(line, "!MEMPAK", 7) == 0)
	{
		buf = &mempakBuf;
		pos = 7;
		ext = "m0";
	}

	if (!buf)
		return;	// Buffer is not allocated

	if (strncmp(line + pos, "END", 3) == 0)
	{
		char *outbuf = new char[256 * 1024];
		CPath filename;
		int ziplen;
		char *zipbuf = new char[256 * 1024];
		unsigned long len;

		GetFileName(filename, ext);
		ziplen = Decode64(*buf, zipbuf);
		len = 256 * 1024;
		uncompress((Bytef*)outbuf, &len, (const Bytef*)zipbuf, ziplen);

		{
			FILE *fp = fopen(filename, "wb");
			if (fp)
			{
				fwrite(outbuf, len, 1, fp);
				fclose(fp);
			}
		}

		free(*buf);
		*buf = NULL;
		delete outbuf;
		delete zipbuf;
	}
	else
	{
		sscanf(line + pos, "%d", &seq);

		startp = strstr(line + pos, "=");
		if (!startp)
			return;

		startp++;
		if (*startp == 0)
			return;

		if (strlen(startp) == 100)
			memcpy(*buf + seq * 100, startp, strlen(startp));
		else
			strcpy(*buf + seq * 100, startp);
	}
}

void CKaillera::clearCodes()
{
	while(codes.size() > 0)
	{
		char * temp = codes.front();
		delete(temp);
		codes.erase(codes.begin());
	}
}

LPCSTR CKaillera::getCode(int i)
{
	return codes.at(i);
}

void CKaillera::sendDmaToSram(uint8_t * Source, int32_t StartOffset, int32_t len)
{
	CKailleraPacket ckp[4];

	memset(ckp, 0, sizeof(ckp));
}


int CKaillera::numCodes()
{
	return codes.size();
}

DWORD CKaillera::getValues(int player)
{
	return values[player];
}

static LARGE_INTEGER Freq;

/************************************************************************/
/* Kaillera thread                                                      */
/************************************************************************/
void(__stdcall StartKailleraThread)(CKaillera *pKaillera)
{
	DWORD signal;
	static bool setFreq = false;
	if (!setFreq)
	{
		QueryPerformanceFrequency(&Freq);
		setFreq = true;
	}

	LARGE_INTEGER CurrentCPUTime;
	LARGE_INTEGER LastCPUTime;
	LARGE_INTEGER Elapsed;
	double elapsedtime;
	int processid;

#ifdef KAILLERA_LOG_KEY_DATA
	char filename[300];
	sprintf(filename, "\\kaillera_log_%d.log", kailleraLocalPlayerNumber);
	kailleralogfp = fopen(filename, "wt");
#endif

	QueryPerformanceCounter(&LastCPUTime);

	memset(pKaillera->kailleraKeyQueues, 0, sizeof(pKaillera->kailleraKeyQueues));
	memset(pKaillera->kailleraKeyValid, 0, sizeof(pKaillera->kailleraKeyValid));
	memset(pKaillera->kBuffers, 0, sizeof(pKaillera->kBuffers));
	memset(pKaillera->savedKeyValues, 0, sizeof(pKaillera->savedKeyValues));
	memset(pKaillera->kailleraLocalSendKeys, 0, sizeof(pKaillera->kailleraLocalSendKeys));
	memset(pKaillera->kailleraLocalSendKeysValid, 0, sizeof(pKaillera->kailleraLocalSendKeysValid));
	memset(LastEntries, 0xFF, sizeof(LastEntries));

	//kailleraKeyQueueIndex[0] = kailleraKeyQueueIndex[1] = kailleraKeyQueueIndex[2] = kailleraKeyQueueIndex[3] = -1;
	pKaillera->kLastKeyUpdateVI = 0;

	//Starting kaillera thread

	pKaillera->kailleraLastUpdateKeysAtVI = 0;
	//kailleraAutoApplyCheat = FALSE;

	pKaillera->Kaillera_Thread_Keep_Running = TRUE;
	pKaillera->Kaillera_Thread_Is_Running = TRUE;

	ResetEvent(pKaillera->kailleraThreadStopEvent);
	signal = WaitForSingleObject(pKaillera->kailleraThreadStopEvent, 0);

	memset(&pKaillera->Keys, 0, sizeof(pKaillera->Keys));

	while (pKaillera->Kaillera_Thread_Keep_Running)
	{
		signal = WaitForSingleObject(pKaillera->kailleraThreadStopEvent, 0);
		if (signal == WAIT_OBJECT_0)
		{
			//TRACE0("Get kaillera stop event");
			break;
		}

		QueryPerformanceCounter(&CurrentCPUTime);
		Elapsed.QuadPart = CurrentCPUTime.QuadPart - LastCPUTime.QuadPart;
		elapsedtime = (double)Elapsed.QuadPart / (double)Freq.QuadPart;

		//if( elapsedtime > 0.10 || viTotalCount>=kLastKeyUpdateVI+kKeyUpdateStep)
		if (g_System)
		{
			auto vicount = g_System->GetVITotalCount();

			if (vicount >= (pKaillera->kLastKeyUpdateVI + pKaillera->kKeyUpdateStep))
			{
				// Update key values at least every 60 ms
				// I am constantly updating player key values every other 3 ms, no matter key values are read or not
				// key values are updated together with current VI count
				pKaillera->UpdatePlayerKeyValues();
				QueryPerformanceCounter(&LastCPUTime);
			}
			else
			{
				//DO_PROFILIER(processid);
				Sleep(4);
				//processid = get_current_profiling_process();
				//DO_PROFILIER(KAILLERA_PROF);
			}
		}
	}
	//TRACE0("kaillera thread is stopped");

	pKaillera->Kaillera_Thread_Is_Running = FALSE;

	SetEvent(pKaillera->kailleraThreadEvent);	//Tell that this thread is terminated

#ifdef KAILLERA_LOG_KEY_DATA
	fclose(kailleralogfp);
#endif
	ExitThread(0);
}

/************************************************************************/
/* Call this function to stop the Kaillera thread                       */
/************************************************************************/
void CKaillera::StopKailleraThread()
{
	if (kailleraThreadHandle != NULL)
	{
		DWORD signal;
		SetEvent(kailleraThreadStopEvent);	//Tell that this thread is terminated
		Kaillera_Thread_Keep_Running = FALSE;
		signal = WaitForSingleObject(kailleraThreadEvent, 300);
		if (signal == WAIT_OBJECT_0)
		{
			//TRACE0("kaillera thread is successfully stopped");
		}
		else
		{
			ResetEvent(kailleraThreadStopEvent);
			TerminateThread(kailleraThreadHandle, 0);
			Kaillera_Thread_Is_Running = FALSE;
			Kaillera_Thread_Keep_Running = FALSE;
			//TRACE0("kaillera thread is terminated");
		}

		kailleraThreadHandle = NULL;
	}
}

void CKaillera::endGame()
{
	StopKailleraThread();
	kailleraEndGame();
}

int CKaillera::LoadKailleraFuncs()
{
	KailleraHandle = LoadLibrary("kailleraclient.dll");

	if (KailleraHandle) {
		//ShowInfo("Kaillera Library found");
		kailleraGetVersion = (void(__stdcall*) (char*)) GetProcAddress(KailleraHandle, "_kailleraGetVersion@4");
		if (kailleraGetVersion == NULL) {
			ShowError("kailleraGetVersion not found");
			return 0;
		}

		kailleraInit = (void(__stdcall *)(void)) GetProcAddress(KailleraHandle, "_kailleraInit@0");
		if (kailleraInit == NULL) {
			ShowError("kailleraInit not found");
			return 0;
		}

		kailleraShutdown = (void(__stdcall *) (void)) GetProcAddress(KailleraHandle, "_kailleraShutdown@0");
		if (kailleraShutdown == NULL) {
			ShowError("kailleraShutdown not found");
			return 0;
		}

		kailleraSetInfos = (void(__stdcall *) (kailleraInfos *)) GetProcAddress(KailleraHandle, "_kailleraSetInfos@4");
		if (kailleraSetInfos == NULL) {
			ShowError("kailleraSetInfos not found");
			return 0;
		}

		kailleraSelectServerDialog = (void(__stdcall*) (HWND parent)) GetProcAddress(KailleraHandle, "_kailleraSelectServerDialog@4");
		if (kailleraSelectServerDialog == NULL) {
			ShowError("kailleraSelectServerDialog not found");
			return 0;
		}

		kailleraModifyPlayValues = (int(__stdcall *) (void *values, int size)) GetProcAddress(KailleraHandle, "_kailleraModifyPlayValues@8");
		if (kailleraModifyPlayValues == NULL) {
			ShowError("kailleraModifyPlayValues not found");
			return 0;
		}

		kailleraChatSend = (void(__stdcall *) (char *)) GetProcAddress(KailleraHandle, "_kailleraChatSend@4");
		if (kailleraChatSend == NULL) {
			ShowError("kailleraChatSend not found");
			return 0;
		}

		kailleraEndGame = (void(__stdcall *) (void)) GetProcAddress(KailleraHandle, "_kailleraEndGame@0");
		if (kailleraEndGame == NULL) {
			ShowError("kailleraEndGame not found");
			return 0;
		}
	}
	else
	{
		MessageBox(NULL, "Kaillearclient.dll not found. Please place it in the main folder and run Project64k again!", "OOPS", NULL);
		PostQuitMessage(0);
	}

	return 1;
}