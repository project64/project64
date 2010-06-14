#include "stdafx.h"
#include "Eeprom.h"

static HANDLE hEepromFile = NULL;
BYTE EEPROM[0x800];

void CloseEeprom (void) {
	if (hEepromFile) {
		CloseHandle(hEepromFile);
		hEepromFile = NULL;
	}
}
