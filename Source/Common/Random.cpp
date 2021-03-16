// Implements the CRandom class
// This class implements the Lehmer Random Number Generator

#include "stdafx.h"
#include "Random.h"
#include <time.h>

CRandom::CRandom()
{
    m_state = (uint32_t)time(NULL);
}

CRandom::CRandom(uint32_t state_value)
{
    m_state = state_value;
}

uint32_t CRandom::randomizer(uint32_t val)
{
    return ((uint64_t)val * 279470273UL) % 4294967291UL;
}

uint32_t CRandom::next()
{
    m_state = randomizer(m_state);
    return m_state;
}

void CRandom::set_state(uint32_t state_value)
{
    if (state_value == 0)
        m_state = 1;
    else
        m_state = state_value;
}

uint32_t CRandom::get_state()
{
    return m_state;
}
