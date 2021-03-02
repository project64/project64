// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

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
