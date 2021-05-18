// Based on MAME's N64DD driver code by Happy_
#pragma once
#include <stdint.h>

void DiskCommand(void);
void DiskReset(void);
void DiskBMControl(void);
void DiskGapSectorCheck(void);
void DiskBMUpdate(void);
bool DiskBMReadWrite(bool write);
void DiskDMACheck(void);

extern bool dd_write;
extern bool dd_reset_hold;
extern uint32_t dd_track_offset, dd_zone;
extern uint32_t dd_start_block, dd_current;

const uint32_t ddZoneSecSize[16] = { 232, 216, 208, 192, 176, 160, 144, 128,
                                   216, 208, 192, 176, 160, 144, 128, 112 };
const uint32_t ddZoneTrackSize[16] = { 158, 158, 149, 149, 149, 149, 149, 114,
                                     158, 158, 149, 149, 149, 149, 149, 114 };
const uint32_t ddStartOffset[16] =
            { 0x0, 0x5F15E0, 0xB79D00, 0x10801A0, 0x1523720, 0x1963D80, 0x1D414C0, 0x20BBCE0,
            0x23196E0, 0x28A1E00, 0x2DF5DC0, 0x3299340, 0x36D99A0, 0x3AB70E0, 0x3E31900, 0x4149200 };

#define SECTORS_PER_BLOCK   85
#define BLOCKS_PER_TRACK    2
