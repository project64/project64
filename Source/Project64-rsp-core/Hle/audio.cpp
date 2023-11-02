// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include "audio.h"
#include <assert.h>
#include <stdint.h>

#include "arithmetics.h"

const int16_t RESAMPLE_LUT[64 * 4] =
    {
        (int16_t)0x0c39u, (int16_t)0x66adu, (int16_t)0x0d46u, (int16_t)0xffdfu,
        (int16_t)0x0b39u, (int16_t)0x6696u, (int16_t)0x0e5fu, (int16_t)0xffd8u,
        (int16_t)0x0a44u, (int16_t)0x6669u, (int16_t)0x0f83u, (int16_t)0xffd0u,
        (int16_t)0x095au, (int16_t)0x6626u, (int16_t)0x10b4u, (int16_t)0xffc8u,
        (int16_t)0x087du, (int16_t)0x65cdu, (int16_t)0x11f0u, (int16_t)0xffbfu,
        (int16_t)0x07abu, (int16_t)0x655eu, (int16_t)0x1338u, (int16_t)0xffb6u,
        (int16_t)0x06e4u, (int16_t)0x64d9u, (int16_t)0x148cu, (int16_t)0xffacu,
        (int16_t)0x0628u, (int16_t)0x643fu, (int16_t)0x15ebu, (int16_t)0xffa1u,
        (int16_t)0x0577u, (int16_t)0x638fu, (int16_t)0x1756u, (int16_t)0xff96u,
        (int16_t)0x04d1u, (int16_t)0x62cbu, (int16_t)0x18cbu, (int16_t)0xff8au,
        (int16_t)0x0435u, (int16_t)0x61f3u, (int16_t)0x1a4cu, (int16_t)0xff7eu,
        (int16_t)0x03a4u, (int16_t)0x6106u, (int16_t)0x1bd7u, (int16_t)0xff71u,
        (int16_t)0x031cu, (int16_t)0x6007u, (int16_t)0x1d6cu, (int16_t)0xff64u,
        (int16_t)0x029fu, (int16_t)0x5ef5u, (int16_t)0x1f0bu, (int16_t)0xff56u,
        (int16_t)0x022au, (int16_t)0x5dd0u, (int16_t)0x20b3u, (int16_t)0xff48u,
        (int16_t)0x01beu, (int16_t)0x5c9au, (int16_t)0x2264u, (int16_t)0xff3au,
        (int16_t)0x015bu, (int16_t)0x5b53u, (int16_t)0x241eu, (int16_t)0xff2cu,
        (int16_t)0x0101u, (int16_t)0x59fcu, (int16_t)0x25e0u, (int16_t)0xff1eu,
        (int16_t)0x00aeu, (int16_t)0x5896u, (int16_t)0x27a9u, (int16_t)0xff10u,
        (int16_t)0x0063u, (int16_t)0x5720u, (int16_t)0x297au, (int16_t)0xff02u,
        (int16_t)0x001fu, (int16_t)0x559du, (int16_t)0x2b50u, (int16_t)0xfef4u,
        (int16_t)0xffe2u, (int16_t)0x540du, (int16_t)0x2d2cu, (int16_t)0xfee8u,
        (int16_t)0xffacu, (int16_t)0x5270u, (int16_t)0x2f0du, (int16_t)0xfedbu,
        (int16_t)0xff7cu, (int16_t)0x50c7u, (int16_t)0x30f3u, (int16_t)0xfed0u,
        (int16_t)0xff53u, (int16_t)0x4f14u, (int16_t)0x32dcu, (int16_t)0xfec6u,
        (int16_t)0xff2eu, (int16_t)0x4d57u, (int16_t)0x34c8u, (int16_t)0xfebdu,
        (int16_t)0xff0fu, (int16_t)0x4b91u, (int16_t)0x36b6u, (int16_t)0xfeb6u,
        (int16_t)0xfef5u, (int16_t)0x49c2u, (int16_t)0x38a5u, (int16_t)0xfeb0u,
        (int16_t)0xfedfu, (int16_t)0x47edu, (int16_t)0x3a95u, (int16_t)0xfeacu,
        (int16_t)0xfeceu, (int16_t)0x4611u, (int16_t)0x3c85u, (int16_t)0xfeabu,
        (int16_t)0xfec0u, (int16_t)0x4430u, (int16_t)0x3e74u, (int16_t)0xfeacu,
        (int16_t)0xfeb6u, (int16_t)0x424au, (int16_t)0x4060u, (int16_t)0xfeafu,
        (int16_t)0xfeafu, (int16_t)0x4060u, (int16_t)0x424au, (int16_t)0xfeb6u,
        (int16_t)0xfeacu, (int16_t)0x3e74u, (int16_t)0x4430u, (int16_t)0xfec0u,
        (int16_t)0xfeabu, (int16_t)0x3c85u, (int16_t)0x4611u, (int16_t)0xfeceu,
        (int16_t)0xfeacu, (int16_t)0x3a95u, (int16_t)0x47edu, (int16_t)0xfedfu,
        (int16_t)0xfeb0u, (int16_t)0x38a5u, (int16_t)0x49c2u, (int16_t)0xfef5u,
        (int16_t)0xfeb6u, (int16_t)0x36b6u, (int16_t)0x4b91u, (int16_t)0xff0fu,
        (int16_t)0xfebdu, (int16_t)0x34c8u, (int16_t)0x4d57u, (int16_t)0xff2eu,
        (int16_t)0xfec6u, (int16_t)0x32dcu, (int16_t)0x4f14u, (int16_t)0xff53u,
        (int16_t)0xfed0u, (int16_t)0x30f3u, (int16_t)0x50c7u, (int16_t)0xff7cu,
        (int16_t)0xfedbu, (int16_t)0x2f0du, (int16_t)0x5270u, (int16_t)0xffacu,
        (int16_t)0xfee8u, (int16_t)0x2d2cu, (int16_t)0x540du, (int16_t)0xffe2u,
        (int16_t)0xfef4u, (int16_t)0x2b50u, (int16_t)0x559du, (int16_t)0x001fu,
        (int16_t)0xff02u, (int16_t)0x297au, (int16_t)0x5720u, (int16_t)0x0063u,
        (int16_t)0xff10u, (int16_t)0x27a9u, (int16_t)0x5896u, (int16_t)0x00aeu,
        (int16_t)0xff1eu, (int16_t)0x25e0u, (int16_t)0x59fcu, (int16_t)0x0101u,
        (int16_t)0xff2cu, (int16_t)0x241eu, (int16_t)0x5b53u, (int16_t)0x015bu,
        (int16_t)0xff3au, (int16_t)0x2264u, (int16_t)0x5c9au, (int16_t)0x01beu,
        (int16_t)0xff48u, (int16_t)0x20b3u, (int16_t)0x5dd0u, (int16_t)0x022au,
        (int16_t)0xff56u, (int16_t)0x1f0bu, (int16_t)0x5ef5u, (int16_t)0x029fu,
        (int16_t)0xff64u, (int16_t)0x1d6cu, (int16_t)0x6007u, (int16_t)0x031cu,
        (int16_t)0xff71u, (int16_t)0x1bd7u, (int16_t)0x6106u, (int16_t)0x03a4u,
        (int16_t)0xff7eu, (int16_t)0x1a4cu, (int16_t)0x61f3u, (int16_t)0x0435u,
        (int16_t)0xff8au, (int16_t)0x18cbu, (int16_t)0x62cbu, (int16_t)0x04d1u,
        (int16_t)0xff96u, (int16_t)0x1756u, (int16_t)0x638fu, (int16_t)0x0577u,
        (int16_t)0xffa1u, (int16_t)0x15ebu, (int16_t)0x643fu, (int16_t)0x0628u,
        (int16_t)0xffacu, (int16_t)0x148cu, (int16_t)0x64d9u, (int16_t)0x06e4u,
        (int16_t)0xffb6u, (int16_t)0x1338u, (int16_t)0x655eu, (int16_t)0x07abu,
        (int16_t)0xffbfu, (int16_t)0x11f0u, (int16_t)0x65cdu, (int16_t)0x087du,
        (int16_t)0xffc8u, (int16_t)0x10b4u, (int16_t)0x6626u, (int16_t)0x095au,
        (int16_t)0xffd0u, (int16_t)0x0f83u, (int16_t)0x6669u, (int16_t)0x0a44u,
        (int16_t)0xffd8u, (int16_t)0x0e5fu, (int16_t)0x6696u, (int16_t)0x0b39u,
        (int16_t)0xffdfu, (int16_t)0x0d46u, (int16_t)0x66adu, (int16_t)0x0c39u};

int32_t rdot(size_t n, const int16_t * x, const int16_t * y)
{
    int32_t accu = 0;

    y += n;

    while (n != 0)
    {
        accu += *(x++) * *(--y);
        --n;
    }

    return accu;
}

void adpcm_compute_residuals(int16_t * dst, const int16_t * src,
                             const int16_t * cb_entry, const int16_t * last_samples, size_t count)
{
    const int16_t * const book1 = cb_entry;
    const int16_t * const book2 = cb_entry + 8;

    const int16_t l1 = last_samples[0];
    const int16_t l2 = last_samples[1];

    size_t i;

    assert(count <= 8);

    for (i = 0; i < count; ++i)
    {
        int32_t accu = (int32_t)src[i] << 11;
        accu += book1[i] * l1 + book2[i] * l2 + rdot(i, book2, src);
        dst[i] = clamp_s16(accu >> 11);
    }
}
