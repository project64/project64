// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

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
    bool del(uint64_t checksum); // Checksum hi:palette low:texture
    bool is_cached(uint64_t checksum); // Checksum hi:palette low:texture
    void clear();
public:
    ~TxCache();
    TxCache(int options, int cachesize, const char *path, const char *ident,
        dispInfoFuncExt callback);
    bool add(uint64_t checksum, // Checksum hi:palette low:texture
        GHQTexInfo *info, int dataSize = 0);
    bool get(uint64_t checksum, // Checksum hi:palette low:texture
        GHQTexInfo *info);
};

#endif /* __TXCACHE_H__ */
