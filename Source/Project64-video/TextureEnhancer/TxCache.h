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

#ifndef __TXCACHE_H__
#define __TXCACHE_H__

#include "TxInternal.h"
#include "TxUtil.h"
#include <list>
#include <map>
#include <string>

class TxCache
{
private:
    std::list<uint64_t> _cachelist;
    uint8 *_gzdest0;
    uint8 *_gzdest1;
    uint32 _gzdestLen;
protected:
    int _options;
    std::string _ident;
    std::string _path;
    dispInfoFuncExt _callback;
    TxUtil *_txUtil;
    struct TXCACHE {
        int size;
        GHQTexInfo info;
        std::list<uint64_t>::iterator it;
    };
    int _totalSize;
    int _cacheSize;
    std::map<uint64_t, TXCACHE*> _cache;
    bool save(const char *path, const char *filename, const int config);
    bool load(const char *path, const char *filename, const int config);
    bool del(uint64_t checksum); /* checksum hi:palette low:texture */
    bool is_cached(uint64_t checksum); /* checksum hi:palette low:texture */
    void clear();
public:
    ~TxCache();
    TxCache(int options, int cachesize, const char *path, const char *ident,
        dispInfoFuncExt callback);
    bool add(uint64_t checksum, /* checksum hi:palette low:texture */
        GHQTexInfo *info, int dataSize = 0);
    bool get(uint64_t checksum, /* checksum hi:palette low:texture */
        GHQTexInfo *info);
};

#endif /* __TXCACHE_H__ */
