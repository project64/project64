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

static inline void load4bCI(uint8_t *src, uint8_t *dst, int wid_64, int height, uint16_t line, int ext, uint16_t *pal)
{
    uint8_t *v7;
    uint8_t *v8;
    int v9;
    int v10;
    int v11;
    uint32_t v12;
    uint8_t *v13;
    uint32_t v14;
    uint32_t *v15;
    uint32_t v16;
    uint8_t *v17;
    uint32_t *v18;
    int v19;
    int v20;
    uint32_t v21;
    uint32_t v22;
    uint32_t *v23;
    uint32_t v24;
    int v25;
    int v26;

    v7 = src;
    v8 = dst;
    v9 = height;
    do
    {
        v25 = v9;
        v10 = wid_64;
        do
        {
            v11 = v10;
            v12 = bswap32(*(uint32_t *)v7);
            v13 = v7 + 4;
            ALOWORD(v10) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 23) & 0x1E)), 1);
            v14 = v10 << 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 27) & 0x1E)), 1);
            *(uint32_t *)v8 = v14;
            v15 = (uint32_t *)(v8 + 4);
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 15) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 19) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 7) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 11) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)(v12 & 0xFF) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 3) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            v16 = bswap32(*(uint32_t *)v13);
            v7 = v13 + 4;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 23) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 27) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 15) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 19) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 7) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 11) & 0x1E)), 1);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)(v16 & 0xFF) & 0x1E)), 1);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 3) & 0x1E)), 1);
            *v15 = v14;
            v8 = (uint8_t *)(v15 + 1);
            v10 = v11 - 1;
        } while (v11 != 1);
        if (v25 == 1)
            break;
        v26 = v25 - 1;
        v17 = &src[(line + (uintptr_t)v7 - (uintptr_t)src) & 0x7FF];
        v18 = (uint32_t *)&v8[ext];
        v19 = wid_64;
        do
        {
            v20 = v19;
            v21 = bswap32(*((uint32_t *)v17 + 1));
            ALOWORD(v19) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 23) & 0x1E)), 1);
            v22 = v19 << 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 27) & 0x1E)), 1);
            *v18 = v22;
            v23 = v18 + 1;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 15) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 19) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 7) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 11) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)(v21 & 0xFF) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 3) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            v24 = bswap32(*(uint32_t *)v17);
            v17 = &src[((uintptr_t)v17 + 8 - (uintptr_t)src) & 0x7FF];
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 23) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 27) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 15) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 19) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 7) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 11) & 0x1E)), 1);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)(v24 & 0xFF) & 0x1E)), 1);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 3) & 0x1E)), 1);
            *v23 = v22;
            v18 = v23 + 1;
            v19 = v20 - 1;
        } while (v20 != 1);
        v7 = &src[(line + (uintptr_t)v17 - (uintptr_t)src) & 0x7FF];
        v8 = (uint8_t *)((char *)v18 + ext);
        v9 = v26 - 1;
    } while (v26 != 1);
}

static inline void load4bIAPal(uint8_t *src, uint8_t *dst, int wid_64, int height, int line, int ext, uint16_t *pal)
{
    uint8_t *v7;
    uint32_t *v8;
    int v9;
    int v10;
    int v11;
    uint32_t v12;
    uint32_t *v13;
    uint32_t v14;
    uint32_t *v15;
    uint32_t v16;
    uint8_t *v17;
    uint32_t *v18;
    int v19;
    int v20;
    uint32_t v21;
    uint32_t v22;
    uint32_t *v23;
    uint32_t v24;
    int v25;
    int v26;

    v7 = src;
    v8 = (uint32_t *)dst;
    v9 = height;
    do
    {
        v25 = v9;
        v10 = wid_64;
        do
        {
            v11 = v10;
            v12 = bswap32(*(uint32_t *)v7);
            v13 = (uint32_t *)(v7 + 4);
            ALOWORD(v10) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 23) & 0x1E)), 8);
            v14 = v10 << 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 27) & 0x1E)), 8);
            *v8 = v14;
            v15 = v8 + 1;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 15) & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 19) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 7) & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 11) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)v12 & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v12 >> 3) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            v16 = bswap32(*v13);
            v7 = (uint8_t *)(v13 + 1);
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 23) & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 27) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 15) & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 19) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 7) & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 11) & 0x1E)), 8);
            *v15 = v14;
            ++v15;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)v16 & 0x1E)), 8);
            v14 <<= 16;
            ALOWORD(v14) = __ROR__(*(uint16_t *)((char *)pal + ((v16 >> 3) & 0x1E)), 8);
            *v15 = v14;
            v8 = v15 + 1;
            v10 = v11 - 1;
        } while (v11 != 1);
        if (v25 == 1)
            break;
        v26 = v25 - 1;
        v17 = &src[(line + (uintptr_t)v7 - (uintptr_t)src) & 0x7FF];
        v18 = (uint32_t *)((char *)v8 + ext);
        v19 = wid_64;
        do
        {
            v20 = v19;
            v21 = bswap32(*((uint32_t *)v17 + 1));
            ALOWORD(v19) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 23) & 0x1E)), 8);
            v22 = v19 << 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 27) & 0x1E)), 8);
            *v18 = v22;
            v23 = v18 + 1;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 15) & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 19) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 7) & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 11) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)v21 & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v21 >> 3) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            v24 = bswap32(*(uint32_t *)v17);
            v17 = &src[((uintptr_t)v17 + 8 - (uintptr_t)src) & 0x7FF];
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 23) & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 27) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 15) & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 19) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 7) & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 11) & 0x1E)), 8);
            *v23 = v22;
            ++v23;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + (2 * (uint8_t)v24 & 0x1E)), 8);
            v22 <<= 16;
            ALOWORD(v22) = __ROR__(*(uint16_t *)((char *)pal + ((v24 >> 3) & 0x1E)), 8);
            *v23 = v22;
            v18 = v23 + 1;
            v19 = v20 - 1;
        } while (v20 != 1);
        v7 = &src[(line + (uintptr_t)v17 - (uintptr_t)src) & 0x7FF];
        v8 = (uint32_t *)((char *)v18 + ext);
        v9 = v26 - 1;
    } while (v26 != 1);
}

static inline void load4bIA(uint8_t *src, uint8_t *dst, int wid_64, int height, int line, int ext)
{
    uint32_t *v6;
    uint32_t *v7;
    int v8;
    int v9;
    int v10;
    uint32_t v11;
    uint32_t *v12;
    uint32_t v13;
    uint32_t v14;
    uint32_t v15;
    uint32_t *v16;
    uint32_t v17;
    uint32_t v18;
    uint32_t v19;
    uint32_t v20;
    uint32_t v21;
    uint32_t v22;
    uint32_t v23;
    uint32_t v24;
    uint32_t v25;
    uint32_t v26;
    uint32_t v27;
    uint32_t v28;
    uint32_t v29;
    uint32_t v30;
    uint32_t v31;
    uint32_t v32;
    uint32_t *v33;
    uint32_t *v34;
    int v35;
    int v36;
    uint32_t v37;
    uint32_t v38;
    uint32_t v39;
    uint32_t *v40;
    uint32_t v41;
    uint32_t v42;
    uint32_t v43;
    uint32_t v44;
    uint32_t v45;
    uint32_t v46;
    uint32_t v47;
    uint32_t v48;
    uint32_t v49;
    uint32_t v50;
    uint32_t v51;
    uint32_t v52;
    uint32_t v53;
    uint32_t v54;
    uint32_t v55;
    uint32_t v56;
    int v57;
    int v58;

    v6 = (uint32_t *)src;
    v7 = (uint32_t *)dst;
    v8 = height;
    do
    {
        v57 = v8;
        v9 = wid_64;
        do
        {
            v10 = v9;
            v11 = bswap32(*v6);
            v12 = v6 + 1;
            v13 = v11;
            v14 = (8 * (v11 & 0x100000)) | (4 * (v11 & 0x100000)) | (2 * (v11 & 0x100000)) | (v11 & 0x100000) | ((((v11 >> 16) & 0xE00) >> 3) & 0x100) | ((v11 >> 16) & 0xE00) | (8 * ((v11 >> 12) & 0x1000)) | (4 * ((v11 >> 12) & 0x1000)) | (2 * ((v11 >> 12) & 0x1000)) | ((v11 >> 12) & 0x1000) | ((((v11 >> 28) & 0xE) >> 3)) | ((v11 >> 28) & 0xE) | (8 * ((v11 >> 24) & 0x10)) | (4 * ((v11 >> 24) & 0x10)) | (2 * ((v11 >> 24) & 0x10)) | ((v11 >> 24) & 0x10);
            v11 >>= 4;
            v11 &= 0xE0000u;
            v15 = v11 | v14;
            v11 >>= 3;
            *v7 = ((((v13 << 8) & 0xE000000) >> 3) & 0x1000000) | ((v13 << 8) & 0xE000000) | (8 * ((v13 << 12) & 0x10000000)) | (4 * ((v13 << 12) & 0x10000000)) | (2 * ((v13 << 12) & 0x10000000)) | ((v13 << 12) & 0x10000000) | (v11 & 0x10000) | v15;
            v16 = v7 + 1;
            v17 = 16 * (uint16_t)(v13 & 0xFFFF) & 0x1000;
            v18 = (((v13 & 0xE00) >> 3) & 0x100) | (v13 & 0xE00) | (8 * v17) | (4 * v17) | (2 * v17) | (v17) | ((((v13 >> 12) & 0xE) >> 3)) | ((v13 >> 12) & 0xE) | (8 * ((v13 >> 8) & 0x10)) | (4 * ((v13 >> 8) & 0x10)) | (2 * ((v13 >> 8) & 0x10)) | ((v13 >> 8) & 0x10);
            v19 = v13 << 16;
            v20 = (8 * (v19 & 0x100000)) | (4 * (v19 & 0x100000)) | (2 * (v19 & 0x100000)) | (v19 & 0x100000) | v18;
            v21 = v13 << 12;
            v21 &= 0xE0000u;
            v22 = v21 | v20;
            v21 >>= 3;
            *v16 = ((((v13 << 24) & 0xE000000) >> 3) & 0x1000000) | ((v13 << 24) & 0xE000000) | (8 * ((v13 << 28) & 0x10000000)) | (4 * ((v13 << 28) & 0x10000000)) | (2 * ((v13 << 28) & 0x10000000)) | ((v13 << 28) & 0x10000000) | (v21 & 0x10000) | v22;
            ++v16;
            v23 = bswap32(*v12);
            v6 = v12 + 1;
            v24 = v23;
            v25 = (8 * (v23 & 0x100000)) | (4 * (v23 & 0x100000)) | (2 * (v23 & 0x100000)) | (v23 & 0x100000) | ((((v23 >> 16) & 0xE00) >> 3) & 0x100) | ((v23 >> 16) & 0xE00) | (8 * ((v23 >> 12) & 0x1000)) | (4 * ((v23 >> 12) & 0x1000)) | (2 * ((v23 >> 12) & 0x1000)) | ((v23 >> 12) & 0x1000) | (((v23 >> 28) & 0xE) >> 3) | ((v23 >> 28) & 0xE) | (8 * ((v23 >> 24) & 0x10)) | (4 * ((v23 >> 24) & 0x10)) | (2 * ((v23 >> 24) & 0x10)) | ((v23 >> 24) & 0x10);
            v23 >>= 4;
            v23 &= 0xE0000u;
            v26 = v23 | v25;
            v23 >>= 3;
            *v16 = ((((v24 << 8) & 0xE000000) >> 3) & 0x1000000) | ((v24 << 8) & 0xE000000) | (8 * ((v24 << 12) & 0x10000000)) | (4 * ((v24 << 12) & 0x10000000)) | (2 * ((v24 << 12) & 0x10000000)) | ((v24 << 12) & 0x10000000) | (v23 & 0x10000) | (v26);
            ++v16;
            v27 = 16 * (uint16_t)(v24 & 0xFFFF) & 0x1000;
            v28 = (((v24 & 0xE00) >> 3) & 0x100) | (v24 & 0xE00) | (8 * v27) | (4 * v27) | (2 * v27) | (v27) | ((((v24 >> 12) & 0xE) >> 3)) | ((v24 >> 12) & 0xE) | (8 * ((v24 >> 8) & 0x10)) | (4 * ((v24 >> 8) & 0x10)) | (2 * ((v24 >> 8) & 0x10)) | ((v24 >> 8) & 0x10);
            v29 = v24 << 16;
            v30 = (8 * (v29 & 0x100000)) | (4 * (v29 & 0x100000)) | (2 * (v29 & 0x100000)) | (v29 & 0x100000) | v28;
            v31 = v24 << 12;
            v31 &= 0xE0000u;
            v32 = v31 | v30;
            v31 >>= 3;
            *v16 = ((((v24 << 24) & 0xE000000) >> 3) & 0x1000000) | ((v24 << 24) & 0xE000000) | (8 * ((v24 << 28) & 0x10000000)) | (4 * ((v24 << 28) & 0x10000000)) | (2 * ((v24 << 28) & 0x10000000)) | ((v24 << 28) & 0x10000000) | (v31 & 0x10000) | v32;
            v7 = v16 + 1;
            v9 = v10 - 1;
        } while (v10 != 1);
        if (v57 == 1)
            break;
        v58 = v57 - 1;
        v33 = (uint32_t *)((char *)v6 + line);
        v34 = (uint32_t *)((char *)v7 + ext);
        v35 = wid_64;
        do
        {
            v36 = v35;
            v37 = bswap32(v33[1]);
            v38 = v37 >> 4;
            v38 &= 0xE0000u;
            v39 = v38 | (8 * (v37 & 0x100000)) | (4 * (v37 & 0x100000)) | (2 * (v37 & 0x100000)) | (v37 & 0x100000) | ((((v37 >> 16) & 0xE00) >> 3) & 0x100) | ((v37 >> 16) & 0xE00) | (8 * ((v37 >> 12) & 0x1000)) | (4 * ((v37 >> 12) & 0x1000)) | (2 * ((v37 >> 12) & 0x1000)) | ((v37 >> 12) & 0x1000) | (((v37 >> 28) & 0xE) >> 3) | ((v37 >> 28) & 0xE) | (8 * ((v37 >> 24) & 0x10)) | (4 * ((v37 >> 24) & 0x10)) | (2 * ((v37 >> 24) & 0x10)) | ((v37 >> 24) & 0x10);
            v38 >>= 3;
            *v34 = ((((v37 << 8) & 0xE000000) >> 3) & 0x1000000) | ((v37 << 8) & 0xE000000) | (8 * ((v37 << 12) & 0x10000000)) | (4 * ((v37 << 12) & 0x10000000)) | (2 * ((v37 << 12) & 0x10000000)) | ((v37 << 12) & 0x10000000) | (v38 & 0x10000) | v39;
            v40 = v34 + 1;
            v41 = 16 * (uint16_t)(v37 & 0xFFFF) & 0x1000;
            v42 = (((v37 & 0xE00) >> 3) & 0x100) | (v37 & 0xE00) | (8 * v41) | (4 * v41) | (2 * v41) | v41 | (((v37 >> 12) & 0xE) >> 3) | ((v37 >> 12) & 0xE) | (8 * ((v37 >> 8) & 0x10)) | (4 * ((v37 >> 8) & 0x10)) | (2 * ((v37 >> 8) & 0x10)) | ((v37 >> 8) & 0x10);
            v43 = v37 << 16;
            v44 = (8 * (v43 & 0x100000)) | (4 * (v43 & 0x100000)) | (2 * (v43 & 0x100000)) | (v43 & 0x100000) | v42;
            v45 = v37 << 12;
            v45 &= 0xE0000u;
            v46 = v45 | v44;
            v45 >>= 3;
            *v40 = ((((v37 << 24) & 0xE000000) >> 3) & 0x1000000) | ((v37 << 24) & 0xE000000) | (8 * ((v37 << 28) & 0x10000000)) | (4 * ((v37 << 28) & 0x10000000)) | (2 * ((v37 << 28) & 0x10000000)) | ((v37 << 28) & 0x10000000) | (v45 & 0x10000) | v46;
            ++v40;
            v47 = bswap32(*v33);
            v33 += 2;
            v48 = v47;
            v49 = (8 * (v47 & 0x100000)) | (4 * (v47 & 0x100000)) | (2 * (v47 & 0x100000)) | (v47 & 0x100000) | ((((v47 >> 16) & 0xE00) >> 3) & 0x100) | ((v47 >> 16) & 0xE00) | (8 * ((v47 >> 12) & 0x1000)) | (4 * ((v47 >> 12) & 0x1000)) | (2 * ((v47 >> 12) & 0x1000)) | ((v47 >> 12) & 0x1000) | (((v47 >> 28) & 0xE) >> 3) | ((v47 >> 28) & 0xE) | (8 * ((v47 >> 24) & 0x10)) | (4 * ((v47 >> 24) & 0x10)) | (2 * ((v47 >> 24) & 0x10)) | ((v47 >> 24) & 0x10);
            v47 >>= 4;
            v47 &= 0xE0000u;
            v50 = v47 | v49;
            v47 >>= 3;
            *v40 = ((((v48 << 8) & 0xE000000) >> 3) & 0x1000000) | ((v48 << 8) & 0xE000000) | (8 * ((v48 << 12) & 0x10000000)) | (4 * ((v48 << 12) & 0x10000000)) | (2 * ((v48 << 12) & 0x10000000)) | ((v48 << 12) & 0x10000000) | (v47 & 0x10000) | v50;
            ++v40;
            v51 = 16 * (uint16_t)(v48 & 0xFFFF) & 0x1000;
            v52 = (((v48 & 0xE00) >> 3) & 0x100) | (v48 & 0xE00) | (8 * v51) | (4 * v51) | (2 * v51) | v51 | (((v48 >> 12) & 0xE) >> 3) | ((v48 >> 12) & 0xE) | (8 * ((v48 >> 8) & 0x10)) | (4 * ((v48 >> 8) & 0x10)) | (2 * ((v48 >> 8) & 0x10)) | ((v48 >> 8) & 0x10);
            v53 = v48 << 16;
            v54 = (8 * (v53 & 0x100000)) | (4 * (v53 & 0x100000)) | (2 * (v53 & 0x100000)) | (v53 & 0x100000) | v52;
            v55 = v48 << 12;
            v55 &= 0xE0000u;
            v56 = v55 | v54;
            v55 >>= 3;
            *v40 = ((((v48 << 24) & 0xE000000) >> 3) & 0x1000000) | ((v48 << 24) & 0xE000000) | (8 * ((v48 << 28) & 0x10000000)) | (4 * ((v48 << 28) & 0x10000000)) | (2 * ((v48 << 28) & 0x10000000)) | ((v48 << 28) & 0x10000000) | (v55 & 0x10000) | v56;
            v34 = v40 + 1;
            v35 = v36 - 1;
        } while (v36 != 1);
        v6 = (uint32_t *)((char *)v33 + line);
        v7 = (uint32_t *)((char *)v34 + ext);
        v8 = v58 - 1;
    } while (v58 != 1);
}

static inline void load4bI(uint8_t *src, uint8_t *dst, int wid_64, int height, int line, int ext)
{
    uint32_t *v6;
    uint32_t *v7;
    int v8;
    int v9;
    int v10;
    uint32_t v11;
    uint32_t *v12;
    uint32_t v13;
    uint32_t v14;
    uint32_t *v15;
    uint32_t v16;
    unsigned int v17;
    unsigned int v18;
    uint32_t v19;
    uint32_t v20;
    uint32_t *v21;
    uint32_t *v22;
    int v23;
    int v24;
    uint32_t v25;
    uint32_t v26;
    uint32_t *v27;
    uint32_t v28;
    uint32_t v29;
    uint32_t v30;
    uint32_t v31;
    uint32_t v32;
    int v33;
    int v34;

    v6 = (uint32_t *)src;
    v7 = (uint32_t *)dst;
    v8 = height;
    do
    {
        v33 = v8;
        v9 = wid_64;
        do
        {
            v10 = v9;
            v11 = bswap32(*v6);
            v12 = v6 + 1;
            v13 = v11;
            v14 = (16 * ((v11 >> 16) & 0xF00)) | ((v11 >> 16) & 0xF00) | (16 * (v11 >> 28)) | (v11 >> 28);
            v11 >>= 4;
            *v7 = (16 * ((v13 << 8) & 0xF000000)) | ((v13 << 8) & 0xF000000) | (16 * (v11 & 0xF0000)) | (v11 & 0xF0000) | v14;
            v15 = v7 + 1;
            v16 = v13 << 12;
            *v15 = (16 * ((v13 << 24) & 0xF000000)) | ((v13 << 24) & 0xF000000) | (16 * (v16 & 0xF0000)) | (v16 & 0xF0000) | (16 * (v13 & 0xF00)) | (v13 & 0xF00) | (16 * ((uint16_t)(v13 & 0xFFFF) >> 12)) | ((uint16_t)(v13 & 0xFFFF) >> 12);
            ++v15;
            v17 = bswap32(*v12);
            v6 = v12 + 1;
            v18 = v17;
            v19 = (16 * ((v17 >> 16) & 0xF00)) | ((v17 >> 16) & 0xF00) | (16 * (v17 >> 28)) | (v17 >> 28);
            v17 >>= 4;
            *v15 = (16 * ((v18 << 8) & 0xF000000)) | ((v18 << 8) & 0xF000000) | (16 * (v17 & 0xF0000)) | (v17 & 0xF0000) | v19;
            ++v15;
            v20 = v18 << 12;
            *v15 = (16 * ((v18 << 24) & 0xF000000)) | ((v18 << 24) & 0xF000000) | (16 * (v20 & 0xF0000)) | (v20 & 0xF0000) | (16 * (v18 & 0xF00)) | (v18 & 0xF00) | (16 * ((uint16_t)(v18 & 0xFFFF) >> 12)) | ((uint16_t)(v18 & 0xFFFF) >> 12);
            v7 = v15 + 1;
            v9 = v10 - 1;
        } while (v10 != 1);
        if (v33 == 1)
            break;
        v34 = v33 - 1;
        v21 = (uint32_t *)((char *)v6 + line);
        v22 = (uint32_t *)((char *)v7 + ext);
        v23 = wid_64;
        do
        {
            v24 = v23;
            v25 = bswap32(v21[1]);
            v26 = v25 >> 4;
            *v22 = (16 * ((v25 << 8) & 0xF000000)) | ((v25 << 8) & 0xF000000) | (16 * (v26 & 0xF0000)) | (v26 & 0xF0000) | (16 * ((v25 >> 16) & 0xF00)) | ((v25 >> 16) & 0xF00) | (16 * (v25 >> 28)) | (v25 >> 28);
            v27 = v22 + 1;
            v28 = v25 << 12;
            *v27 = (16 * ((v25 << 24) & 0xF000000)) | ((v25 << 24) & 0xF000000) | (16 * (v28 & 0xF0000)) | (v28 & 0xF0000) | (16 * (v25 & 0xF00)) | (v25 & 0xF00) | (16 * ((uint16_t)(v25 & 0xFFFF) >> 12)) | ((uint16_t)(v25 & 0xFFFF) >> 12);
            ++v27;
            v29 = bswap32(*v21);
            v21 += 2;
            v30 = v29;
            v31 = (16 * ((v29 >> 16) & 0xF00)) | ((v29 >> 16) & 0xF00) | (16 * (v29 >> 28)) | (v29 >> 28);
            v29 >>= 4;
            *v27 = (16 * ((v30 << 8) & 0xF000000)) | ((v30 << 8) & 0xF000000) | (16 * (v29 & 0xF0000)) | (v29 & 0xF0000) | v31;
            ++v27;
            v32 = v30 << 12;
            *v27 = (16 * ((v30 << 24) & 0xF000000)) | ((v30 << 24) & 0xF000000) | (16 * (v32 & 0xF0000)) | (v32 & 0xF0000) | (16 * (v30 & 0xF00)) | (v30 & 0xF00) | (16 * ((uint16_t)(v30 & 0xFFFF) >> 12)) | ((uint16_t)(v30 & 0xFFFF) >> 12);
            v22 = v27 + 1;
            v23 = v24 - 1;
        } while (v24 != 1);
        v6 = (uint32_t *)((char *)v21 + line);
        v7 = (uint32_t *)((char *)v22 + ext);
        v8 = v34 - 1;
    } while (v34 != 1);
}

//****************************************************************
// Size: 0, Format: 2

uint32_t Load4bCI(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int tile)
{
    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 4));

    if (rdp.tlut_mode == 0)
    {
        //in tlut DISABLE mode load CI texture as plain intensity texture instead of palette dereference.
        //Thanks to angrylion for the advice
        load4bI((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext);
        return /*(0 << 16) | */GFX_TEXFMT_ALPHA_INTENSITY_44;
    }

    uintptr_t pal = uintptr_t(rdp.pal_8 + (rdp.tiles(tile).palette << 4));
    if (rdp.tlut_mode == 2)
    {
        ext <<= 1;
        load4bCI((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext, (uint16_t *)pal);
        return (1 << 16) | GFX_TEXFMT_ARGB_1555;
    }

    ext <<= 1;
    load4bIAPal((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext, (uint16_t *)pal);
    return (1 << 16) | GFX_TEXFMT_ALPHA_INTENSITY_88;
}

//****************************************************************
// Size: 0, Format: 3
//
// ** BY GUGAMAN **

uint32_t Load4bIA(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int tile)
{
    if (rdp.tlut_mode != 0)
        return Load4bCI(dst, src, wid_64, height, line, real_width, tile);

    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 4));
    load4bIA((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext);
    return /*(0 << 16) | */GFX_TEXFMT_ALPHA_INTENSITY_44;
}

//****************************************************************
// Size: 0, Format: 4

uint32_t Load4bI(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int tile)
{
    if (rdp.tlut_mode != 0)
        return Load4bCI(dst, src, wid_64, height, line, real_width, tile);

    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 4));
    load4bI((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext);

    return /*(0 << 16) | */GFX_TEXFMT_ALPHA_INTENSITY_44;
}

//****************************************************************
// Size: 0, Format: 0

uint32_t Load4bSelect(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int tile)
{
    if (rdp.tlut_mode == 0)
        return Load4bI(dst, src, wid_64, height, line, real_width, tile);

    return Load4bCI(dst, src, wid_64, height, line, real_width, tile);
}
