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

#ifndef __TXHIRESCACHE_H__
#define __TXHIRESCACHE_H__

/* support hires textures
 *   0: disable
 *   1: enable
 */
#define HIRES_TEXTURE 1

#include "TxCache.h"
#include "TxQuantize.h"
#include "TxImage.h"
#include "TxReSample.h"

class TxHiResCache : public TxCache
{
private:
    int _maxwidth;
    int _maxheight;
    int _maxbpp;
    bool _haveCache;
    bool _abortLoad;
    TxImage *_txImage;
    TxQuantize *_txQuantize;
    TxReSample *_txReSample;
    bool loadHiResTextures(const char * dir_path, bool replace);
public:
    ~TxHiResCache();
    TxHiResCache(int maxwidth, int maxheight, int maxbpp, int options,
        const char *path, const char *ident,
        dispInfoFuncExt callback);
    bool empty();
    bool load(bool replace);
};

#endif /* __TXHIRESCACHE_H__ */
