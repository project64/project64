/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2007 Hiroshi Morii                                         *
* Copyright (C) 2003 Rice1964                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

#ifndef __TXDBG_H__
#define __TXDBG_H__

#include <stdio.h>
#include "TxInternal.h"

class TxDbg
{
private:
    FILE* _dbgfile;
    int _level;
    TxDbg();
public:
    static TxDbg* getInstance() {
        static TxDbg txDbg;
        return &txDbg;
    }
    ~TxDbg();
    void output(const int level, const char *format, ...);
};

#ifdef DEBUG
#define DBG_INFO(...) TxDbg::getInstance()->output(__VA_ARGS__)
#define INFO(...) DBG_INFO(__VA_ARGS__)
#else
#define DBG_INFO(...)
#ifdef GHQCHK
#define INFO(...) TxDbg::getInstance()->output(__VA_ARGS__)
#else
#if 0 /* XXX enable this to log basic hires texture checks */
#define INFO(...) TxDbg::getInstance()->output(__VA_ARGS__)
#else
#define INFO(...) DBG_INFO(__VA_ARGS__)
#endif
#endif
#endif

#endif /* __TXDBG_H__ */
