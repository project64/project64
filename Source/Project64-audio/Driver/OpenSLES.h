// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64.
// Copyright(C) 2015 Gilles Siberlin
// Copyright(C) 2007 - 2009 Richard Goedeken
// Copyright(C) 2007 - 2008 Ebenblues
// Copyright(C) 2003 JttL
// Copyright(C) 2002 Hacktarux
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <Common/SyncEvent.h>
#include <Project64-plugin-spec/Audio.h>
#include "SoundBase.h"

class OpenSLESDriver :
    public SoundDriverBase
{
public:
    void AI_Startup(void);
    void AI_Shutdown(void);
    void AI_SetFrequency(uint32_t freq, uint32_t BufferSize);
    void AI_LenChanged(uint8_t *start, uint32_t length);
    void AI_Update(bool Wait);

private:
    SyncEvent m_AiUpdateEvent;
};
