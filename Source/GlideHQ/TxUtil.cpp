/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "TxUtil.h"
#include "TxDbg.h"
#include <zlib/zlib.h>
#include <malloc.h>
#include <stdlib.h>
#include <common/stdtypes.h>


/*
 * External libraries
 ******************************************************************************/
TxLoadLib::TxLoadLib()
{
#ifdef DXTN_DLL
  if (!_dxtnlib)
    _dxtnlib = LoadLibrary("dxtn");

  if (_dxtnlib) {
    if (!_tx_compress_dxtn)
      _tx_compress_dxtn = (dxtCompressTexFuncExt)DLSYM(_dxtnlib, "tx_compress_dxtn");

    if (!_tx_compress_fxt1)
      _tx_compress_fxt1 = (fxtCompressTexFuncExt)DLSYM(_dxtnlib, "fxt1_encode");
  }
#else
  _tx_compress_dxtn = tx_compress_dxtn;
  _tx_compress_fxt1 = fxt1_encode;

#endif
}

TxLoadLib::~TxLoadLib()
{
#ifdef DXTN_DLL
  /* free dynamic library */
  if (_dxtnlib)
    FreeLibrary(_dxtnlib);
#endif

}

fxtCompressTexFuncExt
TxLoadLib::getfxtCompressTexFuncExt()
{
  return _tx_compress_fxt1;
}

dxtCompressTexFuncExt
TxLoadLib::getdxtCompressTexFuncExt()
{
  return _tx_compress_dxtn;
}


/*
 * Utilities
 ******************************************************************************/
uint32
TxUtil::checksumTx(uint8 *src, int width, int height, uint16 format)
{
  int dataSize = sizeofTx(width, height, format);

  /* for now we use adler32 if something else is better
   * we can simply swtich later
   */
  /* return (dataSize ? Adler32(src, dataSize, 1) : 0); */

  /* zlib crc32 */
  return (dataSize ? crc32(crc32(0L, Z_NULL, 0), src, dataSize) : 0);
}

int
TxUtil::sizeofTx(int width, int height, uint16 format)
{
  int dataSize = 0;

  /* a lookup table for the shifts would be better */
  switch (format) {
  case GR_TEXFMT_ARGB_CMP_FXT1:
    dataSize = (((width + 0x7) & ~0x7) * ((height + 0x3) & ~0x3)) >> 1;
    break;
  case GR_TEXFMT_ARGB_CMP_DXT1:
    dataSize = (((width + 0x3) & ~0x3) * ((height + 0x3) & ~0x3)) >> 1;
    break;
  case GR_TEXFMT_ARGB_CMP_DXT3:
  case GR_TEXFMT_ARGB_CMP_DXT5:
    dataSize = ((width + 0x3) & ~0x3) * ((height + 0x3) & ~0x3);
    break;
  case GR_TEXFMT_ALPHA_INTENSITY_44:
  case GR_TEXFMT_ALPHA_8:
  case GR_TEXFMT_INTENSITY_8:
  case GR_TEXFMT_P_8:
    dataSize = width * height;
    break;
  case GR_TEXFMT_ARGB_4444:
  case GR_TEXFMT_ARGB_1555:
  case GR_TEXFMT_RGB_565:
  case GR_TEXFMT_ALPHA_INTENSITY_88:
    dataSize = (width * height) << 1;
    break;
  case GR_TEXFMT_ARGB_8888:
    dataSize = (width * height) << 2;
    break;
  default:
    /* unsupported format */
    DBG_INFO(80, L"Error: cannot get size. unsupported gfmt:%x\n", format);
    ;
  }

  return dataSize;
}

#if 0 /* unused */
uint32
TxUtil::chkAlpha(uint32* src, int width, int height)
{
  /* NOTE: _src must be ARGB8888
   * return values
   * 0x00000000: 8bit alpha
   * 0x00000001: 1bit alpha
   * 0xff000001: no alpha
   */

  int _size = width * height;
  uint32 alpha = 0;

  __asm {
    mov esi, dword ptr [src];
    mov ecx, dword ptr [_size];
    mov ebx, 0xff000000;

  tc1_loop:
    mov eax, dword ptr [esi];
    add esi, 4;

    and eax, 0xff000000;
    jz  alpha1bit;
    cmp eax, 0xff000000;
    je  alpha1bit;
    jmp done;

  alpha1bit:
    and ebx, eax;
    dec ecx;
    jnz tc1_loop;

    or  ebx, 0x00000001;
    mov dword ptr [alpha], ebx;

  done:
  }

  return alpha;
}
#endif

uint32
TxUtil::checksum(uint8 *src, int width, int height, int size, int rowStride)
{
  /* Rice CRC32 for now. We can switch this to Jabo MD5 or
   * any other custom checksum.
   * TODO: use *_HIRESTEXTURE option. */

  if (!src) return 0;

  return RiceCRC32(src, width, height, size, rowStride);
}

uint64
TxUtil::checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette)
{
  /* Rice CRC32 for now. We can switch this to Jabo MD5 or
   * any other custom checksum.
   * TODO: use *_HIRESTEXTURE option. */
  /* Returned value is 64bits: hi=palette crc32 low=texture crc32 */

  if (!src) return 0;

  uint64 crc64Ret = 0;

  if (palette) {
    uint32 crc32 = 0, cimax = 0;
    switch (size & 0xff) {
    case 1:
      if (RiceCRC32_CI8(src, width, height, size, rowStride, &crc32, &cimax)) {
        crc64Ret = (uint64)RiceCRC32(palette, cimax + 1, 1, 2, 512);
        crc64Ret <<= 32;
        crc64Ret |= (uint64)crc32;
      }
      break;
    case 0:
      if (RiceCRC32_CI4(src, width, height, size, rowStride, &crc32, &cimax)) {
        crc64Ret = (uint64)RiceCRC32(palette, cimax + 1, 1, 2, 32);
        crc64Ret <<= 32;
        crc64Ret |= (uint64)crc32;
      }
    }
  }
  if (!crc64Ret) {
    crc64Ret = (uint64)RiceCRC32(src, width, height, size, rowStride);
  }

  return crc64Ret;
}

/*
** Computes Adler32 checksum for a stream of data.
**
** From the specification found in RFC 1950: (ZLIB Compressed Data Format
** Specification version 3.3)
**
** ADLER32 (Adler-32 checksum) This contains a checksum value of the
** uncompressed data (excluding any dictionary data) computed according to
** Adler-32 algorithm. This algorithm is a 32-bit extension and improvement
** of the Fletcher algorithm, used in the ITU-T X.224 / ISO 8073 standard.
**
** Adler-32 is composed of two sums accumulated per byte: s1 is the sum of
** all bytes, s2 is the sum of all s1 values. Both sums are done modulo
** 65521. s1 is initialized to 1, s2 to zero. The Adler-32 checksum is stored
** as s2*65536 + s1 in most-significant-byte first (network) order.
**
** 8.2. The Adler-32 algorithm 
**
** The Adler-32 algorithm is much faster than the CRC32 algorithm yet still
** provides an extremely low probability of undetected errors.
**
** The modulo on unsigned long accumulators can be delayed for 5552 bytes,
** so the modulo operation time is negligible. If the bytes are a, b, c,
** the second sum is 3a + 2b + c + 3, and so is position and order sensitive,
** unlike the first sum, which is just a checksum. That 65521 is prime is
** important to avoid a possible large class of two-byte errors that leave
** the check unchanged. (The Fletcher checksum uses 255, which is not prime
** and which also makes the Fletcher check insensitive to single byte
** changes 0 <-> 255.)
**
** The sum s1 is initialized to 1 instead of zero to make the length of
** the sequence part of s2, so that the length does not have to be checked
** separately. (Any sequence of zeroes has a Fletcher checksum of zero.)
*/

uint32
TxUtil::Adler32(const uint8* data, int Len, uint32 dwAdler32)
{
#if 1
  /* zlib adler32 */
  return adler32(dwAdler32, data, Len);
#else
  register uint32 s1 = dwAdler32 & 0xFFFF;
  register uint32 s2 = (dwAdler32 >> 16) & 0xFFFF;
  int k;

  while (Len > 0) {
    /* 5552 is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
    k = (Len < 5552 ? Len : 5552);
    Len -= k;
    while (k--) {
      s1 += *data++;
      s2 += s1;
    }
    /* 65521 is the largest prime smaller than 65536 */
    s1 %= 65521;
    s2 %= 65521;
  }

  return (s2 << 16) | s1;
#endif
}

uint32
TxUtil::Adler32(const uint8* src, int width, int height, int size, int rowStride)
{
  int i;
  uint32 ret = 1;
  uint32 width_in_bytes = width * size;

  for (i = 0; i < height; i++) {
    ret = Adler32(src, width_in_bytes, ret);
    src += rowStride;
  }

  return ret;
}

/* Rice CRC32 for hires texture packs */
/* NOTE: The following is used in Glide64 to calculate the CRC32
 * for Rice hires texture packs.
 *
 * BYTE* addr = (BYTE*)(gfx.RDRAM +
 *                     rdp.addr[rdp.tiles[tile].t_mem] +
 *                     (rdp.tiles[tile].ul_t * bpl) +
 *                     (((rdp.tiles[tile].ul_s<<rdp.tiles[tile].size)+1)>>1));
 * RiceCRC32(addr,
 *          rdp.tiles[tile].width,
 *          rdp.tiles[tile].height,
 *          (unsigned short)(rdp.tiles[tile].format << 8 | rdp.tiles[tile].size),
 *          bpl);
 */
uint32
TxUtil::RiceCRC32(const uint8* src, int width, int height, int size, int rowStride)
{
  /* NOTE: bytes_per_width must be equal or larger than 4 */

  uint32 crc32Ret = 0;
  const uint32 bytes_per_width = ((width << size) + 1) >> 1;

  /*if (bytes_per_width < 4) return 0;*/

  try {
#ifdef WIN32
#ifdef _M_IX86
	  __asm {
      push ebx;
      push esi;
      push edi;

      mov ecx, dword ptr [src];
      mov eax, dword ptr [height];
      mov edx, 0;
      dec eax;

    loop2:
      mov ebx, dword ptr [bytes_per_width];
      sub ebx, 4;

    loop1:
      mov esi, dword ptr [ecx+ebx];
      xor esi, ebx;
      rol edx, 4;
      add edx, esi;
      sub ebx, 4;
      jge loop1;

      xor esi, eax;
      add edx, esi;
      add ecx, dword ptr [rowStride];
      dec eax;
      jge loop2;

      mov dword ptr [crc32Ret], edx;

      pop edi;
      pop esi;
      pop ebx;
    }
#else
	  DebugBreak();
#endif
#else
    asm volatile(
      "pushl %%ebx \n"
      "pushl %%esi \n"
      "pushl %%edi \n"

      "movl %0, %%ecx \n"
      "movl %1, %%eax \n"
      "movl $0, %%edx \n"
      "decl %%eax \n"

      "0: \n"
      "movl %2, %%ebx \n"
      "subl $4, %%ebx \n"

      "1: \n"
      "movl (%%ecx,%%ebx), %%esi \n"
      "xorl %%ebx, %%esi \n"
      "roll $4, %%edx \n"
      "addl %%esi, %%edx \n"
      "subl $4, %%ebx \n"
      "jge  1b \n"

      "xorl %%eax, %%esi \n"
      "addl %%esi, %%edx \n"
      "addl %3, %%ecx \n"
      "decl %%eax \n"
      "jge  0b \n"

      "movl %%edx, %4 \n"

      "popl %%edi \n"
      "popl %%esi \n"
      "popl %%ebx \n"
      :
      : "m"(src), "m"(height), "m"(bytes_per_width), "m"(rowStride), "m"(crc32Ret)
      : "memory", "cc"
      );
#endif
  } catch(...) {
    DBG_INFO(80, L"Error: RiceCRC32 exception!\n");
  }

  return crc32Ret;
}

boolean
TxUtil::RiceCRC32_CI4(const uint8* src, int width, int height, int size, int rowStride,
                        uint32* crc32, uint32* cimax)
{
  /* NOTE: bytes_per_width must be equal or larger than 4 */

  uint32 crc32Ret = 0;
  uint32 cimaxRet = 0;
  const uint32 bytes_per_width = ((width << size) + 1) >> 1;

  /*if (bytes_per_width < 4) return 0;*/

  /* 4bit CI */
  try {
#ifdef WIN32
#ifdef _M_IX86
	  __asm {
      push ebx;
      push esi;
      push edi;

      mov ecx, dword ptr [src];
      mov eax, dword ptr [height];
      mov edx, 0;
      mov edi, 0;
      dec eax;

    loop2:
      mov ebx, dword ptr [bytes_per_width];
      sub ebx, 4;

    loop1:
      mov esi, dword ptr [ecx+ebx];

      cmp edi, 0x0000000f;
      je findmax0;

      push ecx;
      mov ecx, esi;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax8;
      mov edi, ecx;

    findmax8:
      mov ecx, esi;
      shr ecx, 4;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax7;
      mov edi, ecx;

    findmax7:
      mov ecx, esi;
      shr ecx, 8;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax6;
      mov edi, ecx;

    findmax6:
      mov ecx, esi;
      shr ecx, 12;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax5;
      mov edi, ecx;

    findmax5:
      mov ecx, esi;
      shr ecx, 16;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax4;
      mov edi, ecx;

    findmax4:
      mov ecx, esi;
      shr ecx, 20;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax3;
      mov edi, ecx;

    findmax3:
      mov ecx, esi;
      shr ecx, 24;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax2;
      mov edi, ecx;

    findmax2:
      mov ecx, esi;
      shr ecx, 28;
      and ecx, 0x0000000f;
      cmp ecx, edi;
      jb  findmax1;
      mov edi, ecx;

    findmax1:
      pop ecx;

    findmax0:
      xor esi, ebx;
      rol edx, 4;
      add edx, esi;
      sub ebx, 4;
      jge loop1;

      xor esi, eax;
      add edx, esi;
      add ecx, dword ptr [rowStride];
      dec eax;
      jge loop2;

      mov dword ptr [crc32Ret], edx;
      mov dword ptr [cimaxRet], edi;

      pop edi;
      pop esi;
      pop ebx;
    }
#else
DebugBreak();
#endif
#else
    asm volatile(
      "pushl %%ebx \n"
      "pushl %%esi \n"
      "pushl %%edi \n"

      "movl %0, %%ecx \n"
      "movl %1, %%eax \n"
      "movl $0, %%edx \n"
      "movl $0, %%edi \n"
      "decl %%eax \n"

      "0: \n"
      "movl %2, %%ebx \n"
      "subl $4, %%ebx \n"

      "1: \n"
      "movl (%%ecx,%%ebx), %%esi \n"

      "cmpl $0x0000000f, %%edi \n"
      "je  10f \n"

      "pushl %%ecx \n"
      "movl %%esi, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   2f \n"
      "movl %%ecx, %%edi \n"

      "2: \n"
      "movl %%esi, %%ecx \n"
      "shrl $4, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   3f \n"
      "movl %%ecx, %%edi \n"

      "3: \n"
      "movl %%esi, %%ecx \n"
      "shrl $8, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   4f \n"
      "movl %%ecx, %%edi \n"

      "4: \n"
      "movl %%esi, %%ecx \n"
      "shrl $12, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   5f \n"
      "movl %%ecx, %%edi \n"

      "5: \n"
      "movl %%esi, %%ecx \n"
      "shrl $16, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   6f \n"
      "movl %%ecx, %%edi \n"

      "6: \n"
      "movl %%esi, %%ecx \n"
      "shrl $20, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   7f \n"
      "movl %%ecx, %%edi \n"

      "7: \n"
      "movl %%esi, %%ecx \n"
      "shrl $24, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   8f \n"
      "movl %%ecx, %%edi \n"

      "8: \n"
      "movl %%esi, %%ecx \n"
      "shrl $28, %%ecx \n"
      "andl $0x0000000f, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   9f \n"
      "movl %%ecx, %%edi \n"

      "9: \n"
      "popl %%ecx \n"

      "10: \n"
      "xorl %%ebx, %%esi \n"
      "roll $4, %%edx \n"
      "addl %%esi, %%edx \n"
      "subl $4, %%ebx \n"
      "jge  1b \n"

      "xorl %%eax, %%esi \n"
      "addl %%esi, %%edx \n"
      "addl %3, %%ecx \n"
      "decl %%eax \n"
      "jge  0b \n"

      "movl %%edx, %4 \n"
      "movl %%edi, %5 \n"

      "popl %%edi \n"
      "popl %%esi \n"
      "popl %%ebx \n"
      :
      : "m"(src), "m"(height), "m"(bytes_per_width), "m"(rowStride), "m"(crc32Ret), "m"(cimaxRet)
      : "memory", "cc"
      );
#endif
  } catch(...) {
    DBG_INFO(80, L"Error: RiceCRC32 exception!\n");
  }

  *crc32 = crc32Ret;
  *cimax = cimaxRet;

  return 1;
}

boolean
TxUtil::RiceCRC32_CI8(const uint8* src, int width, int height, int size, int rowStride,
                      uint32* crc32, uint32* cimax)
{
  /* NOTE: bytes_per_width must be equal or larger than 4 */

  uint32 crc32Ret = 0;
  uint32 cimaxRet = 0;
  const uint32 bytes_per_width = ((width << size) + 1) >> 1;

  /*if (bytes_per_width < 4) return 0;*/

  /* 8bit CI */
  try {
#ifdef _M_IX86
#ifdef WIN32
	  __asm {
      push ebx;
      push esi;
      push edi;

      mov ecx, dword ptr [src];
      mov eax, dword ptr [height];
      mov edx, 0;
      mov edi, 0;
      dec eax;

    loop2:
      mov ebx, dword ptr [bytes_per_width];
      sub ebx, 4;

    loop1:
      mov esi, dword ptr [ecx+ebx];

      cmp edi, 0x000000ff;
      je findmax0;

      push ecx;
      mov ecx, esi;
      and ecx, 0x000000ff;
      cmp ecx, edi;
      jb  findmax4;
      mov edi, ecx;

    findmax4:
      mov ecx, esi;
      shr ecx, 8;
      and ecx, 0x000000ff;
      cmp ecx, edi;
      jb  findmax3;
      mov edi, ecx;

    findmax3:
      mov ecx, esi;
      shr ecx, 16;
      and ecx, 0x000000ff;
      cmp ecx, edi;
      jb  findmax2;
      mov edi, ecx;

    findmax2:
      mov ecx, esi;
      shr ecx, 24;
      and ecx, 0x000000ff;
      cmp ecx, edi;
      jb  findmax1;
      mov edi, ecx;

    findmax1:
      pop ecx;

    findmax0:
      xor esi, ebx;
      rol edx, 4;
      add edx, esi;
      sub ebx, 4;
      jge loop1;

      xor esi, eax;
      add edx, esi;
      add ecx, dword ptr [rowStride];
      dec eax;
      jge loop2;

      mov dword ptr [crc32Ret], edx;
      mov dword ptr [cimaxRet], edi;

      pop edi;
      pop esi;
      pop ebx;
    }
#else
    asm volatile(
      "pushl %%ebx \n"
      "pushl %%esi \n"
      "pushl %%edi \n"

      "movl %0, %%ecx \n"
      "movl %1, %%eax \n"
      "movl $0, %%edx \n"
      "movl $0, %%edi \n"
      "decl %%eax \n"

      "0: \n"
      "movl %2, %%ebx \n"
      "subl $4, %%ebx \n"

      "1: \n"
      "movl (%%ecx,%%ebx), %%esi \n"

      "cmpl $0x000000ff, %%edi \n"
      "je   6f \n"

      "pushl %%ecx \n"
      "movl %%esi, %%ecx \n"
      "andl $0x000000ff, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   2f \n"
      "movl %%ecx, %%edi \n"

      "2: \n"
      "movl %%esi, %%ecx \n"
      "shrl $8, %%ecx \n"
      "andl $0x000000ff, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   3f \n"
      "movl %%ecx, %%edi \n"

      "3: \n"
      "movl %%esi, %%ecx \n"
      "shrl $16, %%ecx \n"
      "andl $0x000000ff, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   4f \n"
      "movl %%ecx, %%edi \n"

      "4: \n"
      "movl %%esi, %%ecx \n"
      "shrl $24, %%ecx \n"
      "andl $0x000000ff, %%ecx \n"
      "cmpl %%edi, %%ecx \n"
      "jb   5f \n"
      "movl %%ecx, %%edi \n"

      "5: \n"
      "popl %%ecx \n"

      "6: \n"
      "xorl %%ebx, %%esi \n"
      "roll $4, %%edx \n"
      "addl %%esi, %%edx \n"
      "subl $4, %%ebx \n"
      "jge  1b \n"

      "xorl %%eax, %%esi \n"
      "addl %%esi, %%edx \n"
      "addl %3, %%ecx \n"
      "decl %%eax \n"
      "jge  0b \n"

      "movl %%edx, %4 \n"
      "movl %%edi, %5 \n"

      "popl %%edi \n"
      "popl %%esi \n"
      "popl %%ebx \n"
      :
      : "m"(src), "m"(height), "m"(bytes_per_width), "m"(rowStride), "m"(crc32Ret), "m"(cimaxRet)
      : "memory", "cc"
      );
#endif
#else
DebugBreak();
#endif
  }
  catch (...) {
    DBG_INFO(80, L"Error: RiceCRC32 exception!\n");
  }

  *crc32 = crc32Ret;
  *cimax = cimaxRet;

  return 1;
}

int
TxUtil::log2(int num)
{
#if defined(__GNUC__)
  return __builtin_ctz(num);
#elif defined(_MSC_VER) && _MSC_VER >= 1400
  uint32_t i;
  _BitScanForward((DWORD *)&i, num);
  return i;
#elif defined(__MSC__)
  __asm {
    mov eax, dword ptr [num];
    bsr eax, eax;
    mov dword ptr [i], eax;
  }
#else
  switch (num) {
    case 1:    return 0;
    case 2:    return 1;
    case 4:    return 2;
    case 8:    return 3;
    case 16:   return 4;
    case 32:   return 5;
    case 64:   return 6;
    case 128:  return 7;
    case 256:  return 8;
    case 512:  return 9;
    case 1024:  return 10;
    case 2048:  return 11;
  }
#endif
}

int
TxUtil::grLodLog2(int w, int h)
{
  return (w >= h ? log2(w) : log2(h));
}

int
TxUtil::grAspectRatioLog2(int w, int h)
{
  return (w >= h ? log2(w/h) : -log2(h/w));
}

int
TxUtil::getNumberofProcessors()
{
  int numcore = 1;

  /* number of logical processors per physical processor */
  try {
#ifdef WIN32
#if 1
    /* use win32 api */
    SYSTEM_INFO siSysInfo;
    ZeroMemory(&siSysInfo, sizeof(SYSTEM_INFO));
    GetSystemInfo(&siSysInfo);
    numcore = siSysInfo.dwNumberOfProcessors;
#else
    __asm {
      push ebx;

      mov eax, 1;
      cpuid;
      test edx, 0x10000000; /* check HTT */
      jz uniproc;
      and ebx, 0x00ff0000;  /* mask logical core counter bit */
      shr ebx, 16;
      mov dword ptr [numcore], ebx;
    uniproc:

      pop ebx;
    }
#endif
#else
    asm volatile(
      "pushl %%ebx \n"

      "movl $1, %%eax \n"
      "cpuid \n"
      "testl $0x10000000, %%edx \n"
      "jz 0f \n"
      "andl $0x00ff0000, %%ebx \n"
      "shrl $16, %%ebx \n"
      "movl %%ebx, %0 \n"
      "0: \n"

      "popl %%ebx \n"
      :
      : "m"(numcore)
      : "memory", "cc"
      );
#endif
  } catch(...) {
    DBG_INFO(80, L"Error: number of processor detection failed!\n");
  }

  if (numcore > MAX_NUMCORE) numcore = MAX_NUMCORE;

  DBG_INFO(80, L"Number of processors : %d\n", numcore);

  return numcore;
}


/*
 * Memory buffers for texture manipulations
 ******************************************************************************/
TxMemBuf::TxMemBuf()
{
  int i;
  for (i = 0; i < 2; i++) {
    _tex[i] = NULL;
    _size[i] = 0;
  }
}

TxMemBuf::~TxMemBuf()
{
  shutdown();
}

boolean
TxMemBuf::init(int maxwidth, int maxheight)
{
  int i;
  for (i = 0; i < 2; i++) {
    if (!_tex[i]) {
      _tex[i] = (uint8 *)malloc(maxwidth * maxheight * 4);
      _size[i] = maxwidth * maxheight * 4;
    }

    if (!_tex[i]) {
      shutdown();
      return 0;
    }
  }
  return 1;
}

void
TxMemBuf::shutdown()
{
  int i;
  for (i = 0; i < 2; i++) {
    if (_tex[i]) free(_tex[i]);
    _tex[i] = NULL;
    _size[i] = 0;
  }
}

uint8*
TxMemBuf::get(unsigned int num)
{
  return ((num < 2) ? _tex[num] : NULL);
}

uint32
TxMemBuf::size_of(unsigned int num)
{
  return ((num < 2) ? _size[num] : 0);
}
