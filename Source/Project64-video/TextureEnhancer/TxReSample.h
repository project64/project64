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

#ifndef __TXRESAMPLE_H__
#define __TXRESAMPLE_H__

#include "TxInternal.h"

class TxReSample
{
private:
    double tent(double x);
    double gaussian(double x);
    double sinc(double x);
    double lanczos3(double x);
    double mitchell(double x);
    double besselI0(double x);
    double kaiser(double x);
public:
    bool minify(uint8 **src, int *width, int *height, int ratio);
    bool nextPow2(uint8** image, int* width, int* height, int bpp, bool use_3dfx);
    int nextPow2(int num);
};

#endif /* __TXRESAMPLE_H__ */
