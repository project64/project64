// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifndef __TXTEXCACHE_H__
#define __TXTEXCACHE_H__

#include "TxCache.h"

class TxTexCache : public TxCache
{
public:
    ~TxTexCache();
    TxTexCache(int options, int cachesize, const char *path, const char *ident,
        dispInfoFuncExt callback);
    bool add(uint64_t checksum, // checksum hi:palette low:texture
        GHQTexInfo *info);
};

#endif /* __TXTEXCACHE_H__ */
