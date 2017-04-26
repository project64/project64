/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2007  Hiroshi Morii                                        *
* Copyright (C) 2004  Daniel Borca                                         *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

#ifndef INTERNAL_H_included
#define INTERNAL_H_included

/*****************************************************************************\
* Texture compression stuff
\*****************************************************************************/
#define RADEON
#define YUV
#define ARGB

/*****************************************************************************\
 * DLL stuff
\*****************************************************************************/

#define TAPI
#define TAPIENTRY

/*****************************************************************************\
 * 64bit types on 32bit machine
\*****************************************************************************/

#if (defined(__GNUC__) && !defined(__cplusplus)) || defined(WIN32)

typedef unsigned long long qword;

#define Q_MOV32(a, b) a = b
#define Q_OR32(a, b)  a |= b
#define Q_SHL(a, c)   a <<= c

#else  /* !__GNUC__ */

typedef struct {
    dword lo, hi;
} qword;

#define Q_MOV32(a, b) a.lo = b
#define Q_OR32(a, b)  a.lo |= b
#define Q_SHL(a, c)					\
    do {						\
	if ((c) >= 32) {				\
	    a.hi = a.lo << ((c) - 32);			\
	    a.lo = 0;					\
	} else {					\
	    a.hi = (a.hi << (c)) | (a.lo >> (32 - (c)));\
	    a.lo <<= c;					\
	}						\
    } while (0)

#endif /* !__GNUC__ */

/*****************************************************************************\
 * Config
\*****************************************************************************/

#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3

/*****************************************************************************\
 * Metric
\*****************************************************************************/

#define F(i) (float)1 /* can be used to obtain an oblong metric: 0.30 / 0.59 / 0.11 */
#define SAFECDOT 1 /* for paranoids */

#define MAKEIVEC(NV, NC, IV, B, V0, V1)	\
    do {				\
	/* compute interpolation vector */\
	float d2 = 0.0F;		\
	float rd2;			\
					\
	for (i = 0; i < NC; i++) {	\
	    IV[i] = (V1[i] - V0[i]) * F(i);\
	    d2 += IV[i] * IV[i];	\
	}				\
	rd2 = (float)NV / d2;		\
	B = 0;				\
	for (i = 0; i < NC; i++) {	\
	    IV[i] *= F(i);		\
	    B -= IV[i] * V0[i];		\
	    IV[i] *= rd2;		\
	}				\
	B = B * rd2 + 0.5F;		\
    } while (0)

#define CALCCDOT(TEXEL, NV, NC, IV, B, V)\
    do {				\
	float dot = 0.0F;		\
	for (i = 0; i < NC; i++) {	\
	    dot += V[i] * IV[i];	\
	}				\
	TEXEL = (int)(dot + B);		\
	if (SAFECDOT) {			\
	    if (TEXEL < 0) {		\
		TEXEL = 0;		\
	    } else if (TEXEL > NV) {	\
		TEXEL = NV;		\
	    }				\
	}				\
    } while (0)

/*****************************************************************************\
 * Utility functions
\*****************************************************************************/

void
_mesa_upscale_teximage2d(unsigned int inWidth, unsigned int inHeight,
    unsigned int outWidth, unsigned int outHeight,
    unsigned int comps,
    const byte *src, int srcRowStride,
    unsigned char *dest);

#endif
