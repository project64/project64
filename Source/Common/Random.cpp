// Implements the CRandom class
// This class implements the xoshiro512++ pseudo-random number generator
// (Blackman & Vigna). The 32-bit seed is expanded to the 512-bit state with
// splitmix64, which is the seeding scheme recommended by the xoshiro authors
// and guarantees a non-zero state (xoshiro requires the state to be non-zero).

#include "Random.h"
#include <time.h>

static inline uint64_t rotl(const uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

// splitmix64 - used purely to derive the xoshiro512++ state from a 32-bit seed.
static inline uint64_t splitmix64(uint64_t & x)
{
    uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

bool CRandom::State::operator==(const State & other) const
{
    for (int i = 0; i < 8; i++)
    {
        if (s[i] != other.s[i])
        {
            return false;
        }
    }
    return true;
}

bool CRandom::State::operator!=(const State & other) const
{
    return !(*this == other);
}

CRandom::CRandom()
{
    seed((uint32_t)time(nullptr));
}

CRandom::CRandom(uint32_t seed_value)
{
    seed(seed_value);
}

void CRandom::seed(uint32_t seed_value)
{
    uint64_t sm = seed_value;
    for (int i = 0; i < 8; i++)
    {
        m_state[i] = splitmix64(sm);
    }
}

uint64_t CRandom::next()
{
    uint64_t * s = m_state;
    const uint64_t result = rotl(s[0] + s[2], 17) + s[2];

    const uint64_t t = s[1] << 11;

    s[2] ^= s[0];
    s[5] ^= s[1];
    s[1] ^= s[2];
    s[7] ^= s[3];
    s[3] ^= s[4];
    s[4] ^= s[5];
    s[0] ^= s[6];
    s[6] ^= s[7];

    s[6] ^= t;

    s[7] = rotl(s[7], 21);

    return result;
}

CRandom::State CRandom::get_state() const
{
    State state;
    for (int i = 0; i < 8; i++)
    {
        state.s[i] = m_state[i];
    }
    return state;
}

void CRandom::set_state(const State & state_value)
{
    for (int i = 0; i < 8; i++)
    {
        m_state[i] = state_value.s[i];
    }
}
