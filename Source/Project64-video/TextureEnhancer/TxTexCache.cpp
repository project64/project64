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
#ifdef WIN32
#pragma warning(disable: 4786)
#endif

/* dump cache to disk (0:disable, 1:enable) */
#define DUMP_CACHE 1

#include "TxTexCache.h"
#include "TxDbg.h"
#include <zlib/zlib.h>
#include <string>
#include <Common/path.h>
#include <Common/StdString.h>

TxTexCache::~TxTexCache()
{
#if DUMP_CACHE
    if (_options & DUMP_TEXCACHE)
    {
        /* dump cache to disk */
        std::string filename = _ident + "_MEMORYCACHE.dat";
        CPath cachepath(_path.c_str(), "");
        cachepath.AppendDirectory("Cache");

        int config = _options & (FILTER_MASK | ENHANCEMENT_MASK | COMPRESS_TEX | COMPRESSION_MASK | FORCE16BPP_TEX | GZ_TEXCACHE);

        TxCache::save(cachepath, filename.c_str(), config);
    }
#endif
}

TxTexCache::TxTexCache(int options, int cachesize, const char *path, const char *ident, dispInfoFuncExt callback) :
    TxCache((options & ~GZ_HIRESTEXCACHE), cachesize, path, ident, callback)
{
    /* assert local options */
    if (_path.empty() || _ident.empty() || !_cacheSize)
    {
        _options &= ~DUMP_TEXCACHE;
    }

#if DUMP_CACHE
    if (_options & DUMP_TEXCACHE)
    {
        /* find it on disk */
        std::string filename = _ident + "_MEMORYCACHE.dat";
        CPath cachepath(_path.c_str(), "");
        cachepath.AppendDirectory("Cache");
        int config = _options & (FILTER_MASK | ENHANCEMENT_MASK | COMPRESS_TEX | COMPRESSION_MASK | FORCE16BPP_TEX | GZ_TEXCACHE);

        TxCache::load(cachepath, filename.c_str(), config);
    }
#endif
}

bool TxTexCache::add(uint64_t checksum, GHQTexInfo *info)
{
    if (_cacheSize <= 0) return 0;

    return TxCache::add(checksum, info);
}