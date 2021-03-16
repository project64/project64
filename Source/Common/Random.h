// Defines the CRandom class
// This class implements the Lehmer Random Number Generator

#pragma once
#include <Common/stdtypes.h>

class CRandom
{
public:
    CRandom();
    CRandom(uint32_t seed_value);
    uint32_t next();
    uint32_t get_state();
    void set_state(uint32_t state_value);

protected:
    uint32_t randomizer(uint32_t val);
    uint32_t m_state;
};
