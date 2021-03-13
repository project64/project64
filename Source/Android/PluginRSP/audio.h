#pragma once

extern const int16_t RESAMPLE_LUT[64 * 4];

int32_t rdot(size_t n, const int16_t *x, const int16_t *y);

static inline int16_t adpcm_predict_sample(uint8_t byte, uint8_t mask,
    unsigned lshift, unsigned rshift)
{
    int16_t sample = (uint16_t)(byte & mask) << lshift;
    sample >>= rshift; // signed
    return sample;
}

void adpcm_compute_residuals(int16_t* dst, const int16_t* src,
    const int16_t* cb_entry, const int16_t* last_samples, size_t count);
