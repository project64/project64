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

#ifndef __TXTEXCACHE_H__
#define __TXTEXCACHE_H__

#include "TxCache.h"

class TxTexCache : public TxCache
{
public:
    ~TxTexCache();
    TxTexCache(int options, int cachesize, const char *path, const char *ident,
        dispInfoFuncExt callback);
    bool add(uint64_t checksum, /* checksum hi:palette low:texture */
        GHQTexInfo *info);
};

#endif /* __TXTEXCACHE_H__ */
