/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define DBG_LEVEL 80

#include "TxDbg.h"
#include <string.h>
#include <stdarg.h>
#include <Common/StdString.h>
#include <Common/path.h>
#include <Glide64/Config.h>
#include <Settings/Settings.h>

TxDbg::TxDbg()
{

    char log_dir[260];
    memset(log_dir, 0, sizeof(log_dir));
    if (Set_log_dir != 0)
    {
        GetSystemSettingSz(Set_log_dir, log_dir, sizeof(log_dir));
    }

    _level = DBG_LEVEL;

    if (!_dbgfile)
#ifdef GHQCHK
        _dbgfile = fopen(CPath(log_dir, "ghqchk.txt"), "w");
#else
        _dbgfile = fopen(CPath(log_dir, "glidehq.dbg"), "w");
#endif
}

TxDbg::~TxDbg()
{
    if (_dbgfile) {
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