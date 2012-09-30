#pragma once

#ifdef __cplusplus
class CC_Core
{
public:
	static void SetSettings  ( );
};

#endif
#ifdef __cplusplus
extern "C" {
#endif

#include "..\\Types.h"

void CloseSaveChips     ( void );

//settings
extern enum SAVE_CHIP_TYPE g_SaveUsing;

#ifdef __cplusplus
}
#endif