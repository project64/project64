/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#include "Gfx_1.3.h"
#include "rdp.h"
#include "DepthBufferRender.h"

uint16_t * zLUT = 0;

void ZLUT_init()
{
    if (zLUT)
        return;
    zLUT = new uint16_t[0x40000];
    for (int i = 0; i < 0x40000; i++)
    {
        uint32_t exponent = 0;
        uint32_t testbit = 1 << 17;
        while ((i & testbit) && (exponent < 7))
        {
            exponent++;
            testbit = 1 << (17 - exponent);
        }

        uint32_t mantissa = (i >> (6 - (6 < exponent ? 6 : exponent))) & 0x7ff;
        zLUT[i] = (uint16_t)(((exponent << 11) | mantissa) << 2);
    }
}

void ZLUT_release()
{
    delete[] zLUT;
    zLUT = 0;
}

static vertexi * max_vtx;                   // Max y vertex (ending vertex)
static vertexi * start_vtx, *end_vtx;      // First and last vertex in array
static vertexi * right_vtx, *left_vtx;     // Current right and left vertex

static int right_height, left_height;
static int right_x, right_dxdy, left_x, left_dxdy;
static int left_z, left_dzdy;

__inline int imul16(int x, int y)        // (x * y) >> 16
{
    return ((int64_t)x * (int64_t)y) >> 16;
}

__inline int imul14(int x, int y)        // (x * y) >> 14
{
    return ((int64_t)x * (int64_t)y) >> 14;
}

__inline int idiv16(int x, int y)        // (x << 16) / y
{
    x = ((int64_t)x << 16) / (int64_t)y;
    return x;
}

__inline int iceil(int x)
{
    x += 0xffff;
    return (x >> 16);
}

static void RightSection(void)
{
    // Walk backwards trough the vertex array

    vertexi * v2, *v1 = right_vtx;
    if (right_vtx > start_vtx) v2 = right_vtx - 1;
    else                      v2 = end_vtx;         // Wrap to end of array
    right_vtx = v2;

    // v1 = top vertex
    // v2 = bottom vertex

    // Calculate number of scanlines in this section

    right_height = iceil(v2->y) - iceil(v1->y);
    if (right_height <= 0) return;

    // Guard against possible div overflows

    if (right_height > 1) {
        // OK, no worries, we have a section that is at least
        // one pixel high. Calculate slope as usual.

        int height = v2->y - v1->y;
        right_dxdy = idiv16(v2->x - v1->x, height);
    }
    else {
        // Height is less or equal to one pixel.
        // Calculate slope = width * 1/height
        // using 18:14 bit precision to avoid overflows.

        int inv_height = (0x10000 << 14) / (v2->y - v1->y);
        right_dxdy = imul14(v2->x - v1->x, inv_height);
    }

    // Prestep initial values

    int prestep = (iceil(v1->y) << 16) - v1->y;
    right_x = v1->x + imul16(prestep, right_dxdy);
}

static void LeftSection(void)
{
    // Walk forward trough the vertex array

    vertexi * v2, *v1 = left_vtx;
    if (left_vtx < end_vtx) v2 = left_vtx + 1;
    else                   v2 = start_vtx;      // Wrap to start of array
    left_vtx = v2;

    // v1 = top vertex
    // v2 = bottom vertex

    // Calculate number of scanlines in this section

    left_height = iceil(v2->y) - iceil(v1->y);
    if (left_height <= 0) return;

    // Guard against possible div overflows

    if (left_height > 1) {
        // OK, no worries, we have a section that is at least
        // one pixel high. Calculate slope as usual.

        int height = v2->y - v1->y;
        left_dxdy = idiv16(v2->x - v1->x, height);
        left_dzdy = idiv16(v2->z - v1->z, height);
    }
    else {
        // Height is less or equal to one pixel.
        // Calculate slope = width * 1/height
        // using 18:14 bit precision to avoid overflows.

        int inv_height = (0x10000 << 14) / (v2->y - v1->y);
        left_dxdy = imul14(v2->x - v1->x, inv_height);
        left_dzdy = imul14(v2->z - v1->z, inv_height);
    }

    // Prestep initial values

    int prestep = (iceil(v1->y) << 16) - v1->y;
    left_x = v1->x + imul16(prestep, left_dxdy);
    left_z = v1->z + imul16(prestep, left_dzdy);
}

void Rasterize(vertexi * vtx, int vertices, int dzdx)
{
    start_vtx = vtx;        // First vertex in array

    // Search trough the vtx array to find min y, max y
    // and the location of these structures.

    vertexi * min_vtx = vtx;
    max_vtx = vtx;

    int min_y = vtx->y;
    int max_y = vtx->y;

    vtx++;

    for (int n = 1; n < vertices; n++) {
        if (vtx->y < min_y) {
            min_y = vtx->y;
            min_vtx = vtx;
        }
        else
            if (vtx->y > max_y) {
                max_y = vtx->y;
                max_vtx = vtx;
            }
        vtx++;
    }

    // OK, now we know where in the array we should start and
    // where to end while scanning the edges of the polygon

    left_vtx = min_vtx;    // Left side starting vertex
    right_vtx = min_vtx;    // Right side starting vertex
    end_vtx = vtx - 1;      // Last vertex in array

    // Search for the first usable right section

    do {
        if (right_vtx == max_vtx) return;
        RightSection();
    } while (right_height <= 0);

    // Search for the first usable left section

    do {
        if (left_vtx == max_vtx) return;
        LeftSection();
    } while (left_height <= 0);

    uint16_t * destptr = (uint16_t*)(gfx.RDRAM + rdp.zimg);
    int y1 = iceil(min_y);
    if (y1 >= (int)rdp.scissor_o.lr_y) return;
    int shift;

    for (;;)
    {
        int x1 = iceil(left_x);
        if (x1 < (int)rdp.scissor_o.ul_x)
            x1 = rdp.scissor_o.ul_x;
        int width = iceil(right_x) - x1;
        if (x1 + width >= (int)rdp.scissor_o.lr_x)
            width = rdp.scissor_o.lr_x - x1 - 1;

        if (width > 0 && y1 >= (int)rdp.scissor_o.ul_y) {
            // Prestep initial z

            int prestep = (x1 << 16) - left_x;
            int z = left_z + imul16(prestep, dzdx);

            shift = x1 + y1*rdp.zi_width;
            //draw to depth buffer
            int trueZ;
            int idx;
            uint16_t encodedZ;
            for (int x = 0; x < width; x++)
            {
                trueZ = z / 8192;
                if (trueZ < 0) trueZ = 0;
                else if (trueZ > 0x3FFFF) trueZ = 0x3FFFF;
                encodedZ = zLUT[trueZ];
                idx = (shift + x) ^ 1;
                if (encodedZ < destptr[idx])
                    destptr[idx] = encodedZ;
                z += dzdx;
            }
        }

        //destptr += rdp.zi_width;
        y1++;
        if (y1 >= (int)rdp.scissor_o.lr_y) return;

        // Scan the right side

        if (--right_height <= 0) {               // End of this section?
            do {
                if (right_vtx == max_vtx) return;
                RightSection();
            } while (right_height <= 0);
        }
        else
            right_x += right_dxdy;

        // Scan the left side

        if (--left_height <= 0) {                // End of this section?
            do {
                if (left_vtx == max_vtx) return;
                LeftSection();
            } while (left_height <= 0);
        }
        else {
            left_x += left_dxdy;
            left_z += left_dzdy;
        }
    }
}
