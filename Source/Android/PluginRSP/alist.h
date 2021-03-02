// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#pragma once
class CHle;

typedef void(*acmd_callback_t)(CHle* hle, uint32_t w1, uint32_t w2);

void alist_process(CHle * hle, const acmd_callback_t abi[], unsigned int abi_size);
uint32_t alist_get_address(CHle * hle, uint32_t so, const uint32_t *segments, size_t n);
void alist_set_address(CHle * hle, uint32_t so, uint32_t *segments, size_t n);
void alist_clear(CHle * hle, uint16_t dmem, uint16_t count);
void alist_load(CHle * hle, uint16_t dmem, uint32_t address, uint16_t count);
void alist_save(CHle * hle, uint16_t dmem, uint32_t address, uint16_t count);
void alist_move(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t count);
void alist_copy_every_other_sample(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t count);
void alist_repeat64(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint8_t count);
void alist_copy_blocks(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t block_size, uint8_t count);
void alist_interleave(CHle * hle, uint16_t dmemo, uint16_t left, uint16_t right, uint16_t count);
void alist_envmix_exp( CHle * hle, bool init, bool aux, uint16_t dmem_dl, uint16_t dmem_dr, uint16_t dmem_wl, uint16_t dmem_wr, uint16_t dmemi, uint16_t count, int16_t dry, int16_t wet, const int16_t *vol, const int16_t *target, const int32_t *rate, uint32_t address);
void alist_envmix_ge( CHle * hle, bool init, bool aux, uint16_t dmem_dl, uint16_t dmem_dr, uint16_t dmem_wl, uint16_t dmem_wr, uint16_t dmemi, uint16_t count, int16_t dry, int16_t wet, const int16_t *vol, const int16_t *target, const int32_t *rate, uint32_t address);
void alist_envmix_lin( CHle * hle, bool init, uint16_t dmem_dl, uint16_t dmem_dr, uint16_t dmem_wl, uint16_t dmem_wr, uint16_t dmemi, uint16_t count, int16_t dry, int16_t wet, const int16_t *vol, const int16_t *target, const int32_t *rate, uint32_t address);
void alist_envmix_nead( CHle * hle, bool swap_wet_LR, uint16_t dmem_dl, uint16_t dmem_dr, uint16_t dmem_wl, uint16_t dmem_wr, uint16_t dmemi, unsigned count, uint16_t *env_values, uint16_t *env_steps, const int16_t *xors);
void alist_mix(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t count, int16_t gain);
void alist_multQ44(CHle * hle, uint16_t dmem, uint16_t count, int8_t gain);
void alist_add(CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t count);
void alist_adpcm( CHle * hle, bool init, bool loop, bool two_bit_per_sample, uint16_t dmemo, uint16_t dmemi, uint16_t count, const int16_t* codebook, uint32_t loop_address, uint32_t last_frame_address);
void alist_resample( CHle * hle, bool init, bool flag2, uint16_t dmemo, uint16_t dmemi, uint16_t count, uint32_t pitch, uint32_t address);
void alist_polef( CHle * hle, bool init,uint16_t dmemo, uint16_t dmemi, uint16_t count, uint16_t gain, int16_t* table, uint32_t address);
void alist_iirf( CHle * hle, bool init, uint16_t dmemo, uint16_t dmemi, uint16_t count, int16_t* table, uint32_t address);
void alist_resample_zoh( CHle * hle, uint16_t dmemo, uint16_t dmemi, uint16_t count, uint32_t pitch, uint32_t pitch_accu);
void alist_filter( CHle * hle, uint16_t dmem, uint16_t count, uint32_t address, const uint32_t* lut_address);

/*
* Audio flags
*/

#define A_INIT          0x01
#define A_CONTINUE      0x00
#define A_LOOP          0x02
#define A_OUT           0x02
#define A_LEFT          0x02
#define A_RIGHT         0x00
#define A_VOL           0x04
#define A_RATE          0x00
#define A_AUX           0x08
#define A_NOAUX         0x00
#define A_MAIN          0x00
#define A_MIX           0x10
