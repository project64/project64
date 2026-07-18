// Defines the CRandom class
// This class implements the xoshiro512++ pseudo-random number generator
// (Blackman & Vigna). It is the single source of pseudo-randomness used by
// the emulator.

#pragma once
#include <stdint.h>

class CRandom
{
public:
    // Full generator state (512 bits). Exposed so callers (e.g. the sync-core
    // consistency check) can snapshot and compare the complete state.
    struct State
    {
        uint64_t s[8];
        bool operator==(const State & other) const;
        bool operator!=(const State & other) const;
    };

    CRandom();                    // Seed from time(nullptr)
    CRandom(uint32_t seed_value); // Deterministic seed (splitmix64 expansion)

    void seed(uint32_t seed_value); // Reseed deterministically
    uint64_t next();                // xoshiro512++ output

    State get_state() const;
    void set_state(const State & state_value);

protected:
    uint64_t m_state[8];
};
