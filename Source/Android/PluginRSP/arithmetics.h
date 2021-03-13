#pragma once

static inline int16_t clamp_s16(int_fast32_t x)
{
    x = (x < INT16_MIN) ? INT16_MIN : x;
    x = (x > INT16_MAX) ? INT16_MAX : x;

    return (int16_t)x;
}

static inline int32_t vmulf(int16_t x, int16_t y)
{
    return (((int32_t)(x))*((int32_t)(y))+0x4000)>>15;
}
