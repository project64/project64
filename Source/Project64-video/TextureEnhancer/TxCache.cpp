// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include <string.h> // memcpy, memset
#include <stdlib.h> // malloc, free

#include "TxCache.h"
#include "TxDbg.h"
#include <zlib/zlib.h>
#include <Common/path.h>
#include <Common/StdString.h>
#include <Project64-video/Renderer/types.h>

TxCache::~TxCache()
{
    // Free memory, clean up, etc.
    clear();

    delete _txUtil;
}

TxCache::TxCache(int options, int cachesize, const char *path, const char *ident,
    dispInfoFuncExt callback)
{
    _txUtil = new TxUtil();

    _options = options;
    _cacheSize = cachesize;
    _callback = callback;
    _totalSize = 0;

    // Save path name
    if (path)
    {
        _path.assign(path);
    }

    // Save ROM name
    if (ident)
    {
        _ident.assign(ident);
    }

    // zlib memory buffers to (de)compress high resolution textures
    if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE))
    {
        _gzdest0 = TxMemBuf::getInstance()->get(0);
        _gzdest1 = TxMemBuf::getInstance()->get(1);
        _gzdestLen = (TxMemBuf::getInstance()->size_of(0) < TxMemBuf::getInstance()->size_of(1)) ?
            TxMemBuf::getInstance()->size_of(0) : TxMemBuf::getInstance()->size_of(1);

        if (!_gzdest0 || !_gzdest1 || !_gzdestLen)
        {
            _options &= ~(GZ_TEXCACHE | GZ_HIRESTEXCACHE);
            _gzdest0 = nullptr;
            _gzdest1 = nullptr;
            _gzdestLen = 0;
        }
    }
}

bool
TxCache::add(uint64_t checksum, GHQTexInfo *info, int dataSize)
{
    // NOTE: dataSize must be provided if info->data is zlib compressed

    if (!checksum || !info->data) return 0;

    uint8 *dest = info->data;
    uint16 format = info->format;

    if (!dataSize)
    {
        dataSize = _txUtil->sizeofTx(info->width, info->height, info->format);

        if (!dataSize) return 0;

        if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE))
        {
            // zlib compress it. Compression level:1 (best speed)
            uLongf destLen = _gzdestLen;
            dest = (dest == _gzdest0) ? _gzdest1 : _gzdest0;
            if (compress2(dest, &destLen, info->data, dataSize, 1) != Z_OK)
            {
                dest = info->data;
                DBG_INFO(80, "Error: zlib compression failed!\n");
            }
            else
            {
                DBG_INFO(80, "zlib compressed: %.02fkb->%.02fkb\n", (float)dataSize / 1000, (float)destLen / 1000);
                dataSize = destLen;
                format |= GFX_TEXFMT_GZ;
            }
        }
    }

    // If cache size exceeds limit, remove old cache
    if (_cacheSize > 0)
    {
        _totalSize += dataSize;
        if ((_totalSize > _cacheSize) && !_cachelist.empty())
        {
            // _cachelist is arranged so that frequently used textures are in the back
            std::list<uint64_t>::iterator itList = _cachelist.begin();
            while (itList != _cachelist.end())
            {
                // Find it in _cache
                std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.find(*itList);
                if (itMap != _cache.end())
                {
                    // Yep we have it, let's remove it
                    _totalSize -= (*itMap).second->size;
                    free((*itMap).second->info.data);
                    delete (*itMap).second;
                    _cache.erase(itMap);
                }
                itList++;

                // Check if memory cache has enough space
                if (_totalSize <= _cacheSize)
                    break;
            }
            // Remove from _cachelist
            _cachelist.erase(_cachelist.begin(), itList);

            DBG_INFO(80, "+++++++++\n");
        }
        _totalSize -= dataSize;
    }

    // Cache it
    uint8 *tmpdata = (uint8*)malloc(dataSize);
    if (tmpdata)
    {
        TXCACHE *txCache = new TXCACHE;
        if (txCache)
        {
            // We can directly write as we filter, but for now we get away
            // with doing memcpy after all the filtering is done.

            memcpy(tmpdata, dest, dataSize);

            // Copy it
            memcpy(&txCache->info, info, sizeof(GHQTexInfo));
            txCache->info.data = tmpdata;
            txCache->info.format = format;
            txCache->size = dataSize;

            // Add to cache
            if (_cacheSize > 0)
            {
                _cachelist.push_back(checksum);
                txCache->it = --(_cachelist.end());
            }
            // _cache[checksum] = txCache;
            _cache.emplace(checksum, txCache);

#ifdef DEBUG
            DBG_INFO(80, "[%5d] added! CRC:%08X %08X %d x %d gfmt:%x total:%.02fmb\n",
                _cache.size(), (uint32)(checksum >> 32), (uint32)(checksum & 0xffffffff),
                info->width, info->height, info->format, (float)_totalSize / 1000000);

            DBG_INFO(80, "smalllodlog2:%d largelodlog2:%d aspectratiolog2:%d\n",
                txCache->info.smallLodLog2, txCache->info.largeLodLog2, txCache->info.aspectRatioLog2);

            if (info->tiles)
            {
                DBG_INFO(80, "tiles:%d un-tiled size:%d x %d\n", info->tiles, info->untiled_width, info->untiled_height);
            }

            if (_cacheSize > 0)
            {
                DBG_INFO(80, "Cache max config:%.02fmb\n", (float)_cacheSize / 1000000);

                if (_cache.size() != _cachelist.size())
                {
                    DBG_INFO(80, "Error: Cache/cache list mismatch! (%d/%d)\n", _cache.size(), _cachelist.size());
                }
            }
#endif

            // Total cache size
            _totalSize += dataSize;

            return 1;
        }
        free(tmpdata);
    }

    return 0;
}

bool
TxCache::get(uint64_t checksum, GHQTexInfo *info)
{
    if (!checksum || _cache.empty()) return 0;

    // Find a match in cache
    std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.find(checksum);
    if (itMap != _cache.end())
    {
        // Yep, we've got it
        memcpy(info, &(((*itMap).second)->info), sizeof(GHQTexInfo));

        // Push it to the back of the list
        if (_cacheSize > 0)
        {
            _cachelist.erase(((*itMap).second)->it);
            _cachelist.push_back(checksum);
            ((*itMap).second)->it = --(_cachelist.end());
        }

        // zlib decompress it
        if (info->format & GFX_TEXFMT_GZ)
        {
            uLongf destLen = _gzdestLen;
            uint8 *dest = (_gzdest0 == info->data) ? _gzdest1 : _gzdest0;
            if (uncompress(dest, &destLen, info->data, ((*itMap).second)->size) != Z_OK)
            {
                DBG_INFO(80, "Error: zlib decompression failed!\n");
                return 0;
            }
            info->data = dest;
            info->format &= ~GFX_TEXFMT_GZ;
            DBG_INFO(80, "zlib decompressed: %.02fkb->%.02fkb\n", (float)(((*itMap).second)->size) / 1000, (float)destLen / 1000);
        }
        return 1;
    }
    return 0;
}

bool TxCache::save(const char *path, const char *filename, int config)
{
    if (!_cache.empty())
    {
        CPath(path, "").DirectoryCreate();

        gzFile gzfp = gzopen(CPath(path, filename), "wb1");
        DBG_INFO(80, "gzfp:%x file:%ls\n", gzfp, filename);
        if (gzfp)
        {
            // Write header to determine config match
            gzwrite(gzfp, &config, 4);

            std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.begin();
            while (itMap != _cache.end())
            {
                uint8 *dest = (*itMap).second->info.data;
                uint32 destLen = (*itMap).second->size;
                uint16 format = (*itMap).second->info.format;

                /*
				To keep things simple, we save the texture data in a zlib uncompressed state
                For those who cannot wait the extra few seconds, we changed to keep
                texture data in a zlib compressed state. If the GZ_TEXCACHE or GZ_HIRESTEXCACHE
                option is toggled, the cache will need to be rebuilt.
                */
				
                 /*if (format & GFX_TEXFMT_GZ) {
                   dest = _gzdest0;
                   destLen = _gzdestLen;
                   if (dest && destLen) {
                   if (uncompress(dest, &destLen, (*itMap).second->info.data, (*itMap).second->size) != Z_OK) {
                   dest = nullptr;
                   destLen = 0;
                   }
                   format &= ~GFX_TEXFMT_GZ;
                   }
                   }*/

                if (dest && destLen)
                {
                    // Texture checksum
                    gzwrite(gzfp, &((*itMap).first), 8);

                    // Other texture info
                    gzwrite(gzfp, &((*itMap).second->info.width), 4);
                    gzwrite(gzfp, &((*itMap).second->info.height), 4);
                    gzwrite(gzfp, &format, 2);

                    gzwrite(gzfp, &((*itMap).second->info.smallLodLog2), 4);
                    gzwrite(gzfp, &((*itMap).second->info.largeLodLog2), 4);
                    gzwrite(gzfp, &((*itMap).second->info.aspectRatioLog2), 4);

                    gzwrite(gzfp, &((*itMap).second->info.tiles), 4);
                    gzwrite(gzfp, &((*itMap).second->info.untiled_width), 4);
                    gzwrite(gzfp, &((*itMap).second->info.untiled_height), 4);

                    gzwrite(gzfp, &((*itMap).second->info.is_hires_tex), 1);

                    gzwrite(gzfp, &destLen, 4);
                    gzwrite(gzfp, dest, destLen);
                }

                itMap++;

                // Not ready yet
                /*if (_callback)
                  (*_callback)("Total textures saved to storage: %d\n", std::distance(itMap, _cache.begin()));*/
            }
            gzclose(gzfp);
        }
    }
    return _cache.empty();
}

bool TxCache::load(const char *path, const char *filename, int config)
{
    // Find it on disk
    CPath cbuf(path, filename);

    gzFile gzfp = gzopen(cbuf, "rb");
    DBG_INFO(80, "gzfp:%x file:%ls\n", gzfp, filename);
    if (gzfp)
    {
        // Yep, we have it, let's load it into memory cache
        int dataSize;
        uint64_t checksum;
        GHQTexInfo tmpInfo;
        int tmpconfig;
        // Read header to determine config match
        gzread(gzfp, &tmpconfig, 4);

        if (tmpconfig == config)
        {
            do
            {
                memset(&tmpInfo, 0, sizeof(GHQTexInfo));

                gzread(gzfp, &checksum, 8);

                gzread(gzfp, &tmpInfo.width, 4);
                gzread(gzfp, &tmpInfo.height, 4);
                gzread(gzfp, &tmpInfo.format, 2);

                gzread(gzfp, &tmpInfo.smallLodLog2, 4);
                gzread(gzfp, &tmpInfo.largeLodLog2, 4);
                gzread(gzfp, &tmpInfo.aspectRatioLog2, 4);

                gzread(gzfp, &tmpInfo.tiles, 4);
                gzread(gzfp, &tmpInfo.untiled_width, 4);
                gzread(gzfp, &tmpInfo.untiled_height, 4);

                gzread(gzfp, &tmpInfo.is_hires_tex, 1);

                gzread(gzfp, &dataSize, 4);

                tmpInfo.data = (uint8*)malloc(dataSize);
                if (tmpInfo.data)
                {
                    gzread(gzfp, tmpInfo.data, dataSize);

                    // Add to memory cache
                    add(checksum, &tmpInfo, (tmpInfo.format & GFX_TEXFMT_GZ) ? dataSize : 0);

                    free(tmpInfo.data);
                }
                else
                {
                    gzseek(gzfp, dataSize, SEEK_CUR);
                }

                // Skip in between to prevent the loop from being tied down to v-sync
                if (_callback && (!(_cache.size() % 100) || gzeof(gzfp)))
                    (*_callback)("[%d] total mem:%.02fmb - %ls\n", _cache.size(), (float)_totalSize / 1000000, filename);
            } while (!gzeof(gzfp));
            gzclose(gzfp);
        }
    }
    return !_cache.empty();
}

bool TxCache::del(uint64_t checksum)
{
    if (!checksum || _cache.empty()) return 0;

    std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.find(checksum);
    if (itMap != _cache.end())
    {
        // For texture cache (not high resolution cache)
        if (!_cachelist.empty()) _cachelist.erase(((*itMap).second)->it);

        // Remove from cache
        free((*itMap).second->info.data);
        _totalSize -= (*itMap).second->size;
        delete (*itMap).second;
        _cache.erase(itMap);

        DBG_INFO(80, "Removed from cache: checksum = %08X %08X\n", (uint32)(checksum & 0xffffffff), (uint32)(checksum >> 32));

        return 1;
    }

    return 0;
}

bool TxCache::is_cached(uint64_t checksum)
{
    std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.find(checksum);
    if (itMap != _cache.end()) return 1;

    return 0;
}

void TxCache::clear()
{
    if (!_cache.empty())
    {
        std::map<uint64_t, TXCACHE*>::iterator itMap = _cache.begin();
        while (itMap != _cache.end())
        {
            free((*itMap).second->info.data);
            delete (*itMap).second;
            itMap++;
        }
        _cache.clear();
    }

    if (!_cachelist.empty()) _cachelist.clear();

    _totalSize = 0;
}