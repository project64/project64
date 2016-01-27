// MD5.CC - source code for the C++/object oriented translation and
//          modification of MD5.

// Translation and modification (c) 1995 by Mordechai T. Abzug

// This translation/ modification is provided "as is," without express or
// implied warranty of any kind.

// The translator/ modifier does not claim (1) that MD5 will do what you think
// it does; (2) that this translation/ modification is accurate; or (3) that
// this software is "merchantible."  (Language for this disclaimer partially
// copied from the disclaimer below).

/* based on:

MD5.H - header file for MD5C.C
MDDRIVER.C - test driver for MD2, MD4 and MD5

Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

#pragma once

#include <string.h>
#include <stdio.h>
#include <string>
#include <functional>
#include "path.h"

struct MD5Digest
{
    MD5Digest() { Reset(); }
    unsigned char digest[16];

    void Reset() { ::memset(digest, 0, sizeof(digest)); }
    bool IsClear()
    {
        int isClear = 0;
        for (int i = 0; i < 16; i++)
        {
            isClear += digest[i];
        }
        return (isClear == 0);
    }
    std::string String(void)
    {
        char s[33];

        ::memset(s, 0, sizeof(s));

        for (int i = 0; i < 16; i++)
        {
            sprintf(s + i * 2, "%02X", digest[i]);
        }
        s[32] = '\0';

        return s;
    }
};

struct MD5Digest_less : std::binary_function < MD5Digest, MD5Digest, bool >
{
    bool operator()(const MD5Digest& x, const MD5Digest& y) const
    {
        return (memcmp(x.digest, y.digest, sizeof(x.digest)) < 0);
    }
};

class MD5
{
public:
    // methods for controlled operation:
    MD5();  // simple initializer
    ~MD5();
    void  update(const unsigned char *input, unsigned int input_length);
    void  update(FILE *file);
    void  finalize();

    // constructors for special circumstances.  All these constructors finalize
    // the MD5 context.
    MD5(CPath File);            // digest File, finalize
    MD5(const unsigned char *string); // digest string, finalize
    MD5(FILE *file);            // digest file, close, finalize
    MD5(const unsigned char *input, unsigned int input_length);
    MD5(const stdstr & string);

    // methods to acquire finalized result
    void				get_digest(MD5Digest& extdigest); //Digest into a digest structure
    const unsigned char *raw_digest();  // digest as a 16-byte binary array
    const char *      hex_digest();  // digest as a 33-byte ascii-hex string

private:

    // first, some types:
    typedef unsigned       int uint4; // assumes integer is 4 words long
    typedef unsigned short int uint2; // assumes short integer is 2 words long
    typedef unsigned      char uint1; // assumes char is 1 word long

    // next, the private data:
    uint4 state[4];
    uint4 count[2];     // number of *bits*, mod 2^64
    uint1 buffer[64];   // input buffer
    uint1 digest[16];
    uint1 finalized;
    stdstr m_hex_digest;

    // last, the private methods, mostly static:
    void init();               // called by all constructors
    void transform(uint1 *buffer);  // does the real update work.  Note
    // that length is implied to be 64.

    static void encode(uint1 *dest, uint4 *src, uint4 length);
    static void decode(uint4 *dest, uint1 *src, uint4 length);
    static void memcpy(uint1 *dest, uint1 *src, uint4 length);
    static void memset(uint1 *start, uint1 val, uint4 length);

    static inline uint4  rotate_left(uint4 x, uint4 n);
    static inline uint4  F(uint4 x, uint4 y, uint4 z);
    static inline uint4  G(uint4 x, uint4 y, uint4 z);
    static inline uint4  H(uint4 x, uint4 y, uint4 z);
    static inline uint4  I(uint4 x, uint4 y, uint4 z);
    static inline void   FF(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,
        uint4 s, uint4 ac);
    static inline void   GG(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,
        uint4 s, uint4 ac);
    static inline void   HH(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,
        uint4 s, uint4 ac);
    static inline void   II(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,
        uint4 s, uint4 ac);
};
