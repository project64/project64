#include "stdafx.h"
#include "mempak.h"
#include "Logging.h"

//settings
enum SAVE_CHIP_TYPE g_SaveUsing;

void CC_Core::SetSettings  ( )
{
	if (_Settings)
	{
		g_SaveUsing           = (SAVE_CHIP_TYPE)_Settings->LoadDword(Game_SaveChip);
	}
}

void CloseSaveChips ( void )
{
	CloseMempak();
}
