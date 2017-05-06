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
#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned int CRCTable[256];

unsigned int Reflect(unsigned int ref, char ch)
{
    unsigned int value = 0;

    // Swap bit 0 for bit 7
    // bit 1 for bit 6, etc.
    for (char i = 1; i < (ch + 1); i++)
    {
        if (ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

void CRC_BuildTable()
{
    unsigned int crc;

    for (unsigned i = 0; i <= 255; i++)
    {
        crc = Reflect(i, 8) << 24;
        for (unsigned j = 0; j < 8; j++)
            crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);

        CRCTable[i] = Reflect(crc, 32);
    }
}
//*/
//*
unsigned int CRC32(unsigned int crc, void *buffer, unsigned int count)
{
    unsigned int orig = crc;
    unsigned char * p = reinterpret_cast<unsigned char*>(buffer);
    while (count--)
        crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
    return crc ^ orig;
}
//*/

/*
uint32_t CRC_Calculate( uint32_t crc, void *buffer, uint32_t count )
{
uint32_t Crc32=crc;
__asm {
mov	esi, buffer
mov	ecx, count
mov	edx, crc
xor eax, eax

loop1:
inc	esi
mov	al, dl
xor	al, byte ptr [esi]
shr	edx, 8
mov	ebx, [CRCTable+eax*4]
xor	edx, ebx

loop loop1

xor	Crc32, edx
}
return Crc32;
}
*/

/*
unsigned int CRC_Calculate( unsigned int crc, void *buffer, unsigned int count )
{
unsigned int Crc32=crc;
__asm {
mov	esi, buffer
mov	edx, count
add	edx, esi
mov	ecx, crc

loop1:
mov	bl, byte ptr [esi]
movzx	eax, cl
inc	esi
xor	al, bl
shr	ecx, 8
mov	ebx, [CRCTable+eax*4]
xor	ecx, ebx

cmp	edx, esi
jne	loop1

xor	Crc32, ecx
}
return Crc32;
}
//*/