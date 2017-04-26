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
#define DBG_LEVEL 80

#include "TxDbg.h"
#include <string.h>
#include <stdarg.h>
#include <Common/StdString.h>
#include <Common/path.h>
#include <Project64-video/Config.h>
#include <Project64-video/Settings.h>

TxDbg::TxDbg()
{
    const char * log_dir = g_settings->log_dir();
    if (log_dir != NULL && log_dir[0] != '\0')
    {
        _level = DBG_LEVEL;

        if (!_dbgfile)
#ifdef GHQCHK
            _dbgfile = fopen(CPath(log_dir, "ghqchk.txt"), "w");
#else
            _dbgfile = fopen(CPath(log_dir, "glidehq.dbg"), "w");
#endif
    }
}

TxDbg::~TxDbg()
{
    if (_dbgfile)
    {
        fclose(_dbgfile);
        _dbgfile = 0;
    }

    _level = DBG_LEVEL;
}

void
TxDbg::output(const int level, const char *format, ...)
{
    if (level > _level)
        return;

    stdstr_f newformat("%d:\t%s", level, format);

    va_list args;
    va_start(args, format);
    vfprintf(_dbgfile, newformat.c_str(), args);
    fflush(_dbgfile);
    va_end(args);
}