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
#ifdef _WIN32
#pragma warning(disable: 4786)
#endif

#include <Common/path.h>
#include <Common/StdString.h>
#include <Project64-video/Renderer/types.h>
#include "TxFilter.h"
#include "TextureFilters.h"
#include "TxDbg.h"

void TxFilter::clear()
{
    /* clear hires texture cache */
    delete _txHiResCache;

    /* clear texture cache */
    delete _txTexCache;

    /* free memory */
    TxMemBuf::getInstance()->shutdown();

    /* clear other stuff */
    delete _txImage;
    delete _txQuantize;
    delete _txUtil;
}

TxFilter::~TxFilter()
{
    clear();
}

TxFilter::TxFilter(int maxwidth, int maxheight, int maxbpp, int options,
    int cachesize, const char *path, const char *ident,
    dispInfoFuncExt callback) :
    _numcore(0),
    _tex1(NULL),
    _tex2(NULL),
    _maxwidth(0),
    _maxheight(0),
    _maxbpp(0),
    _options(0),
    _cacheSize(0),
    _txQuantize(NULL),
    _txTexCache(NULL),
    _txHiResCache(NULL),
    _txUtil(NULL),
    _txImage(NULL),
    _initialized(false)
{
    /* HACKALERT: the emulator misbehaves and sometimes forgets to shutdown */
    if ((ident && strcmp(ident, "DEFAULT") != 0 && _ident.compare(ident) == 0) &&
        _maxwidth == maxwidth  &&
        _maxheight == maxheight &&
        _maxbpp == maxbpp    &&
        _options == options   &&
        _cacheSize == cachesize) return;
    clear(); /* gcc does not allow the destructor to be called */

    /* shamelessness :P this first call to the debug output message creates
     * a file in the executable directory. */
    INFO(0, "------------------------------------------------------------------\n");
#ifdef GHQCHK
    INFO(0, " GlideHQ Hires Texture Checker 1.02.00.%d\n", 0);
#else
    INFO(0, " GlideHQ version 1.02.00.%d\n", 0);
#endif
    INFO(0, " Copyright (C) 2010  Hiroshi Morii   All Rights Reserved\n");
    INFO(0, "    email   : koolsmoky(at)users.sourceforge.net\n");
    INFO(0, "    website : http://www.3dfxzone.it/koolsmoky\n");
    INFO(0, "\n");
    INFO(0, " Glide64 official website : http://glide64.emuxhaven.net\n");
    INFO(0, "------------------------------------------------------------------\n");

    _options = options;

    _txImage = new TxImage();
    _txQuantize = new TxQuantize();
    _txUtil = new TxUtil();

    /* get number of CPU cores. */
    _numcore = _txUtil->getNumberofProcessors();

    _initialized = 0;

    _tex1 = NULL;
    _tex2 = NULL;

    /* XXX: anything larger than 1024 * 1024 is overkill */
    _maxwidth = maxwidth > 1024 ? 1024 : maxwidth;
    _maxheight = maxheight > 1024 ? 1024 : maxheight;
    _maxbpp = maxbpp;

    _cacheSize = cachesize;

    /* TODO: validate options and do overrides here*/

    /* save path name */
    if (path)
        _path.assign(path);

    /* save ROM name */
    if (ident && strcmp(ident, "DEFAULT") != 0)
        _ident.assign(ident);

    /* check for dxtn extensions */
    if (!TxLoadLib::getInstance()->getdxtCompressTexFuncExt())
        _options &= ~S3TC_COMPRESSION;

    if (!TxLoadLib::getInstance()->getfxtCompressTexFuncExt())
        _options &= ~FXT1_COMPRESSION;

    switch (options & COMPRESSION_MASK) {
    case FXT1_COMPRESSION:
    case S3TC_COMPRESSION:
        break;
    case NCC_COMPRESSION:
    default:
        _options &= ~COMPRESSION_MASK;
    }

    if (TxMemBuf::getInstance()->init(_maxwidth, _maxheight)) {
        if (!_tex1)
            _tex1 = TxMemBuf::getInstance()->get(0);

        if (!_tex2)
            _tex2 = TxMemBuf::getInstance()->get(1);
    }

#if !_16BPP_HACK
    /* initialize hq4x filter */
    hq4x_init();
#endif

    /* initialize texture cache in bytes. 128Mb will do nicely in most cases */
    _txTexCache = new TxTexCache(_options, _cacheSize, _path.c_str(), _ident.c_str(), callback);

    /* hires texture */
#if HIRES_TEXTURE
    _txHiResCache = new TxHiResCache(_maxwidth, _maxheight, _maxbpp, _options, _path.c_str(), _ident.c_str(), callback);

    if (_txHiResCache->empty())
        _options &= ~HIRESTEXTURES_MASK;
#endif

    if (!(_options & COMPRESS_TEX))
        _options &= ~COMPRESSION_MASK;

    if (_tex1 && _tex2)
        _initialized = 1;
}

bool
TxFilter::filter(uint8 *src, int srcwidth, int srcheight, uint16 srcformat, uint64_t g64crc, GHQTexInfo *info)
{
    uint8 *texture = src;
    uint8 *tmptex = _tex1;
    uint16 destformat = srcformat;

    /* We need to be initialized first! */
    if (!_initialized) return 0;

    /* find cached textures */
    if (_cacheSize) {
        /* calculate checksum of source texture */
        if (!g64crc)
            g64crc = (uint64_t)(_txUtil->checksumTx(texture, srcwidth, srcheight, srcformat));

        DBG_INFO(80, "filter: crc:%08X %08X %d x %d gfmt:%x\n",
            (uint32)(g64crc >> 32), (uint32)(g64crc & 0xffffffff), srcwidth, srcheight, srcformat);

#if 0 /* use hirestex to retrieve cached textures. */
        /* check if we have it in cache */
        if (!(g64crc & 0xffffffff00000000) && /* we reach here only when there is no hires texture for this crc */
            _txTexCache->get(g64crc, info)) {
            DBG_INFO(80, "cache hit: %d x %d gfmt:%x\n", info->width, info->height, info->format);
            return 1; /* yep, we've got it */
        }
#endif
    }

    /* Leave small textures alone because filtering makes little difference.
     * Moreover, some filters require at least 4 * 4 to work.
     * Bypass _options to do ARGB8888->16bpp if _maxbpp=16 or forced color reduction.
     */
    if ((srcwidth >= 4 && srcheight >= 4) &&
        ((_options & (FILTER_MASK | ENHANCEMENT_MASK | COMPRESSION_MASK)) ||
        (srcformat == GFX_TEXFMT_ARGB_8888 && (_maxbpp < 32 || _options & FORCE16BPP_TEX)))) {
#if !_16BPP_HACK
        /* convert textures to a format that the compressor accepts (ARGB8888) */
        if (_options & COMPRESSION_MASK) {
#endif
            if (srcformat != GFX_TEXFMT_ARGB_8888) {
                if (!_txQuantize->quantize(texture, tmptex, srcwidth, srcheight, srcformat, GFX_TEXFMT_ARGB_8888)) {
                    DBG_INFO(80, "Error: unsupported format! gfmt:%x\n", srcformat);
                    return 0;
                }
                texture = tmptex;
                destformat = GFX_TEXFMT_ARGB_8888;
            }
#if !_16BPP_HACK
        }
#endif

        switch (destformat) {
        case GFX_TEXFMT_ARGB_8888:

            /*
             * prepare texture enhancements (x2, x4 scalers)
             */
            int scale_shift = 0, num_filters = 0;
            uint32 filter = 0;

            if ((_options & ENHANCEMENT_MASK) == HQ4X_ENHANCEMENT) {
                if (srcwidth <= (_maxwidth >> 2) && srcheight <= (_maxheight >> 2)) {
                    filter |= HQ4X_ENHANCEMENT;
                    scale_shift = 2;
                    num_filters++;
                }
                else if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    filter |= HQ2X_ENHANCEMENT;
                    scale_shift = 1;
                    num_filters++;
                }
            }
            else if (_options & ENHANCEMENT_MASK) {
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    filter |= (_options & ENHANCEMENT_MASK);
                    scale_shift = 1;
                    num_filters++;
                }
            }

            /*
             * prepare texture filters
             */
            if (_options & (SMOOTH_FILTER_MASK | SHARP_FILTER_MASK)) {
                filter |= (_options & (SMOOTH_FILTER_MASK | SHARP_FILTER_MASK));
                num_filters++;
            }

            /*
             * execute texture enhancements and filters
             */
            while (num_filters > 0) {
                tmptex = (texture == _tex1) ? _tex2 : _tex1;

                uint8 *_texture = texture;
                uint8 *_tmptex = tmptex;

                unsigned int numcore = _numcore;
                unsigned int blkrow = 0;
                while (numcore > 1 && blkrow == 0) {
                    blkrow = (srcheight >> 2) / numcore;
                    numcore--;
                }
                filter_8888((uint32*)_texture, srcwidth, srcheight, (uint32*)_tmptex, filter);

                if (filter & ENHANCEMENT_MASK) {
                    srcwidth <<= scale_shift;
                    srcheight <<= scale_shift;
                    filter &= ~ENHANCEMENT_MASK;
                    scale_shift = 0;
                }

                texture = tmptex;
                num_filters--;
            }

            /*
             * texture compression
             */
             /* ignored if we only have texture compression option on.
              * only done when texture enhancer is used. see constructor. */
            if ((_options & COMPRESSION_MASK) &&
                (srcwidth >= 64 && srcheight >= 64) /* Texture compression is not suitable for low pixel coarse detail
                                                     * textures. The assumption here is that textures larger than 64x64
                                                     * have enough detail to produce decent quality when compressed. The
                                                     * down side is that narrow stripped textures that the N64 often use
                                                     * for large background textures are also ignored. It would be more
                                                     * reasonable if decisions are made based on fourier-transform
                                                     * spectrum or RMS error.
                                                     */
                ) {
                int compressionType = _options & COMPRESSION_MASK;
                int tmpwidth, tmpheight;
                uint16 tmpformat;
                /* XXX: textures that use 8bit alpha channel look bad with the current
                 * fxt1 library, so we substitute it with dxtn for now. afaik all gfx
                 * cards that support fxt1 also support dxtn. (3dfx and Intel) */
                if ((destformat == GFX_TEXFMT_ALPHA_INTENSITY_88) ||
                    (destformat == GFX_TEXFMT_ARGB_8888) ||
                    (destformat == GFX_TEXFMT_ALPHA_8)) {
                    compressionType = S3TC_COMPRESSION;
                }
                tmptex = (texture == _tex1) ? _tex2 : _tex1;
                if (_txQuantize->compress(texture, tmptex,
                    srcwidth, srcheight, srcformat,
                    &tmpwidth, &tmpheight, &tmpformat,
                    compressionType)) {
                    srcwidth = tmpwidth;
                    srcheight = tmpheight;
                    destformat = tmpformat;
                    texture = tmptex;
                }
            }

            /*
             * texture (re)conversions
             */
            if (destformat == GFX_TEXFMT_ARGB_8888) {
                if (srcformat == GFX_TEXFMT_ARGB_8888 && (_maxbpp < 32 || _options & FORCE16BPP_TEX)) srcformat = GFX_TEXFMT_ARGB_4444;
                if (srcformat != GFX_TEXFMT_ARGB_8888) {
                    tmptex = (texture == _tex1) ? _tex2 : _tex1;
                    if (!_txQuantize->quantize(texture, tmptex, srcwidth, srcheight, GFX_TEXFMT_ARGB_8888, srcformat)) {
                        DBG_INFO(80, "Error: unsupported format! gfmt:%x\n", srcformat);
                        return 0;
                    }
                    texture = tmptex;
                    destformat = srcformat;
                }
            }

            break;
#if !_16BPP_HACK
        case GFX_TEXFMT_ARGB_4444:

            int scale_shift = 0;
            tmptex = (texture == _tex1) ? _tex2 : _tex1;

            switch (_options & ENHANCEMENT_MASK) {
            case HQ4X_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 2) && srcheight <= (_maxheight >> 2)) {
                    hq4x_4444((uint8*)texture, (uint8*)tmptex, srcwidth, srcheight, srcwidth, srcwidth * 4 * 2);
                    scale_shift = 2;
                }/* else if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                  hq2x_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                  scale_shift = 1;
                  }*/
                break;
            case HQ2X_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    hq2x_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                    scale_shift = 1;
                }
                break;
            case HQ2XS_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    hq2xS_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                    scale_shift = 1;
                }
                break;
            case LQ2X_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    lq2x_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                    scale_shift = 1;
                }
                break;
            case LQ2XS_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    lq2xS_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                    scale_shift = 1;
                }
                break;
            case X2SAI_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    Super2xSaI_4444((uint16*)texture, (uint16*)tmptex, srcwidth, srcheight, srcwidth);
                    scale_shift = 1;
                }
                break;
            case X2_ENHANCEMENT:
                if (srcwidth <= (_maxwidth >> 1) && srcheight <= (_maxheight >> 1)) {
                    Texture2x_16((uint8*)texture, srcwidth * 2, (uint8*)tmptex, srcwidth * 2 * 2, srcwidth, srcheight);
                    scale_shift = 1;
                }
            }
            if (scale_shift) {
                srcwidth <<= scale_shift;
                srcheight <<= scale_shift;
                texture = tmptex;
            }

            if (_options & SMOOTH_FILTER_MASK) {
                tmptex = (texture == _tex1) ? _tex2 : _tex1;
                SmoothFilter_4444((uint16*)texture, srcwidth, srcheight, (uint16*)tmptex, (_options & SMOOTH_FILTER_MASK));
                texture = tmptex;
            }
            else if (_options & SHARP_FILTER_MASK) {
                tmptex = (texture == _tex1) ? _tex2 : _tex1;
                SharpFilter_4444((uint16*)texture, srcwidth, srcheight, (uint16*)tmptex, (_options & SHARP_FILTER_MASK));
                texture = tmptex;
            }

            break;
        case GFX_TEXFMT_ARGB_1555:
            break;
        case GFX_TEXFMT_RGB_565:
            break;
        case GFX_TEXFMT_ALPHA_8:
            break;
#endif /* _16BPP_HACK */
        }
    }

    /* fill in the texture info. */
    info->data = texture;
    info->width = srcwidth;
    info->height = srcheight;
    info->format = destformat;
    info->smallLodLog2 = _txUtil->grLodLog2(srcwidth, srcheight);
    info->largeLodLog2 = info->smallLodLog2;
    info->aspectRatioLog2 = _txUtil->grAspectRatioLog2(srcwidth, srcheight);
    info->is_hires_tex = 0;

    /* cache the texture. */
    if (_cacheSize) _txTexCache->add(g64crc, info);

    DBG_INFO(80, "filtered texture: %d x %d gfmt:%x\n", info->width, info->height, info->format);

    return 1;
}

bool
TxFilter::hirestex(uint64_t g64crc, uint64_t r_crc64, uint16 *palette, GHQTexInfo *info)
{
    /* NOTE: Rice CRC32 sometimes return the same value for different textures.
     * As a workaround, Glide64 CRC32 is used for the key for NON-hires
     * texture cache.
     *
     * r_crc64 = hi:palette low:texture
     *           (separate crc. doesn't necessary have to be rice crc)
     * g64crc  = texture + palette glide64 crc32
     *           (can be any other crc if robust)
     */

    DBG_INFO(80, "hirestex: r_crc64:%08X %08X, g64crc:%08X %08X\n",
        (uint32)(r_crc64 >> 32), (uint32)(r_crc64 & 0xffffffff),
        (uint32)(g64crc >> 32), (uint32)(g64crc & 0xffffffff));

#if HIRES_TEXTURE
    /* check if we have it in hires memory cache. */
    if ((_options & HIRESTEXTURES_MASK) && r_crc64) {
        if (_txHiResCache->get(r_crc64, info)) {
            DBG_INFO(80, "hires hit: %d x %d gfmt:%x\n", info->width, info->height, info->format);

            /* TODO: Enable emulation for special N64 combiner modes. There are few ways
             * to get this done. Also applies for CI textures below.
             *
             * Solution 1. Load the hiresolution textures in ARGB8888 (or A8, IA88) format
             * to cache. When a cache is hit, then we take the modes passed in from Glide64
             * (also TODO) and apply the modification. Then we do color reduction or format
             * conversion or compression if desired and stuff it into the non-hires texture
             * cache.
             *
             * Solution 2. When a cache is hit and if the combiner modes are present,
             * convert the texture to ARGB4444 and pass it back to Glide64 to process.
             * If a texture is compressed, it needs to be decompressed first. Then add
             * the processed texture to the non-hires texture cache.
             *
             * Solution 3. Hybrid of the above 2. Load the textures in ARGB8888 (A8, IA88)
             * format. Convert the texture to ARGB4444 and pass it back to Glide64 when
             * the combiner modes are present. Get the processed texture back from Glide64
             * and compress if desired and add it to the non-hires texture cache.
             *
             * Solution 4. Take the easy way out and forget about this whole thing.
             */

            return 1; /* yep, got it */
        }
        if (_txHiResCache->get((r_crc64 & 0xffffffff), info)) {
            DBG_INFO(80, "hires hit: %d x %d gfmt:%x\n", info->width, info->height, info->format);

            /* for true CI textures, we use the passed in palette to convert to
             * ARGB1555 and add it to memory cache.
             *
             * NOTE: we do this AFTER all other texture cache searches because
             * only a few texture packs actually use true CI textures.
             *
             * NOTE: the pre-converted palette from Glide64 is in RGBA5551 format.
             * A comp comes before RGB comp.
             */
            if (palette && info->format == GFX_TEXFMT_P_8) {
                DBG_INFO(80, "found GFX_TEXFMT_P_8 format. Need conversion!!\n");

                int width = info->width;
                int height = info->height;
                uint16 format = info->format;
                /* XXX: avoid collision with zlib compression buffer in TxHiResTexture::get */
                uint8 *texture = info->data;
                uint8 *tmptex = (texture == _tex1) ? _tex2 : _tex1;

                /* use palette and convert to 16bit format */
                _txQuantize->P8_16BPP((uint32*)texture, (uint32*)tmptex, info->width, info->height, (uint32*)palette);
                texture = tmptex;
                format = GFX_TEXFMT_ARGB_1555;

#if 1
                /* XXX: compressed if memory cache compression is ON */
                if (_options & COMPRESSION_MASK) {
                    tmptex = (texture == _tex1) ? _tex2 : _tex1;
                    if (_txQuantize->quantize(texture, tmptex, info->width, info->height, format, GFX_TEXFMT_ARGB_8888)) {
                        texture = tmptex;
                        format = GFX_TEXFMT_ARGB_8888;
                    }
                    if (format == GFX_TEXFMT_ARGB_8888) {
                        tmptex = (texture == _tex1) ? _tex2 : _tex1;
                        if (_txQuantize->compress(texture, tmptex,
                            info->width, info->height, GFX_TEXFMT_ARGB_1555,
                            &width, &height, &format,
                            _options & COMPRESSION_MASK)) {
                            texture = tmptex;
                        }
                        else {
                            /*if (!_txQuantize->quantize(texture, tmptex, info->width, info->height, GFX_TEXFMT_ARGB_8888, GFX_TEXFMT_ARGB_1555)) {
                              DBG_INFO(80, "Error: unsupported format! gfmt:%x\n", format);
                              return 0;
                              }*/
                            texture = tmptex;
                            format = GFX_TEXFMT_ARGB_1555;
                        }
                    }
                }
#endif

                /* fill in the required info to return */
                info->data = texture;
                info->width = width;
                info->height = height;
                info->format = format;
                info->smallLodLog2 = _txUtil->grLodLog2(width, height);
                info->largeLodLog2 = info->smallLodLog2;
                info->aspectRatioLog2 = _txUtil->grAspectRatioLog2(width, height);
                info->is_hires_tex = 1;

                /* XXX: add to hires texture cache!!! */
                _txHiResCache->add(r_crc64, info);

                DBG_INFO(80, "GFX_TEXFMT_P_8 loaded as gfmt:%x!\n", format);
            }

            return 1;
        }
    }
#endif

    /* check if we have it in memory cache */
    if (_cacheSize && g64crc)
    {
        if (_txTexCache->get(g64crc, info))
        {
            DBG_INFO(80, "cache hit: %d x %d gfmt:%x\n", info->width, info->height, info->format);
            return 1; /* yep, we've got it */
        }
    }

    DBG_INFO(80, "no cache hits.\n");

    return 0;
}

uint64_t
TxFilter::checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette)
{
    if (_options & (HIRESTEXTURES_MASK | DUMP_TEX))
    {
        return _txUtil->checksum64(src, width, height, size, rowStride, palette);
    }
    return 0;
}

bool
TxFilter::dmptx(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, uint16 n64fmt, uint64_t r_crc64)
{
    if (!_initialized)
    {
        return 0;
    }
    if (!(_options & DUMP_TEX))
    {
        return 0;
    }
    DBG_INFO(80, "gfmt = %02x n64fmt = %02x\n", gfmt, n64fmt);
    DBG_INFO(80, "hirestex: r_crc64:%08X %08X\n",
        (uint32)(r_crc64 >> 32), (uint32)(r_crc64 & 0xffffffff));

    if (!_txQuantize->quantize(src, _tex1, rowStridePixel, height, (gfmt & 0x00ff), GFX_TEXFMT_ARGB_8888))
    {
        return 0;
    }

    src = _tex1;

    if (!_path.empty() && !_ident.empty())
    {
        /* dump it to disk */
        FILE *fp = NULL;
        CPath tmpbuf(_path.c_str(), "");

        /* create directories */
        tmpbuf.AppendDirectory("texture_dump");

        if (!tmpbuf.DirectoryExists() && !tmpbuf.DirectoryCreate())
        {
            return 0;
        }

        tmpbuf.AppendDirectory(_ident.c_str());
        if (!tmpbuf.DirectoryExists() && !tmpbuf.DirectoryCreate())
        {
            return 0;
        }

        tmpbuf.AppendDirectory("GlideHQ");
        if (!tmpbuf.DirectoryExists() && !tmpbuf.DirectoryCreate())
        {
            return 0;
        }

        if ((n64fmt >> 8) == 0x2)
        {
            tmpbuf.SetNameExtension(stdstr_f("%ls#%08X#%01X#%01X#%08X_ciByRGBA.png", _ident.c_str(), (uint32)(r_crc64 & 0xffffffff), (n64fmt >> 8), (n64fmt & 0xf), (uint32)(r_crc64 >> 32)).c_str());
        }
        else
        {
            tmpbuf.SetNameExtension(stdstr_f("%ls#%08X#%01X#%01X_all.png", _ident.c_str(), (uint32)(r_crc64 & 0xffffffff), (n64fmt >> 8), (n64fmt & 0xf)).c_str());
        }
        if ((fp = fopen(tmpbuf, "wb")) != NULL)
        {
            _txImage->writePNG(src, fp, width, height, (rowStridePixel << 2), 0x0003, 0);
            fclose(fp);
            return 1;
        }
    }

    return 0;
}

bool TxFilter::reloadhirestex()
{
    DBG_INFO(80, "Reload hires textures from texture pack.\n");

    if (_txHiResCache->load(0)) {
        if (_txHiResCache->empty()) _options &= ~HIRESTEXTURES_MASK;
        else _options |= HIRESTEXTURES_MASK;

        return 1;
    }

    return 0;
}