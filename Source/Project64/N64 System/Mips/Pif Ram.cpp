#include "stdafx.h"
#include "../C Core/mempak.h"

int   CPifRamSettings::m_RefCount = 0;
bool  CPifRamSettings::m_bShowPifRamErrors = false;
bool  CPifRamSettings::m_DelaySI = false;
DWORD CPifRamSettings::m_RdramSize = 0;

CPifRamSettings::CPifRamSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		_Settings->RegisterChangeCB(Debugger_ShowPifErrors,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_RDRamSize,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_DelaySI,NULL,RefreshSettings);
		RefreshSettings(NULL);
	}
}

CPifRamSettings::~CPifRamSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(Debugger_ShowPifErrors,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_RDRamSize,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_DelaySI,NULL,RefreshSettings);
	}
}

void CPifRamSettings::RefreshSettings(void *)
{
	m_bShowPifRamErrors   = _Settings->LoadBool(Debugger_ShowPifErrors);
	m_RdramSize           = _Settings->LoadDword(Game_RDRamSize);
	m_DelaySI             = _Settings->LoadBool(Game_DelaySI);
}

CPifRam::CPifRam( bool SavesReadOnly ) :
	CEeprom(SavesReadOnly)
{
	Reset();
}

CPifRam::~CPifRam( void )
{
}

void CPifRam::Reset ( void )
{
	memset(m_PifRam,0,sizeof(m_PifRam));
	memset(m_PifRom,0,sizeof(m_PifRom));
}

void CPifRam::n64_cic_nus_6105(char challenge[], char respone[], int length)
{
	static char lut0[0x10] = {
		0x4, 0x7, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1, 
		0xC, 0xF, 0x8, 0xF, 0x6, 0x3, 0x6, 0x9
	};
	static char lut1[0x10] = {
		0x4, 0x1, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1, 
		0xC, 0x9, 0x8, 0x5, 0x6, 0x3, 0xC, 0x9
	};
	char key, *lut;
	int i, sgn, mag, mod;

	for (key = 0xB, lut = lut0, i = 0; i < length; i++) {
		respone[i] = (key + 5 * challenge[i]) & 0xF;
		key = lut[respone[i]];
		sgn = (respone[i] >> 3) & 0x1;
		mag = ((sgn == 1) ? ~respone[i] : respone[i]) & 0x7;
		mod = (mag % 3 == 1) ? sgn : 1 - sgn;
		if (lut == lut1 && (respone[i] == 0x1 || respone[i] == 0x9))
			mod = 1;
		if (lut == lut1 && (respone[i] == 0xB || respone[i] == 0xE))
			mod = 0;
		lut = (mod == 1) ? lut1 : lut0;
	}
}


void CPifRam::PifRamRead (void) 
{
	if (m_PifRam[0x3F] == 0x2) 
	{
		return;
	}

	CONTROL * Controllers = _Plugins->Control()->PluginControllers();

	int Channel = 0;
	for (int CurPos = 0; CurPos < 0x40; CurPos ++)
	{
		switch(m_PifRam[CurPos]) {
		case 0x00: 
			Channel += 1; 
			if (Channel > 6) 
			{ 
				CurPos = 0x40; 
			}
			break;
		case 0xFE: CurPos = 0x40; break;
		case 0xFF: break;
		case 0xB4: case 0x56: case 0xB8: break; /* ??? */
		default:
			if ((m_PifRam[CurPos] & 0xC0) == 0) 
			{
				if (Channel < 4) {
					if (Controllers[Channel].Present && Controllers[Channel].RawData) {
						if (_Plugins->Control()->ReadController) { _Plugins->Control()->ReadController(Channel,&m_PifRam[CurPos]); }
					} else {						
						ReadControllerCommand(Channel,&m_PifRam[CurPos]);
					}
				} 
				CurPos += m_PifRam[CurPos] + (m_PifRam[CurPos + 1] & 0x3F) + 1;
				Channel += 1;
			} else {
				if (bShowPifRamErrors()) { DisplayError("Unknown Command in PifRamRead(%X)",m_PifRam[CurPos]); }
				CurPos = 0x40;
			}
			break;
		}
	} 
	if (_Plugins->Control()->ReadController) { _Plugins->Control()->ReadController(-1,NULL); }
}

void CPifRam::PifRamWrite (void) {
	CONTROL * Controllers = _Plugins->Control()->PluginControllers();
	int Channel, CurPos;
	char Challenge[30], Response[30];

	Channel = 0;

	if( m_PifRam[0x3F] > 0x1) { 
		switch (m_PifRam[0x3F]) {
		case 0x02:
			// format the 'challenge' message into 30 nibbles for X-Scale's CIC code
			for (int i = 0; i < 15; i++)
			{
				Challenge[i*2] =   (m_PifRam[48+i] >> 4) & 0x0f;
				Challenge[i*2+1] =  m_PifRam[48+i]       & 0x0f;
			}
			//Calcuate the proper respone for the give challange(X-Scales algorithm)
			n64_cic_nus_6105(Challenge, Response, CHALLENGE_LENGTH - 2);
			// re-format the 'response' into a byte stream
			for (int i = 0; i < 15; i++)
			{
				m_PifRam[48+i] = (Response[i*2] << 4) + Response[i*2+1];
			}
			// the last byte (2 nibbles) is always 0
			m_PifRam[63] = 0;
			break;
		case 0x08: 
			m_PifRam[0x3F] = 0; 
			_Reg->MI_INTR_REG |= MI_INTR_SI;
			_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
			_Reg->CheckInterrupts();
			break;
		case 0x10:
			memset(m_PifRom,0,0x7C0);
			break;
		case 0x30:
			m_PifRam[0x3F] = 0x80;		
			break;
		case 0xC0:
			memset(m_PifRam,0,0x40);
			break;
		default:
			if (bShowPifRamErrors()) { DisplayError("Unkown PifRam control: %d",m_PifRam[0x3F]); }
		}
		return;
	}

	for (CurPos = 0; CurPos < 0x40; CurPos++){
		switch(m_PifRam[CurPos]) {
		case 0x00: 
			Channel += 1; 
			if (Channel > 6) { CurPos = 0x40; }
			break;
		case 0xFE: CurPos = 0x40; break;
		case 0xFF: break;
		case 0xB4: case 0x56: case 0xB8: break; /* ??? */
		default:
			if ((m_PifRam[CurPos] & 0xC0) == 0) {
				if (Channel < 4) {
					if (Controllers[Channel].Present && Controllers[Channel].RawData) {
						if (_Plugins->Control()->ControllerCommand) { _Plugins->Control()->ControllerCommand(Channel,&m_PifRam[CurPos]); }
					} else {
						ProcessControllerCommand(Channel,&m_PifRam[CurPos]);
					}
				} else if (Channel == 4) {
					EepromCommand(&m_PifRam[CurPos]);
				} else {
					if (bShowPifRamErrors()) 
					{
						DisplayError("Command on channel 5?");
					}
				}
				CurPos += m_PifRam[CurPos] + (m_PifRam[CurPos + 1] & 0x3F) + 1;
				Channel += 1;
			} else {
				if (bShowPifRamErrors()) { DisplayError("Unknown Command in PifRamWrite(%X)",m_PifRam[CurPos]); }
				CurPos = 0x40;
			}
			break;
		}
	}
	m_PifRam[0x3F] = 0;
	if (_Plugins->Control()->ControllerCommand) { _Plugins->Control()->ControllerCommand(-1,NULL); }
}

void CPifRam::SI_DMA_READ (void) 
{
	BYTE * PifRamPos = m_PifRam;
	BYTE * RDRAM = _MMU->Rdram();
	
	DWORD & SI_DRAM_ADDR_REG = _Reg->SI_DRAM_ADDR_REG;
	if ((int)SI_DRAM_ADDR_REG > (int)RdramSize()) 
	{
		if (bShowPifRamErrors()) 
		{
			DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
		}
		return;
	}
	
	PifRamRead();
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	if ((int)SI_DRAM_ADDR_REG < 0) {
		int count, RdramPos;

		RdramPos = (int)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { continue; }
			RDRAM[RdramPos ^3] = m_PifRam[count];
		}
	} else {
		_asm {
			mov edi, dword ptr [SI_DRAM_ADDR_REG]
			mov edi, dword ptr [edi]
			add edi, RDRAM
			mov ecx, PifRamPos
			mov edx, 0		
	memcpyloop:
			mov eax, dword ptr [ecx + edx]
			bswap eax
			mov  dword ptr [edi + edx],eax
			mov eax, dword ptr [ecx + edx + 4]
			bswap eax
			mov  dword ptr [edi + edx + 4],eax
			mov eax, dword ptr [ecx + edx + 8]
			bswap eax
			mov  dword ptr [edi + edx + 8],eax
			mov eax, dword ptr [ecx + edx + 12]
			bswap eax
			mov  dword ptr [edi + edx + 12],eax
			add edx, 16
			cmp edx, 64
			jb memcpyloop
		}
	}
	
#ifndef EXTERNAL_RELEASE
	if (LogOptions.LogPRDMAMemStores) {
		int count;
		char HexData[100], AsciiData[100], Addon[20];
		LogMessage("\tData DMAed to RDRAM:");			
		LogMessage("\t--------------------");
		for (count = 0; count < 16; count ++ ) {
			if ((count % 4) == 0) { 
				sprintf(HexData,"\0"); 
				sprintf(AsciiData,"\0"); 
			}
 			sprintf(Addon,"%02X %02X %02X %02X", 
				m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1], 
				m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3] );
			strcat(HexData,Addon);
			if (((count + 1) % 4) != 0) {
				sprintf(Addon,"-");
				strcat(HexData,Addon);
			} 
			
			sprintf(Addon,"%c%c%c%c", 
				m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1], 
				m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3] );
			strcat(AsciiData,Addon);
			
			if (((count + 1) % 4) == 0) {
				LogMessage("\t%s %s",HexData, AsciiData);
			} 
		}
		LogMessage("");
	}
#endif

	if (bDelaySI()) {
		_SystemTimer->SetTimer(CSystemTimer::SiTimer,0x900,false);
	} else {
		_Reg->MI_INTR_REG |= MI_INTR_SI;
		_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		_Reg->CheckInterrupts();
	}
}

void CPifRam::SI_DMA_WRITE (void) 
{
	BYTE * PifRamPos = m_PifRam;
	
	DWORD & SI_DRAM_ADDR_REG = _Reg->SI_DRAM_ADDR_REG;
	if ((int)SI_DRAM_ADDR_REG > (int)RdramSize()) 
	{
		if (bShowPifRamErrors()) 
		{
			DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
		}
		return;
	}
	
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	BYTE * RDRAM = _MMU->Rdram();

	if ((int)SI_DRAM_ADDR_REG < 0) {
		int count, RdramPos;

		RdramPos = (int)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { m_PifRam[count] = 0; continue; }
			m_PifRam[count] = RDRAM[RdramPos ^3];
		}
	} else {
		_asm {
			mov ecx, dword ptr [SI_DRAM_ADDR_REG]
			mov ecx, dword ptr [ecx]
			add ecx, RDRAM
			mov edi, PifRamPos
			mov edx, 0		
	memcpyloop:
			mov eax, dword ptr [ecx + edx]
			bswap eax
			mov  dword ptr [edi + edx],eax
			mov eax, dword ptr [ecx + edx + 4]
			bswap eax
			mov  dword ptr [edi + edx + 4],eax
			mov eax, dword ptr [ecx + edx + 8]
			bswap eax
			mov  dword ptr [edi + edx + 8],eax
			mov eax, dword ptr [ecx + edx + 12]
			bswap eax
			mov  dword ptr [edi + edx + 12],eax
			add edx, 16
			cmp edx, 64
			jb memcpyloop
		}
	}
	
#ifndef EXTERNAL_RELEASE
	if (LogOptions.LogPRDMAMemLoads) {
		int count;
		char HexData[100], AsciiData[100], Addon[20];
		LogMessage("");
		LogMessage("\tData DMAed to the Pif Ram:");			
		LogMessage("\t--------------------------");
		for (count = 0; count < 16; count ++ ) {
			if ((count % 4) == 0) { 
				sprintf(HexData,"\0"); 
				sprintf(AsciiData,"\0"); 
			}
			sprintf(Addon,"%02X %02X %02X %02X", 
				m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1], 
				m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3] );
			strcat(HexData,Addon);
			if (((count + 1) % 4) != 0) {
				sprintf(Addon,"-");
				strcat(HexData,Addon);
			} 
			
			sprintf(Addon,"%c%c%c%c", 
				m_PifRam[(count << 2) + 0], m_PifRam[(count << 2) + 1], 
				m_PifRam[(count << 2) + 2], m_PifRam[(count << 2) + 3] );
			strcat(AsciiData,Addon);
			
			if (((count + 1) % 4) == 0) {
				LogMessage("\t%s %s",HexData, AsciiData);
			} 
		}
		LogMessage("");
	}
#endif

	PifRamWrite();
	
	if (bDelaySI()) {
		_SystemTimer->SetTimer(CSystemTimer::SiTimer,0x900,false);
	} else {
		_Reg->MI_INTR_REG |= MI_INTR_SI;
		_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		_Reg->CheckInterrupts();
	}
}

void CPifRam::ProcessControllerCommand ( int Control, BYTE * Command) 
{
	CONTROL * Controllers = _Plugins->Control()->PluginControllers();

	switch (Command[2]) {
	case 0x00: // check
	case 0xFF: // reset & check ?
		if ((Command[1] & 0x80) != 0) { break; }
		if (bShowPifRamErrors()) 
		{
			if (Command[0] != 1) { DisplayError("What am I meant to do with this Controller Command"); }
			if (Command[1] != 3) { DisplayError("What am I meant to do with this Controller Command"); }
		}
		if (Controllers[Control].Present == TRUE) {
			Command[3] = 0x05;
			Command[4] = 0x00;
			switch ( Controllers[Control].Plugin) {
			case PLUGIN_RUMBLE_PAK: Command[5] = 1; break;
			case PLUGIN_MEMPAK: Command[5] = 1; break;
			case PLUGIN_RAW: Command[5] = 1; break;
			default: Command[5] = 0; break;
			}
		} else {
			Command[1] |= 0x80;
		}
		break;
	case 0x01: // read controller
		if (bShowPifRamErrors()) 
		{
			if (Command[0] != 1) { DisplayError("What am I meant to do with this Controller Command"); }
			if (Command[1] != 4) { DisplayError("What am I meant to do with this Controller Command"); }
		}
		if (Controllers[Control].Present == FALSE) {
			Command[1] |= 0x80;
		}
		break;
	case 0x02: //read from controller pack
#ifndef EXTERNAL_RELEASE
		if (LogOptions.LogControllerPak) { LogControllerPakData("Read: Before Gettting Results"); }
#endif
		if (bShowPifRamErrors()) 
		{
			if (Command[0] != 3) { DisplayError("What am I meant to do with this Controller Command"); }
			if (Command[1] != 33) { DisplayError("What am I meant to do with this Controller Command"); }
		}
		if (Controllers[Control].Present == TRUE) {
			DWORD address = ((Command[3] << 8) | Command[4]);
			switch (Controllers[Control].Plugin) {
			case PLUGIN_RUMBLE_PAK:
				memset(&Command[5], (address >= 0x8000 && address < 0x9000) ? 0x80 : 0x00, 0x20);
				Command[0x25] = Mempacks_CalulateCrc(&Command[5]);
				break;
			case PLUGIN_MEMPAK: ReadFromMempak(Control, address, &Command[5]); break;
			case PLUGIN_RAW: if (_Plugins->Control()->ControllerCommand) { _Plugins->Control()->ControllerCommand(Control, Command); } break;
			default:
				memset(&Command[5], 0, 0x20);
				Command[0x25] = 0;
			}
		} else {
			Command[1] |= 0x80;
		}
#ifndef EXTERNAL_RELEASE
		if (LogOptions.LogControllerPak) { LogControllerPakData("Read: After Gettting Results"); }
#endif
		break;
	case 0x03: //write controller pak
#ifndef EXTERNAL_RELEASE
		if (LogOptions.LogControllerPak) { LogControllerPakData("Write: Before Processing"); }
#endif
		if (bShowPifRamErrors()) 
		{
			if (Command[0] != 35) { DisplayError("What am I meant to do with this Controller Command"); }
			if (Command[1] != 1) { DisplayError("What am I meant to do with this Controller Command"); }
		}		
		if (Controllers[Control].Present == TRUE) {
			DWORD address = ((Command[3] << 8) | Command[4]);
			switch (Controllers[Control].Plugin) {
			case PLUGIN_MEMPAK: WriteToMempak(Control, address, &Command[5]); break;
			case PLUGIN_RAW: if (_Plugins->Control()->ControllerCommand) { _Plugins->Control()->ControllerCommand(Control, Command); } break;
			case PLUGIN_RUMBLE_PAK: 
				if ((address & 0xFFE0) == 0xC000 && _Plugins->Control()->RumbleCommand != NULL) {
					_Plugins->Control()->RumbleCommand(Control, *(BOOL *)(&Command[5]));
				}
			default:
				Command[0x25] = Mempacks_CalulateCrc(&Command[5]);
			}
		} else {
			Command[1] |= 0x80;
		}
#ifndef EXTERNAL_RELEASE
		if (LogOptions.LogControllerPak) { LogControllerPakData("Write: After Processing"); }
#endif
		break;
	default:
		if (bShowPifRamErrors()) { DisplayError("Unknown ControllerCommand %d",Command[2]); }
	}
}

void CPifRam::ReadControllerCommand (int Control, BYTE * Command) {
	CONTROL * Controllers = _Plugins->Control()->PluginControllers();

	switch (Command[2]) {
	case 0x01: // read controller
		if (Controllers[Control].Present == TRUE) 
		{
			if (bShowPifRamErrors()) 
			{
				if (Command[0] != 1) { DisplayError("What am I meant to do with this Controller Command"); }
				if (Command[1] != 4) { DisplayError("What am I meant to do with this Controller Command"); }
			}
			*(DWORD *)&Command[3] = _BaseSystem->GetButtons(Control);
		}
		break;
	case 0x02: //read from controller pack
		if (Controllers[Control].Present == TRUE) {
			switch (Controllers[Control].Plugin) {
			case PLUGIN_RAW: if (_Plugins->Control()->ReadController) { _Plugins->Control()->ReadController(Control, Command); } break;
			}
		} 
		break;
	case 0x03: //write controller pak
		if (Controllers[Control].Present == TRUE) {
			switch (Controllers[Control].Plugin) {
			case PLUGIN_RAW: if (_Plugins->Control()->ReadController) { _Plugins->Control()->ReadController(Control, Command); } break;
			}
		}
		break;
	}
}

void CPifRam::LogControllerPakData (char * Description) 
{
	BYTE * PIF_Ram = _MMU->PifRam();

#if (!defined(EXTERNAL_RELEASE))
	int count, count2;
	char HexData[100], AsciiData[100], Addon[20];
	LogMessage("\t%s:",Description);			
	LogMessage("\t------------------------------");
	for (count = 0; count < 16; count ++ ) {
		if ((count % 4) == 0) { 
			sprintf(HexData,"\0"); 
			sprintf(AsciiData,"\0"); 
		}
 		sprintf(Addon,"%02X %02X %02X %02X", 
			PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1], 
			PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3] );
		strcat(HexData,Addon);
		if (((count + 1) % 4) != 0) {
			sprintf(Addon,"-");
			strcat(HexData,Addon);
		} 
		
		Addon[0] = 0;
		for (count2 = 0; count2 < 4; count2++) {
			if (PIF_Ram[(count << 2) + count2] < 30) {
				strcat(Addon,".");
			} else {
				sprintf(Addon,"%s%c",Addon,PIF_Ram[(count << 2) + count2]);
			}
		}
		strcat(AsciiData,Addon);
		
		if (((count + 1) % 4) == 0) {
			LogMessage("\t%s %s",HexData, AsciiData);
		} 
	}
	LogMessage("");
#endif
}