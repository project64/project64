// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
//
// Shared pseudo-random source for the video plugin. Uses the same xoshiro512++
// generator (CRandom) as the emulator core, so all of the emulator's
// randomness comes from a single generator implementation.

#pragma once
#include <Common/Random.h>
#include <stdint.h>

// The video plugin's generator instance (defined in Main.cpp). It is seeded
// from the stipple-pattern setting, mirroring the previous srand() behaviour.
extern CRandom g_VideoRandom;

// Drop-in replacement for the old libc rand(): returns a non-negative int in
// the 31-bit range, keeping the existing noise/stipple math unchanged.
inline int VideoRand()
{
    return (int)(g_VideoRandom.next() & 0x7FFFFFFF);
}
