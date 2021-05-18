// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

// Dump cache to disk (0:disable, 1:enable)
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
        // Dump cache to disk
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
    // Assert local options
    if (_path.empty() || _ident.empty() || !_cacheSize)
    {
        _options &= ~DUMP_TEXCACHE;
    }

#if DUMP_CACHE
    if (_options & DUMP_TEXCACHE)
    {
        // Find it on disk
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