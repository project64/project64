#pragma once

class CHle;

// cic_x105 microcode

void cicx105_ucode(CHle * hle);

// Audio list microcodes

enum { N_SEGMENTS = 16 };

struct alist_audio_t
{
    // Segments
    uint32_t segments[N_SEGMENTS];

    // Main buffers
    uint16_t in;
    uint16_t out;
    uint16_t count;

    // Auxiliary buffers
    uint16_t dry_right;
    uint16_t wet_left;
    uint16_t wet_right;

    // Gains
    int16_t dry;
    int16_t wet;

    // Envelopes (0:left, 1:right)
    int16_t vol[2];
    int16_t target[2];
    int32_t rate[2];

    // ADPCM loop point address
    uint32_t loop;

    // Storage for ADPCM table and polef coefficients
    int16_t table[16 * 8];
};

void alist_process_audio(CHle * hle);
void alist_process_audio_ge(CHle * hle);
void alist_process_audio_bc(CHle * hle);

// Audio list microcodes - naudio

struct alist_naudio_t
{
    // Gains
    int16_t dry;
    int16_t wet;

    // Envelopes (0:left, 1:right)
    int16_t vol[2];
    int16_t target[2];
    int32_t rate[2];

    // ADPCM loop point address
    uint32_t loop;

    // Storage for ADPCM table and polef coefficients
    int16_t table[16 * 8];
};

void alist_process_naudio(CHle * hle);
void alist_process_naudio_bk(CHle * hle);
void alist_process_naudio_dk(CHle * hle);
void alist_process_naudio_mp3(CHle * hle);
void alist_process_naudio_cbfd(CHle * hle);

// Audio list microcodes - nead

struct alist_nead_t
{
    // Main buffers
    uint16_t in;
    uint16_t out;
    uint16_t count;

    // Envmixer ramps
    uint16_t env_values[3];
    uint16_t env_steps[3];

    // ADPCM loop point address
    uint32_t loop;

    // Storage for ADPCM table and polef coefficients
    int16_t table[16 * 8];

    // Filter audio command state
    uint16_t filter_count;
    uint32_t filter_lut_address[2];
};

void alist_process_nead_mk(CHle * hle);
void alist_process_nead_sfj(CHle * hle);
void alist_process_nead_sf(CHle * hle);
void alist_process_nead_fz(CHle * hle);
void alist_process_nead_wrjb(CHle * hle);
void alist_process_nead_ys(CHle * hle);
void alist_process_nead_1080(CHle * hle);
void alist_process_nead_oot(CHle * hle);
void alist_process_nead_mm(CHle * hle);
void alist_process_nead_mmb(CHle * hle);
void alist_process_nead_ac(CHle * hle);

// MP3 microcode
void mp3_task(CHle * hle, unsigned int index, uint32_t address);

// Musyx microcodes
void musyx_v1_task(CHle * hle);
void musyx_v2_task(CHle * hle);

// JPEG microcodes
void jpeg_decode_PS0(CHle * hle);
void jpeg_decode_PS(CHle * hle);
void jpeg_decode_OB(CHle * hle);
