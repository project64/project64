/*
** Copyright (c) 1995, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** $Header: /devel/cvg/glide3/src/glideutl.h 4     7/24/98 1:41p Hohn $
** $Log: /devel/cvg/glide3/src/glideutl.h $
** 
** 4     7/24/98 1:41p Hohn
** 
** 3     1/30/98 4:27p Atai
** gufog* prototype
** 
** 1     1/29/98 4:00p Atai
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 11    1/07/98 11:18a Atai
 * remove GrMipMapInfo and GrGC.mm_table in glide3
 * 
 * 10    1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 9     1/05/98 6:04p Atai
 * move 3df gu related data structure from glide.h to glideutl.h
 * 
 * 8     12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 7     12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 6     8/14/97 5:32p Pgj
 * remove dead code per GMT
 * 
 * 5     6/12/97 5:19p Pgj
 * Fix bug 578
 * 
 * 4     3/05/97 9:36p Jdt
 * Removed guFbWriteRegion added guEncodeRLE16
 * 
 * 3     1/16/97 3:45p Dow
 * Embedded fn protos in ifndef FX_GLIDE_NO_FUNC_PROTO 
*/

/* Glide Utility routines */

#ifndef __GLIDEUTL_H__
#define __GLIDEUTL_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
** 3DF texture file structs
*/

typedef struct
{
  FxU32               width, height;
  int                 small_lod, large_lod;
  GrAspectRatio_t     aspect_ratio;
  GrTextureFormat_t   format;
} Gu3dfHeader;

typedef struct
{
  FxU8  yRGB[16];
  FxI16 iRGB[4][3];
  FxI16 qRGB[4][3];
  FxU32 packed_data[12];
} GuNccTable;

typedef struct {
    FxU32 data[256];
} GuTexPalette;

typedef union {
    GuNccTable   nccTable;
    GuTexPalette palette;
} GuTexTable;

typedef struct
{
  Gu3dfHeader  header;
  GuTexTable   table;
  void        *data;
  FxU32        mem_required;    /* memory required for mip map in bytes. */
} Gu3dfInfo;

#ifndef FX_GLIDE_NO_FUNC_PROTO
/*
** Gamma functions
*/

FX_ENTRY void FX_CALL 
guGammaCorrectionRGB( FxFloat red, FxFloat green, FxFloat blue );

/*
** fog stuff
*/
FX_ENTRY float FX_CALL
guFogTableIndexToW( int i );

FX_ENTRY void FX_CALL
guFogGenerateExp( GrFog_t *fogtable, float density );

FX_ENTRY void FX_CALL
guFogGenerateExp2( GrFog_t *fogtable, float density );

FX_ENTRY void FX_CALL
guFogGenerateLinear(GrFog_t *fogtable,
                    float nearZ, float farZ );

/*
** hi-level texture manipulation tools.
*/
FX_ENTRY FxBool FX_CALL
gu3dfGetInfo( const char *filename, Gu3dfInfo *info );

FX_ENTRY FxBool FX_CALL
gu3dfLoad( const char *filename, Gu3dfInfo *data );

#endif /* FX_GLIDE_NO_FUNC_PROTO */

#ifdef __cplusplus
}
#endif

#endif /* __GLIDEUTL_H__ */
