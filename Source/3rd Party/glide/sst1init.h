/*-*-c++-*-*/
/*
** Copyright (c) 1997, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
**
** $Revision: 62 $ 
** $Date: 7/24/98 1:38p $ 
**
*/

#ifndef __SST1INIT_H__
#define __SST1INIT_H__

/*
** Copyright (c) 1996, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
** $Revision: 62 $
** $Date: 7/24/98 1:38p $
**
** SST-1 Initialization routine protypes
**
** If all initialization routines are called, it is assumed they are called
** in the following order:
**   1. sst1InitMapBoard();
**   2. sst1InitRegisters();
**   3. sst1InitGamma();
**   4. sst1InitVideoBuffers();
**   5. sst1InitSli(); [Optional]
**   6. sst1InitCmdFifo();
**
** sst1InitShutdown() is called at the end of an application to turn off
**  the SST-1 graphics subsystem
**
*/

/* sst1init.h assumes "glide.h" and "sst.h" are already included */

/* Init code debug print routine */
#ifdef INIT_DOS /* Glide version... */
#define INIT_OUTPUT
#define INIT_PRINTF(a) sst1InitPrintf a
#define INIT_INFO(A)
#endif

#ifndef DIRECTX
#undef GETENV
#undef ATOI
#undef ATOF
#undef SSCANF
#undef POW
#define GETENV(A) sst1InitGetenv(A)
#define ATOI(A) atoi(A)
#define ATOF(A) atof(A)
#define SSCANF( A, B, C ) sscanf( A, B, C )
#define POW( A, B ) pow( A, B )
#define FTOL( X ) ((FxU32)(X))

// Video resolution declarations
#include "sst1vid.h"

// Info Structure declaration
#include "cvginfo.h"

#else /* DIRECTX */
#include "ddglobal.h"
#pragma optimize ("",off)   /* ddglobal.h tuns this on for retail builds */
#undef INIT_PRINTF
#undef INIT_INFO
#undef GETENV
#undef ATOI
#undef ATOF
#undef FTOL
#undef ITOF_INV
#undef SSCANF
#undef POW
/* #define INIT_PRINTF(a) */
#ifdef FXTRACE
  #define INIT_PRINTF DDPRINTF
#else
  #define INIT_PRINTF 1 ? (void) 0 : (void)
#endif
#define INIT_INFO(A)
#define GETENV(A)  ddgetenv(A)
#define ATOI(A) ddatoi(A)
#define ATOF(A) ddatof(A)
#define FTOL(A) ddftol(A)
#define ITOF_INV(A) dd_itof_inv(A)
#define SSCANF( A, B, C ) ddsscanf( A, B, C )
#define POW( A, B ) ddpow( A, B )

#endif /* DIRECTX */

/* Defines to writing to/reading from SST-1 */
#if 0
#define IGET(A)    A
#define ISET(A,D)  A = (D)
#else
#define IGET(A)    sst1InitRead32((FxU32 *) &(A))
#define ISET(A,D)  sst1InitWrite32((FxU32 *) &(A), D)  
#endif

/*
**  P6 Fence
** 
**  Here's the stuff to do P6 Fencing.  This is required for the
**  certain things on the P6
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
  FxU32 p6FenceVar;
#else
  extern FxU32 p6FenceVar;
#endif

#if defined(__WATCOMC__)
void
p6Fence(void);
#  pragma aux p6Fence = \
      "xchg eax, p6FenceVar" \
      modify [eax];
#  define P6FENCE p6Fence()
#elif defined(__MSC__)
#  define P6FENCE {_asm xchg eax, p6FenceVar}
#elif defined(macintosh) && __POWERPC__ && defined(__MWERKS__)
#  define P6FENCE __eieio()
#else
#  error "P6 Fencing in-line assembler code needs to be added for this compiler"
#endif  

#ifdef __cplusplus
}
#endif

#ifndef _FXPCI_H_
#include <fxpci.h>
#endif
#include <sst1_pci.h>

/*--------------------------------------------------------*/
/* Following defines need to go in "cvgdefs.h" eventually */
#define SST_CMDFIFO_ADDR                   BIT(21) 

/*--------- SST PCI Configuration Command bits --------------*/
#define SST_PCIMEM_ACCESS_EN               BIT(1)

/*------- SST PCI Configuration Register defaults -----------*/
#define SST_PCI_INIT_ENABLE_DEFAULT        0x0
#define SST_PCI_BUS_SNOOP_DEFAULT          0x0

/*--- SST PCI Init Enable Configuration Register defaults ---*/
#define SST_SLI_OWNPCI                     SST_SCANLINE_SLV_OWNPCI
#define SST_SLI_MASTER_OWNPCI              0x0
#define SST_SLI_SLAVE_OWNPCI               SST_SCANLINE_SLV_OWNPCI
#define SST_CHUCK_REVISION_ID_SHIFT        12
#define SST_CHUCK_REVISION_ID              (0xF<<SST_CHUCK_REVISION_ID_SHIFT)
#define SST_CHUCK_MFTG_ID_SHIFT            16
#define SST_CHUCK_MFTG_ID                  (0xF<<SST_CHUCK_MFTG_ID_SHIFT)
#define SST_PCI_INTR_EN                    BIT(20)
#define SST_PCI_INTR_TIMEOUT_EN            BIT(21)
#define SST_SLI_SNOOP_EN                   BIT(23)
#define SST_SLI_SNOOP_MEMBASE_SHIFT        24
#define SST_SLI_SNOOP_MEMBASE              (0xFF<<SST_SLI_SNOOP_MEMBASE_SHIFT)

/*------- SST Silicon Process Monitor Register Defines ------*/
#define SST_SIPROCESS_OSC_CNTR             0xFFFF
#define SST_SIPROCESS_PCI_CNTR_SHIFT       16
#define SST_SIPROCESS_PCI_CNTR             (0xFFF<<SST_SIPROCESS_PCI_CNTR_SHIFT)
#define SST_SIPROCESS_OSC_CNTR_RESET_N     0
#define SST_SIPROCESS_OSC_CNTR_RUN         BIT(28)
#define SST_SIPROCESS_OSC_NAND_SEL         0
#define SST_SIPROCESS_OSC_NOR_SEL          BIT(29)
#define SST_SIPROCESS_OSC_FORCE_ENABLE     BIT(30)

/*----------------- SST fbiinit0 bits -----------------------*/
//#define SST_FBIINIT0_DEFAULT               0x00000410
// Must include SST_EN_TEX_MEMFIFO and SST_EN_LFB_MEMFIFO in FBIINIT0_DEFAULT
// or else texture memory detection will hang on some machines (see bug_3.c)
#define SST_FBIINIT0_DEFAULT               (0x00000410 | SST_EN_TEX_MEMFIFO | \
                                            SST_EN_LFB_MEMFIFO)
#define SST_GRX_RESET                      BIT(1)
#define SST_PCI_FIFO_RESET                 BIT(2)
#define SST_EN_ENDIAN_SWAPPING             BIT(3)

/*----------------- SST fbiinit1 bits -----------------------*/
#define SST_FBIINIT1_DEFAULT               0x00201102
#define SST_VIDEO_TILES_MASK               0x010000F0
#define SST_VIDEO_TILES_IN_X_MSB_SHIFT     24
#define SST_VIDEO_TILES_IN_X_MSB           (1<<SST_VIDEO_TILES_IN_X_MSB_SHIFT)

/*----------------- SST fbiinit2 bits -----------------------*/
#define SST_FBIINIT2_DEFAULT               0x80000040
#define SST_SWAP_ALGORITHM_SHIFT           9
#define SST_SWAP_ALGORITHM                 (0x3<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_VSYNC              (0<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_DACDATA0           (1<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_FIFOSTALL          (2<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_SLISYNC            (3<<SST_SWAP_ALGORITHM_SHIFT)
#define SST_DRAM_REFRESH_16MS              (0x30 << SST_DRAM_REFRESH_CNTR_SHIFT)

/*----------------- SST fbiinit3 bits -----------------------*/
#define SST_TEXMAP_DISABLE                 BIT(6)
#define SST_FBI_MEM_TYPE_SHIFT             8
#define SST_FBI_MEM_TYPE                   (0x7<<SST_FBI_MEM_TYPE_SHIFT)
#define SST_FBI_VGA_PASS_POWERON           BIT(12)
#define SST_FT_CLK_DEL_ADJ_SHIFT           13
#define SST_FT_CLK_DEL_ADJ                 (0xF<<SST_FT_CLK_DEL_ADJ_SHIFT)
#define SST_TF_FIFO_THRESH_SHIFT           17
#define SST_TF_FIFO_THRESH                 (0x1F<<SST_TF_FIFO_THRESH_SHIFT)
#define SST_FBIINIT3_DEFAULT               (0x001E4000|SST_TEXMAP_DISABLE)

/*----------------- SST fbiinit4 bits -----------------------*/
#define SST_FBIINIT4_DEFAULT               0x00000001
#define SST_PCI_RDWS_1                     0x0
#define SST_PCI_RDWS_2                     BIT(0)
#define SST_EN_LFB_RDAHEAD                 BIT(1)
#define SST_MEM_FIFO_LWM_SHIFT             2
#define SST_MEM_FIFO_LWM                   (0x3F<<SST_MEM_FIFO_LWM_SHIFT)
#define SST_MEM_FIFO_ROW_BASE_SHIFT        8
#define SST_MEM_FIFO_ROW_BASE              (0x3FF<<SST_MEM_FIFO_ROW_BASE_SHIFT)
#define SST_MEM_FIFO_ROW_ROLL_SHIFT        18
#define SST_MEM_FIFO_ROW_ROLL              (0x3FF<<SST_MEM_FIFO_ROW_ROLL_SHIFT)

/*----------------- SST fbiinit5 bits -----------------------*/
#define SST_DAC_24BPP_PORT                 BIT(2)
#define SST_GPIO_0                         BIT(3)
#define SST_GPIO_0_DRIVE0                  0x0
#define SST_GPIO_0_DRIVE1                  BIT(3)
#define SST_GPIO_0_SHIFT                   3
#define SST_GPIO_1                         BIT(4)
#define SST_GPIO_1_DRIVE0                  0x0
#define SST_GPIO_1_DRIVE1                  BIT(4)
#define SST_GPIO_1_SHIFT                   4
#define SST_BUFFER_ALLOC_SHIFT             9
#define SST_BUFFER_ALLOC                   (0x3 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_2C0Z       (0x0 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_2C1Z       (0x0 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_3C0Z       (0x1 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_3C1Z       (0x2 << SST_BUFFER_ALLOC_SHIFT)
#define SST_VIDEO_CLK_SLAVE_OE_EN          BIT(11)
#define SST_VID_CLK_2X_OUT_OE_EN           BIT(12)
#define SST_VID_CLK_DAC_DATA16_SEL         BIT(13)
#define SST_SLI_DETECT                     BIT(14)
#define SST_HVRETRACE_SYNC_READS           BIT(15)
#define SST_COLOR_BORDER_RIGHT_EN          BIT(16)
#define SST_COLOR_BORDER_LEFT_EN           BIT(17)
#define SST_COLOR_BORDER_BOTTOM_EN         BIT(18)
#define SST_COLOR_BORDER_TOP_EN            BIT(19)
#define SST_SCAN_DOUBLE_HORIZ              BIT(20)
#define SST_SCAN_DOUBLE_VERT               BIT(21)
#define SST_GAMMA_CORRECT_16BPP_EN         BIT(22)
#define SST_INVERT_HSYNC                   BIT(23)
#define SST_INVERT_VSYNC                   BIT(24)
#define SST_VIDEO_OUT_24BPP_EN             BIT(25)
#define SST_GPIO_1_SEL                     BIT(27)
#define SST_FBIINIT5_DEFAULT \
  (SST_HVRETRACE_SYNC_READS | \
   SST_GAMMA_CORRECT_16BPP_EN | \
   SST_GPIO_1_SEL)
 
/*----------------- SST fbiinit6 bits -----------------------*/
#define SST_SLI_SWAP_VACTIVE_SHIFT         0
#define SST_SLI_SWAP_VACTIVE               (0x7<<SST_SLI_SWAP_VACTIVE_SHIFT)
#define SST_SLI_SWAP_VACTIVE_DRAG_SHIFT    3
#define SST_SLI_SWAP_VACTIVE_DRAG        (0x1F<<SST_SLI_SWAP_VACTIVE_DRAG_SHIFT)
#define SST_SLI_SYNC_MASTER                BIT(8)
#define SST_GPIO_2                         (0x3<<9)
#define SST_GPIO_2_DRIVE0                  (0x2<<9)
#define SST_GPIO_2_DRIVE1                  (0x3<<9)
#define SST_GPIO_2_FLOAT                   (0x1<<9)
#define SST_GPIO_2_SHIFT                   9
#define SST_GPIO_3                         (0x3<<11)
#define SST_GPIO_3_DRIVE0                  (0x2<<11)
#define SST_GPIO_3_DRIVE1                  (0x3<<11)
#define SST_GPIO_3_FLOAT                   (0x1<<11)
#define SST_GPIO_3_SHIFT                   11
#define SST_SLI_SYNCIN                     (0x3<<13)
#define SST_SLI_SYNCIN_DRIVE0              (0x2<<13)
#define SST_SLI_SYNCIN_DRIVE1              (0x3<<13)
#define SST_SLI_SYNCIN_FLOAT               (0x1<<13)
#define SST_SLI_SYNCOUT                    (0x3<<15)
#define SST_SLI_SYNCOUT_DRIVE0             (0x2<<15)
#define SST_SLI_SYNCOUT_DRIVE1             (0x3<<15)
#define SST_SLI_SYNCOUT_FLOAT              (0x1<<15)
#define SST_DAC_RD                         (0x3<<17)
#define SST_DAC_RD_DRIVE0                  (0x2<<17)
#define SST_DAC_RD_DRIVE1                  (0x3<<17)
#define SST_DAC_RD_FLOAT                   (0x1<<17)
#define SST_DAC_WR                         (0x3<<19)
#define SST_DAC_WR_DRIVE0                  (0x2<<19)
#define SST_DAC_WR_DRIVE1                  (0x3<<19)
#define SST_DAC_WR_FLOAT                   (0x1<<19)
#define SST_PCI_FIFO_LWM_RDY_SHIFT         21
#define SST_PCI_FIFO_LWM_RDY               (0x7f<<SST_PCI_FIFO_LWM_RDY_SHIFT)
#define SST_VGA_PASS_N                     (0x3<<28)
#define SST_VGA_PASS_N_DRIVE0              (0x2<<28)
#define SST_VGA_PASS_N_DRIVE1              (0x3<<28)
#define SST_VIDEO_TILES_IN_X_LSB_SHIFT     30
#define SST_VIDEO_TILES_IN_X_LSB           (1<<SST_VIDEO_TILES_IN_X_LSB_SHIFT)
#define SST_FBIINIT6_DEFAULT               0x0

/*----------------- SST fbiinit7 bits -----------------------*/
#define SST_CMDFIFO_EN                     BIT(8)
#define SST_CMDFIFO_STORE_OFFSCREEN        BIT(9)
#define SST_CMDFIFO_DISABLE_HOLES          BIT(10)
#define SST_CMDFIFO_RDFETCH_THRESH_SHIFT   11
#define SST_CMDFIFO_RDFETCH_THRESH    (0x1FUL<<SST_CMDFIFO_RDFETCH_THRESH_SHIFT)
#define SST_CMDFIFO_SYNC_WRITES            BIT(16)
#define SST_CMDFIFO_SYNC_READS             BIT(17)
#define SST_PCI_PACKER_RESET               BIT(18)
#define SST_TMU_CHROMA_REG_WR_EN           BIT(19)
#define SST_CMDFIFO_PCI_TIMEOUT_SHIFT      20
#define SST_CMDFIFO_PCI_TIMEOUT          (0x7FUL<<SST_CMDFIFO_PCI_TIMEOUT_SHIFT)
#define SST_TEXMEMWR_BURST_EN              BIT(27)
#define SST_FBIINIT7_DEFAULT \
  (SST_TEXMEMWR_BURST_EN | SST_TMU_CHROMA_REG_WR_EN)

/*----------------- SST trexInit0 bits -----------------------*/
#define SST_EN_TEX_MEM_REFRESH             BIT(0)
#define SST_TEX_MEM_REFRESH_SHIFT          1
#define SST_TEX_MEM_REFRESH                (0x1FF<<SST_TEX_MEM_REFRESH_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_SHIFT        10
#define SST_TEX_MEM_PAGE_SIZE_8BITS        (0x0<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_9BITS        (0x1<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_10BITS       (0x2<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_SHIFT   12
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT17   (0x0<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT18   (0x1<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_EN_TEX_MEM_SECOND_RAS          BIT(14)
#define SST_TEX_MEM_TYPE_SHIFT             15
#define SST_TEX_MEM_TYPE_EDO               (0x0<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_TYPE_SYNC              (0x1<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_DATA_SIZE_16BIT        0x0
#define SST_TEX_MEM_DATA_SIZE_8BIT         BIT(18)
#define SST_TEX_MEM_DO_EXTRA_CAS           BIT(19)
#define SST_TEX_MEM2                       BIT(20)

#define SST_TREXINIT0_DEFAULT \
  ( (SST_EN_TEX_MEM_REFRESH)  \
  | (0x020 << SST_TEX_MEM_REFRESH_SHIFT) \
  | (SST_TEX_MEM_PAGE_SIZE_9BITS) \
  | (SST_TEX_MEM_SECOND_RAS_BIT_BIT18) \
  | (SST_EN_TEX_MEM_SECOND_RAS) \
  | (SST_TEX_MEM_TYPE_EDO) \
  | (SST_TEX_MEM_DATA_SIZE_16BIT) \
  | (0 & SST_TEX_MEM_DO_EXTRA_CAS) \
  | (0 & SST_TEX_MEM2)  )

#define SST_TREX0INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX1INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX2INIT0_DEFAULT             SST_TREXINIT0_DEFAULT

/*----------------- SST trexInit1 bits -----------------------*/
#define SST_TEX_SCANLINE_INTERLEAVE_MASTER 0x0
#define SST_TEX_SCANLINE_INTERLEAVE_SLAVE  BIT(0)
#define SST_EN_TEX_SCANLINE_INTERLEAVE     BIT(1)
#define SST_TEX_FT_FIFO_SIL_SHIFT          2
#define SST_TEX_FT_FIFO_SIL                (0x1F<<SST_TEX_FT_FIFO_SIL_SHIFT)
#define SST_TEX_TT_FIFO_SIL_SHIFT          7
#define SST_TEX_TT_FIFO_SIL                (0xF<<SST_TEX_TT_FIFO_SIL_SHIFT)
#define SST_TEX_TF_CLK_DEL_ADJ_SHIFT       12
#define SST_TEX_TF_CLK_DEL_ADJ             (0xF<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT)
#define SST_TEX_RG_TTCII_INH               BIT(16)
#define SST_TEX_USE_RG_TTCII_INH           BIT(17)
#define SST_TEX_SEND_CONFIG                BIT(18)
#define SST_TEX_RESET_FIFO                 BIT(19)
#define SST_TEX_RESET_GRX                  BIT(20)
#define SST_TEX_PALETTE_DEL_SHIFT          21
#define SST_TEX_PALETTE_DEL                (0x3<<SST_TEX_PALETTE_DEL_SHIFT)
#define SST_TEX_SEND_CONFIG_SEL_SHIFT      23
#define SST_TEX_SEND_CONFIG_SEL            (0x7<<SST_TEX_SEND_CONFIG_SEL_SHIFT)

/* After things stabilize, the fifo stall inputs levels should be backed off

   from the max. conservative values that are being used now for better
        performance.
   SST_TEX_FT_FIFO_SIL =  ??
   SST_TEX_TT_FIFO_SIL =  ??  (effects multi-trex only)
        */

/* for trex ver. 1 bringup, SST_TEX_PALETTE_DEL should be set to it's max
   (== 3) for <50 MHz bringup */

#define SST_TREXINIT1_DEFAULT \
  ( (SST_TEX_SCANLINE_INTERLEAVE_MASTER) \
  | (0 & SST_EN_TEX_SCANLINE_INTERLEAVE) \
  | (0x8 << SST_TEX_FT_FIFO_SIL_SHIFT) \
  | (0x8 << SST_TEX_TT_FIFO_SIL_SHIFT) \
  | (0xf << SST_TEX_TF_CLK_DEL_ADJ_SHIFT) \
  | (0 & SST_TEX_RG_TTCII_INH) \
  | (0 & SST_TEX_USE_RG_TTCII_INH) \
  | (0 & SST_TEX_SEND_CONFIG) \
  | (0 & SST_TEX_RESET_FIFO) \
  | (0 & SST_TEX_RESET_GRX) \
  | (0 << SST_TEX_PALETTE_DEL_SHIFT) \
  | (0 << SST_TEX_SEND_CONFIG_SEL_SHIFT) )

#define SST_TREX0INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX1INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX2INIT1_DEFAULT              SST_TREXINIT1_DEFAULT

/*----------------- SST clutData bits -----------------------*/
#define SST_CLUTDATA_INDEX_SHIFT           24
#define SST_CLUTDATA_RED_SHIFT             16
#define SST_CLUTDATA_GREEN_SHIFT           8
#define SST_CLUTDATA_BLUE_SHIFT            0

/*----------------- SST video setup shifts ------------------*/
#define SST_VIDEO_HSYNC_OFF_SHIFT          16
#define SST_VIDEO_HSYNC_ON_SHIFT           0
#define SST_VIDEO_VSYNC_OFF_SHIFT          16
#define SST_VIDEO_VSYNC_ON_SHIFT           0
#define SST_VIDEO_HBACKPORCH_SHIFT         0
#define SST_VIDEO_VBACKPORCH_SHIFT         16
#define SST_VIDEO_XDIM_SHIFT               0
#define SST_VIDEO_YDIM_SHIFT               16

/*----------------- SST dacData constants -------------------*/
#define SST_DACREG_WMA                     0x0
#define SST_DACREG_LUT                     0x1
#define SST_DACREG_RMR                     0x2
#define SST_DACREG_RMA                     0x3
#define SST_DACREG_ICS_PLLADDR_WR          0x4 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_RD          0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_DATA        0x5 /* ICS only */
#define SST_DACREG_ICS_CMD                 0x6 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_16BPP     0x50 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_24BPP     0x70 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK0       0x0 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1       0x1 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7       0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1_DEFAULT 0x55 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7_DEFAULT 0x71 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK0       0xa /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1       0xb /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1_DEFAULT 0x79 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_CTRL        0xe /* ICS only */
#define SST_DACREG_ICS_PLLCTRL_CLK1SEL     BIT(4)
#define SST_DACREG_ICS_PLLCTRL_CLK0SEL     BIT(5)
#define SST_DACREG_ICS_PLLCTRL_CLK0FREQ    0x7
#define SST_DACREG_INDEXADDR               SST_DACREG_WMA
#define SST_DACREG_INDEXDATA               SST_DACREG_RMR
#define SST_DACREG_INDEX_RMR               0x0
#define SST_DACREG_INDEX_CR0               0x1
#define SST_DACREG_INDEX_MIR               0x2
#define SST_DACREG_INDEX_MIR_ATT_DEFAULT   0x84   /* AT&T */
#define SST_DACREG_INDEX_MIR_TI_DEFAULT    0x97   /* TI */
#define SST_DACREG_INDEX_DIR               0x3
#define SST_DACREG_INDEX_DIR_ATT_DEFAULT   0x9    /* AT&T */
#define SST_DACREG_INDEX_DIR_TI_DEFAULT    0x9    /* TI */
#define SST_DACREG_INDEX_TST               0x4
#define SST_DACREG_INDEX_CR1               0x5
#define SST_DACREG_INDEX_CC                0x6
#define SST_DACREG_INDEX_AA0               0xff  /* can't access */
#define SST_DACREG_INDEX_AA1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB0               0xff  /* can't access */
#define SST_DACREG_INDEX_AB1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB2               0xff  /* can't access */
#define SST_DACREG_INDEX_AC0               0x48
#define SST_DACREG_INDEX_AC1               0x49
#define SST_DACREG_INDEX_AC2               0x4a
#define SST_DACREG_INDEX_AD0               0x4c
#define SST_DACREG_INDEX_AD1               0x4d
#define SST_DACREG_INDEX_AD2               0x4e
#define SST_DACREG_INDEX_BA0               0xff  /* can't access */
#define SST_DACREG_INDEX_BA1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB0               0xff  /* can't access */
#define SST_DACREG_INDEX_BB1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB2               0xff  /* can't access */
#define SST_DACREG_INDEX_BC0               0xff  /* can't access */
#define SST_DACREG_INDEX_BC1               0xff  /* can't access */
#define SST_DACREG_INDEX_BC2               0xff  /* can't access */
#define SST_DACREG_INDEX_BD0               0x6c
#define SST_DACREG_INDEX_BD1               0x6d
#define SST_DACREG_INDEX_BD2               0x6e

#define SST_DACREG_CR0_INDEXED_ADDRESSING  BIT(0)
#define SST_DACREG_CR0_8BITDAC             BIT(1)
#define SST_DACREG_CR0_SLEEP               BIT(3)
#define SST_DACREG_CR0_COLOR_MODE_SHIFT    4
#define SST_DACREG_CR0_COLOR_MODE         (0xF<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_16BPP   (0x3<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_24BPP   (0x5<<SST_DACREG_CR0_COLOR_MODE_SHIFT)

#define SST_DACREG_CR1_BLANK_PEDASTAL_EN  BIT(4)

#define SST_DACREG_CC_BCLK_SEL_SHIFT       0
#define SST_DACREG_CC_BCLK_SELECT_BD       BIT(3)
#define SST_DACREG_CC_ACLK_SEL_SHIFT       4
#define SST_DACREG_CC_ACLK_SELECT_AD       BIT(7)

#define SST_DACREG_CLKREG_MSHIFT           0
#define SST_DACREG_CLKREG_PSHIFT           6
#define SST_DACREG_CLKREG_NSHIFT           0
#define SST_DACREG_CLKREG_LSHIFT           4
#define SST_DACREG_CLKREG_IBSHIFT          0

#define SST_FBI_DACTYPE_ATT                0
#define SST_FBI_DACTYPE_ICS                1
#define SST_FBI_DACTYPE_TI                 2

/* Definitions for parsing voodoo.ini file */
#define DACRDWR_TYPE_WR                    0
#define DACRDWR_TYPE_RDMODWR               1
#define DACRDWR_TYPE_RDNOCHECK             2
#define DACRDWR_TYPE_RDCHECK               3
#define DACRDWR_TYPE_RDPUSH                4
#define DACRDWR_TYPE_WRMOD_POP             5

/* Other useful defines */
#define PCICFG_WR(ADDR, DATA)                                         \
    n = DATA;                                                         \
    if(pciSetConfigData(ADDR, sst1InitDeviceNumber, &n) == FXFALSE)   \
        return(FXFALSE)
#define PCICFG_RD(ADDR, DATA)                                          \
    if(pciGetConfigData(ADDR, sst1InitDeviceNumber, &DATA) == FXFALSE) \
        return(FXFALSE)
#define DAC_INDEXWRADDR(ADDR)                                         \
    sst1InitDacWr(sstbase, SST_DACREG_INDEXADDR, ADDR)
#define DAC_INDEXWR(DATA)                                             \
    sst1InitDacWr(sstbase, SST_DACREG_INDEXDATA, (DATA))
#define DAC_INDEXRD()                                                 \
    sst1InitDacRd(sstbase, SST_DACREG_INDEXDATA)

/*-----------------------------------------------------------*/

/*
** SST-1 Initialization typedefs
**
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float freq;
    FxU32 clkTiming_M;
    FxU32 clkTiming_P;
    FxU32 clkTiming_N;
    FxU32 clkTiming_L;
    FxU32 clkTiming_IB;
} sst1ClkTimingStruct;

typedef struct {
    unsigned char type;
    unsigned char addr;
    FxU32 data;
    FxU32 mask;
    void *nextRdWr;
} sst1InitDacRdWrStruct;

typedef struct {
    FxU32 width;
    FxU32 height;
    FxU32 refresh;
    FxU32 video16BPP;
    sst1InitDacRdWrStruct *setVideoRdWr;
    void *nextSetVideo;
} sst1InitDacSetVideoStruct;

typedef struct {
    FxU32 frequency;
    sst1InitDacRdWrStruct *setMemClkRdWr;
    void *nextSetMemClk;
} sst1InitDacSetMemClkStruct;

typedef struct {
    FxU32 video16BPP;
    sst1InitDacRdWrStruct *setVideoModeRdWr;
    void *nextSetVideoMode;
} sst1InitDacSetVideoModeStruct;

typedef struct {
    char dacManufacturer[100];
    char dacDevice[100];
    sst1InitDacRdWrStruct *detect;
    sst1InitDacSetVideoStruct *setVideo;
    sst1InitDacSetMemClkStruct *setMemClk;
    sst1InitDacSetVideoModeStruct *setVideoMode;
    void *nextDac;
} sst1InitDacStruct;

#define kMaxEnvVarLen 100
#define kMaxEnvValLen 256
typedef struct {
    char envVariable[kMaxEnvVarLen];
    char envValue[kMaxEnvValLen];
    void *nextVar;
} sst1InitEnvVarStruct;

FX_ENTRY FxU32 * FX_CALL sst1InitMapBoard(FxU32);
FX_ENTRY FxU32 * FX_CALL sst1InitMapBoardDirect(FxU32, FxBool);
FX_ENTRY FxU32 FX_CALL sst1InitNumBoardsInSystem(void);
FX_ENTRY FxBool FX_CALL sst1InitRegisters(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGamma(FxU32 *, double);
FX_ENTRY FxBool FX_CALL sst1InitGammaRGB(FxU32 *, double, double, double);
FX_ENTRY FxBool FX_CALL sst1InitGammaTable(FxU32 *, FxU32, FxU32 *, FxU32 *, FxU32 *);
// Note: sst1InitVideo() is for compatibility with SST-1 only, and should
// not be used for Voodoo2. Use sst1InitVideoBuffers() instead
FX_ENTRY FxBool FX_CALL sst1InitVideo(FxU32 *, GrScreenResolution_t,
  GrScreenRefresh_t, void *);
FX_ENTRY FxBool FX_CALL sst1InitVideoBuffers(FxU32 *, GrScreenResolution_t,
  GrScreenRefresh_t, FxU32, FxU32, sst1VideoTimingStruct *, FxBool);
FX_ENTRY FxBool FX_CALL sst1InitAllocBuffers(FxU32 *, FxU32, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitVideoShutdown(FxU32 *, FxBool);
FX_ENTRY FxBool FX_CALL sst1InitShutdown(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitShutdownSli(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitSli(FxU32 *, FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetDeviceInfo(FxU32 *, sst1DeviceInfoStruct *);
FX_ENTRY FxBool FX_CALL sst1InitVideoBorder(FxU32 *, FxU32, FxU32);

/* Miscellaneous routines */
FX_ENTRY void FX_CALL sst1InitWrite32(FxU32 *, FxU32);
FX_ENTRY FxU32 FX_CALL sst1InitRead32(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdle(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdleWithTimeout(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitIdleNoNOP(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdleFBI(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdleFBINoNOP(FxU32 *);
FX_ENTRY FxU32  FX_CALL sst1InitReturnStatus(FxU32 *);
FX_ENTRY FxU32 FX_CALL sst1InitDacRd(FxU32 *, FxU32);
FX_ENTRY void FX_CALL sst1InitDacWr(FxU32 *, FxU32, FxU32);
FxBool sst1InitExecuteDacRdWr(FxU32 *, sst1InitDacRdWrStruct *);
FX_ENTRY void FX_CALL sst1InitSetResolution(FxU32 *, sst1VideoTimingStruct *,
    FxU32);
FX_ENTRY FxBool FX_CALL sst1InitDacIndexedEnable(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitGrxClk(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitCalcGrxClk(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitDacDetect(FxU32 *);
FxBool sst1InitDacDetectATT(FxU32 *);
FxBool sst1InitDacDetectTI(FxU32 *);
FxBool sst1InitDacDetectICS(FxU32 *);
FxBool sst1InitDacDetectINI(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitSetGrxClk(FxU32 *, sst1ClkTimingStruct *);
FX_ENTRY FxBool FX_CALL sst1InitComputeClkParams(float, sst1ClkTimingStruct *);
FxBool sst1InitComputeClkParamsATT(float, sst1ClkTimingStruct *);
FxBool sst1InitComputeClkParamsTI(float, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkATT(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkICS(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkINI(FxU32 *, sst1ClkTimingStruct *);
FX_ENTRY FxBool FX_CALL sst1InitSetVidClk(FxU32 *, float);
FxBool sst1InitSetVidClkATT(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetVidClkICS(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetVidClkINI(FxU32 *, FxU32, FxU32, FxU32, FxU32);
FxBool sst1InitSetVidMode(FxU32 *, FxU32);
FxBool sst1InitSetVidModeATT(FxU32 *, FxU32);
FxBool sst1InitSetVidModeICS(FxU32 *, FxU32);
FxBool sst1InitSetVidModeINI(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitCheckBoard(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetFbiInfo(FxU32 *, sst1DeviceInfoStruct *);
FX_ENTRY FxBool FX_CALL sst1InitGetTmuInfo(FxU32 *, sst1DeviceInfoStruct *);
FX_ENTRY void FX_CALL sst1InitRenderingRegisters(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetTmuMemory(FxU32 *sstbase,
    sst1DeviceInfoStruct *info, FxU32 tmu, FxU32 *TmuMemorySize);
FX_ENTRY FxBool FX_CALL sst1InitClearSwapPending(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitVgaPassCtrl(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitResetFbi(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitResetTmus(FxU32 *);
FX_ENTRY FxU32 FX_CALL sst1InitSliDetect(FxU32 *);
FX_ENTRY FxU32 FX_CALL sst1InitSliPaired(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitVoodooFile(void);
FX_ENTRY char * FX_CALL sst1InitGetenv(char *);
FX_ENTRY FxU32 * FX_CALL sst1InitGetBaseAddr(FxU32);
FxBool sst1InitFillDeviceInfo(FxU32 *, sst1DeviceInfoStruct *);
void sst1InitIdleLoop(FxU32 *, FxBool);
int sst1InitIdleWithTimeoutLoop(FxU32 *, FxBool, FxU32);
void sst1InitPciFifoIdleLoop(FxU32 *);
void sst1InitClearBoardInfo(void);
FX_ENTRY FxBool FX_CALL sst1InitCaching(FxU32* sstBase, FxBool enableP);
FX_ENTRY void FX_CALL sst1InitPrintInitRegs(FxU32 *);
FX_ENTRY sst1VideoTimingStruct* FX_CALL
sst1InitFindVideoTimingStruct(GrScreenResolution_t, GrScreenRefresh_t);
FX_ENTRY FxU32 FX_CALL sst1InitMeasureSiProcess(FxU32 *, FxU32);

FX_ENTRY FxBool FX_CALL sst1InitCmdFifo(FxU32 *, FxBool, FxU32 *, FxU32 *,
                                        FxU32 *, FxSet32Proc);
FX_ENTRY FxBool FX_CALL sst1InitCmdFifoDirect(FxU32 *, FxU32, FxU32, FxU32,
                                              FxBool, FxBool, FxSet32Proc);
FX_ENTRY FxBool FX_CALL sst1InitLfbLock(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitLfbLockDirect(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitLfbUnlock(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitLfbUnlockDirect(FxU32 *);
FxU32 sst1InitConvertRefreshRate( FxU32 );
FX_ENTRY FxBool FX_CALL sst1InitMonitorDetect(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitCalcTClkDelay(FxU32 *, FxU32, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitSetClkDelays(FxU32 *);
void sst1InitCheckTmuMemConst(FxU32 *, FxU32, FxU32);
void sst1InitDrawRectUsingTris(FxU32 *, FxU32, FxU32, FxU32);

#ifdef __cplusplus
}
#endif

/* Info/Print routines */
#ifdef INIT_OUTPUT

#ifdef __cplusplus
extern "C" {
#endif
FX_ENTRY void FX_CALL sst1InitPrintf(const char *, ...);
#ifdef __cplusplus
}
#endif

#ifndef _FILE_DEFINED
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
FILE *sst1InitMsgFile = stdout;
#else
extern FILE *sst1InitMsgFile;
#endif

#ifdef __cplusplus
}
#endif

#endif

/* Maximum number of SST-1 boards supported in system */
#define SST1INIT_MAX_BOARDS 16

/* Maximum number of read pushes in "voodoo.ini" file */
#define DACRDWR_MAX_PUSH    16

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
  static char headersIdent[] = "@#%Voodoo2 InitHeaders $Revision: 62 $";
  FxBool sst1InitUseVoodooFile = FXFALSE;
  sst1InitEnvVarStruct *envVarsBase = (sst1InitEnvVarStruct *) NULL;
  sst1InitDacStruct *dacStructBase = (sst1InitDacStruct *) NULL;
  sst1InitDacStruct *iniDac = (sst1InitDacStruct *) NULL;
  sst1InitDacSetVideoStruct *iniVideo = (sst1InitDacSetVideoStruct *) NULL;
  sst1InitDacSetMemClkStruct *iniMemClk = (sst1InitDacSetMemClkStruct *) NULL;
  FxU32 iniStack[DACRDWR_MAX_PUSH];
  int iniStackPtr = 0;
  sst1DeviceInfoStruct *sst1CurrentBoard;
  FxU32 sst1InitDeviceNumber;
  sst1DeviceInfoStruct sst1BoardInfo[SST1INIT_MAX_BOARDS];
  FxU32 boardsInSystem;
  FxU32 boardsInSystemReally;
  FxU32 initIdleEnabled = 1;


  const PciRegister SST1_PCI_CFG_SCRATCH   = { 0x50, 4, READ_WRITE };
  const PciRegister SST1_PCI_SIPROCESS     = { 0x54, 4, READ_WRITE };
#else
  extern FxBool sst1InitUseVoodooFile;
  extern sst1InitEnvVarStruct *envVarsBase;
  extern sst1InitDacStruct *dacStructBase;
  extern sst1InitDacStruct *iniDac;
  extern sst1InitDacSetVideoStruct *iniVideo;
  extern sst1InitDacSetMemClkStruct *iniMemClk;
  extern FxU32 iniStack[];
  extern int iniStackPtr;
  extern sst1DeviceInfoStruct *sst1CurrentBoard;
  extern FxU32 sst1InitDeviceNumber;
  extern sst1DeviceInfoStruct sst1BoardInfo[SST1INIT_MAX_BOARDS];
  extern FxU32 boardsInSystem;
  extern FxU32 boardsInSystemReally;
  extern FxU32 initIdleEnabled;

  extern PciRegister SST1_PCI_CFG_SCRATCH;
  extern PciRegister SST1_PCI_SIPROCESS;
#endif /* SST1INIT_ALLOCATE */

#ifdef __3Dfx_PCI_CFG__
/* This is really ugly, but it makes us happy w/ the top of the tree
 * pci library which is happier than Gary's library.  
 */
#define SST1_PCI_INIT_ENABLE PCI_SST1_INIT_ENABLE 
#define SST1_PCI_BUS_SNOOP0  PCI_SST1_BUS_SNOOP_0 
#define SST1_PCI_BUS_SNOOP1  PCI_SST1_BUS_SNOOP_1 
#define SST1_PCI_CFG_STATUS  PCI_SST1_CFG_STATUS 
#else /* !__3Dfx_PCI_CFG__ */
#define SST1_PCI_BUS_SNOOP0  SST1_PCI_BUS_SNOOP_0 
#define SST1_PCI_BUS_SNOOP1  SST1_PCI_BUS_SNOOP_1
#endif /* !__3Dfx_PCI_CFG__ */

#ifdef __cplusplus
}
#endif

#ifdef SST1INIT_VIDEO_ALLOCATE
/* SST1INIT_VIDEO_ALLOCATE is only #defined in video.c
  
   Define useful clock and video timings
   Clocks generated are follows:
     Clock Freq. (MHz) =
       [14.318 * (clkTiming_M+2)] / [(clkTiming_N+2) * (2^clkTiming_P)]
  
   Solving for clkTiming_M yields:
     clkTiming_M =
       [ [(Clock Freq (Mhz)) * (clkTiming_N+2) * (2^clkTiming_P)] / 14.318 ] - 2
     
   NOTE: [14.318 * (clkTiming_M+2)] / (clkTiming_N+2) should be between
    120 and 240
   NOTE: Max. M is 127
   NOTE: Max. N is 31
   NOTE: Max. P is 3
   NOTE: Max. L is 15
   NOTE: Max. IB is 15
*/


/*  H3D video timing structures */
#ifdef H3D
/*  This guy's not used anywhere    */
sst1VideoTimingStruct SST_VREZ_640X502_60 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    15,        /* vBackPorch */
    640,       /* xDimension */
    502,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    160,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};


/* Line doubled 640x480...line doubling done externally */
sst1VideoTimingStruct SST_VREZ_640X960LD_60 = {
    45,        /* hSyncOn */
    785,       /* hSyncOff */
    3,         /* vSyncOn */
    1044,      /* vSyncOff */
    100,       /* hBackPorch */
    18,        /* vBackPorch */
    640,       /* xDimension */
    502,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    160,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    50.82F,    /* clkFreq16bpp */
    101.64F    /* clkFreq24bpp */
};


/* Full resolution 640x480... */
sst1VideoTimingStruct SST_VREZ_640X960_60 = {
    45,        /* hSyncOn */
    785,       /* hSyncOff */
    3,         /* vSyncOn */
    1044,      /* vSyncOff */
    100,       /* hBackPorch */
    18,        /* vBackPorch */
    640,       /* xDimension */
    1004,      /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    320,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    50.82F,    /* clkFreq16bpp */
    101.64F    /* clkFreq24bpp */
};


/* Line doubled 800x600...line doubling done externally */
sst1VideoTimingStruct SST_VREZ_800X1200LD_45 = {
    63,        /* hSyncOn */
    983,       /* hSyncOff */
    3,         /* vSyncOn */
    1242,      /* vSyncOff */
    150,       /* hBackPorch */
    27,        /* vBackPorch */
    800,       /* xDimension */
    608,       /* yDimension */
    42,        /* refreshRate */
    0,         /* miscCtrl */
    247,       /* memOffset */
    26,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.25F,    /* clkFreq16bpp */
    112.5F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_800X630_60 = {
    127,       /* hSyncOn */
    927,       /* hSyncOff */
    4,         /* vSyncOn */
    656,       /* vSyncOff */
    86,        /* hBackPorch */
    23,        /* vBackPorch */
    800,       /* xDimension */
    630,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    260,       /* memOffset */
    26,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    40.0F,     /* clkFreq16bpp */
    80.0F      /* clkFreq24bpp */
};


/* Full res 800x600...so far, ain't nobody got enough memory */
sst1VideoTimingStruct SST_VREZ_800X1200_45 = {
    63,        /* hSyncOn */
    983,       /* hSyncOff */
    3,         /* vSyncOn */
    1244,      /* vSyncOff */
    150,       /* hBackPorch */
    27,        /* vBackPorch */
    800,       /* xDimension */
    1216,      /* yDimension */
    42,        /* refreshRate */
    0,         /* miscCtrl */
    494,       /* memOffset */
    26,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.25F,    /* clkFreq16bpp */
    112.5F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_960X742_60 = {
    103,       /* hSyncOn */
    1151,      /* hSyncOff */
    3,         /* vSyncOn */
    765,       /* vSyncOff */
    142,       /* hBackPorch */
    22,        /* vBackPorch */
    960,       /* xDimension */
    742,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    360,       /* memOffset */
    30,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.219F,   /* clkFreq16bpp */
    112.437F   /* clkFreq24bpp */
};

#endif


sst1VideoTimingStruct SST_VREZ_320X200_70 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    447,       /* vSyncOff */
    48,        /* hBackPorch */
    35,        /* vBackPorch */
    320,       /* xDimension */
    200,       /* yDimension */
    70,        /* refreshRate */
    0x3,       /* miscCtrl */
    35,        /* memOffset */
    10,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X200_75 = {
    99,        /* hSyncOn */
    733,       /* hSyncOff */
    3,         /* vSyncOn */
    429,       /* vSyncOff */
    52,        /* hBackPorch */
    25,        /* vBackPorch */
    320,       /* xDimension */
    200,       /* yDimension */
    75,        /* refreshRate */
    0x3,       /* miscCtrl */
    35,        /* memOffset */
    10,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    27.0F,     /* clkFreq16bpp */
    54.0F      /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X200_85 = {
    63,        /* hSyncOn */
    767,       /* hSyncOff */
    3,         /* vSyncOn */
    442,       /* vSyncOff */
    94,        /* hBackPorch */
    41,        /* vBackPorch */
    320,       /* xDimension */
    200,       /* yDimension */
    85,        /* refreshRate */
    0x3,       /* miscCtrl */
    35,        /* memOffset */
    10,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X200_120 = {
    67,        /* hSyncOn */
    798,       /* hSyncOff */
    3,         /* vSyncOn */
    424,       /* vSyncOff */
    94,        /* hBackPorch */
    16,        /* vBackPorch */
    320,       /* xDimension */
    200,       /* yDimension */
    120,       /* refreshRate */
    0x3,       /* miscCtrl */
    35,        /* memOffset */
    10,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    44.47F,    /* clkFreq16bpp */
    88.94F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X240_60 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    25,        /* vBackPorch */
    320,       /* xDimension */
    240,       /* yDimension */
    60,        /* refreshRate */
    0x3,       /* miscCtrl */
    40,        /* memOffset */
    10,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X240_75 = {
    63,        /* hSyncOn */
    775,       /* hSyncOff */
    3,         /* vSyncOn */
    497,       /* vSyncOff */
    118,       /* hBackPorch */
    16,        /* vBackPorch */
    320,       /* xDimension */
    240,       /* yDimension */
    75,        /* refreshRate */
    0x3,       /* miscCtrl */
    40,        /* memOffset */
    10,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X240_85 = {
    55,        /* hSyncOn */
    776,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    78,        /* hBackPorch */
    25,        /* vBackPorch */
    320,       /* xDimension */
    240,       /* yDimension */
    85,        /* refreshRate */
    0x3,       /* miscCtrl */
    40,        /* memOffset */
    10,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    36.0F,     /* clkFreq16bpp */
    72.0F      /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_320X240_120 = {
    45,        /* hSyncOn */
    785,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    100,       /* hBackPorch */
    18,        /* vBackPorch */
    320,       /* xDimension */
    240,       /* yDimension */
    120,       /* refreshRate */
    0x3,       /* miscCtrl */
    40,        /* memOffset */
    10,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    50.82F,    /* clkFreq16bpp */
    101.64F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_400X300_60 = {
    39,        /* hSyncOn */
    471,       /* hSyncOff */
    3,         /* vSyncOn */
    619,       /* vSyncOff */
    54,        /* hBackPorch */
    18,        /* vBackPorch */
    400,       /* xDimension */
    300,       /* yDimension */
    60,        /* refreshRate */
    0x2,       /* miscCtrl */
    70,        /* memOffset */
    14,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    19.108F,   /* clkFreq16bpp */
    38.216F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_400X300_75 = {
    39,        /* hSyncOn */
    487,       /* hSyncOff */
    3,         /* vSyncOn */
    624,       /* vSyncOff */
    62,        /* hBackPorch */
    23,        /* vBackPorch */
    400,       /* xDimension */
    300,       /* yDimension */
    75,        /* refreshRate */
    0x2,       /* miscCtrl */
    70,        /* memOffset */
    14,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    24.829F,   /* clkFreq16bpp */
    49.658F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_400X300_85 = {
    39,        /* hSyncOn */
    487,       /* hSyncOff */
    3,         /* vSyncOn */
    627,       /* vSyncOff */
    62,        /* hBackPorch */
    26,        /* vBackPorch */
    400,       /* xDimension */
    300,       /* yDimension */
    85,        /* refreshRate */
    0x2,       /* miscCtrl */
    70,        /* memOffset */
    14,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    28.274F,   /* clkFreq16bpp */
    56.548F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_400X300_120 = {
    39,        /* hSyncOn */
    503,       /* hSyncOff */
    3,         /* vSyncOn */
    640,       /* vSyncOff */
    70,        /* hBackPorch */
    39,        /* vBackPorch */
    400,       /* xDimension */
    300,       /* yDimension */
    120,       /* refreshRate */
    0x2,       /* miscCtrl */
    70,        /* memOffset */
    14,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    41.975F,   /* clkFreq16bpp */
    83.950F    /* clkFreq24bpp */
};

/* 512x256@60 only syncs to Arcade-style monitors */
sst1VideoTimingStruct SST_VREZ_512X256_60 = {
    41,        /* hSyncOn */
    626,       /* hSyncOff */
    4,         /* vSyncOn */
    286,       /* vSyncOff */
    65,        /* hBackPorch */
    24,        /* vBackPorch */
    512,       /* xDimension */
    256,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    64,        /* memOffset */
    16,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXFALSE,   /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    23.334F,   /* clkFreq16bpp */
    23.334F    /* clkFreq24bpp */
};

#if 0
// For Arcade monitors...
sst1VideoTimingStruct SST_VREZ_512X384_60 = {
    23,        /* hSyncOn */
    640,       /* hSyncOff */
    3,         /* vSyncOn */
    411,       /* vSyncOff */
    90,        /* hBackPorch */
    24,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXFALSE,   /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    33.0F,     /* clkFreq16bpp */
    33.0F      /* clkFreq24bpp */
};
#else
// For PC monitors...
sst1VideoTimingStruct SST_VREZ_512X384_60 = {
    55,        /* hSyncOn */
    615,       /* hSyncOff */
    3,         /* vSyncOn */
    792,       /* vSyncOff */
    78,        /* hBackPorch */
    23,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    60,        /* refreshRate */
    0x2,       /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    32.054F,   /* clkFreq16bpp */
    64.108F    /* clkFreq24bpp */
};
#endif

sst1VideoTimingStruct SST_VREZ_512X384_72 = {
    51,        /* hSyncOn */
    591,       /* hSyncOff */
    3,         /* vSyncOn */
    430,       /* vSyncOff */
    55,        /* hBackPorch */
    25,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    72,        /* refreshRate */
    0,         /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    20.093F,   /* clkFreq16bpp */
    40.186F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_512X384_75 = {
    55,        /* hSyncOn */
    631,       /* hSyncOff */
    3,         /* vSyncOn */
    799,       /* vSyncOff */
    86,        /* hBackPorch */
    30,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    75,        /* refreshRate */
    0x2,       /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    41.383F,   /* clkFreq16bpp */
    82.766F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_512X384_75_NOSCANDOUBLE = {
    47,        /* hSyncOn */
    591,       /* hSyncOff */
    3,         /* vSyncOn */
    399,       /* vSyncOff */
    62,        /* hBackPorch */
    14,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    19.296F,   /* clkFreq16bpp */
    38.592F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_512X384_85 = {
    55,        /* hSyncOn */
    631,       /* hSyncOff */
    3,         /* vSyncOn */
    804,       /* vSyncOff */
    86,        /* hBackPorch */
    35,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    85,        /* refreshRate */
    0x2,       /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    47.193F,   /* clkFreq16bpp */
    94.386F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_512X384_85_NOSCANDOUBLE = {
    55,        /* hSyncOn */
    599,       /* hSyncOff */
    3,         /* vSyncOn */
    401,       /* vSyncOff */
    70,        /* hBackPorch */
    16,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    22.527F,   /* clkFreq16bpp */
    45.054F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_512X384_120 = {
    25,        /* hSyncOn */
    650,       /* hSyncOff */
    3,         /* vSyncOn */
    409,       /* vSyncOff */
    110,       /* hBackPorch */
    25,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    120,       /* refreshRate */
    0,         /* miscCtrl */
    96,        /* memOffset */
    16,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    33.5F,     /* clkFreq16bpp */
    67.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_70 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    447,       /* vSyncOff */
    48,        /* hBackPorch */
    35,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    70,        /* refreshRate */
    0,         /* miscCtrl */
    130,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_75 = {
    99,        /* hSyncOn */
    733,       /* hSyncOff */
    3,         /* vSyncOn */
    429,       /* vSyncOff */
    52,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    130,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    27.0F,     /* clkFreq16bpp */
    54.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_85 = {
    63,        /* hSyncOn */
    767,       /* hSyncOff */
    3,         /* vSyncOn */
    442,       /* vSyncOff */
    94,        /* hBackPorch */
    41,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    130,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_120 = {
    67,        /* hSyncOn */
    798,       /* hSyncOff */
    3,         /* vSyncOn */
    424,       /* vSyncOff */
    94,        /* hBackPorch */
    16,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    120,       /* refreshRate */
    0,         /* miscCtrl */
    130,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    44.47F,    /* clkFreq16bpp */
    88.94F     /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_60 = {
    96,        /* hSyncOn */

    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    150,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_75 = {
    63,        /* hSyncOn */
    775,       /* hSyncOff */
    3,         /* vSyncOn */
    497,       /* vSyncOff */
    118,       /* hBackPorch */
    16,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    150,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_85 = {
    55,        /* hSyncOn */
    776,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    78,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    150,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    36.0F,     /* clkFreq16bpp */
    72.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_120 = {
    45,        /* hSyncOn */
    785,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    100,       /* hBackPorch */
    18,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    120,       /* refreshRate */
    0,         /* miscCtrl */
    150,       /* memOffset */
    20,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    50.82F,    /* clkFreq16bpp */
    101.64F    /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
// 800x600 requires 832x608 amount of memory usage...
sst1VideoTimingStruct SST_VREZ_800X600_60 = {
    127,       /* hSyncOn */
    927,       /* hSyncOff */
    4,         /* vSyncOn */
    624,       /* vSyncOff */
    86,        /* hBackPorch */
    23,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    247,       /* memOffset */
    26,        /* tilesInX */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    40.0F,     /* clkFreq16bpp */
    80.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_800X600_75 = {
    79,        /* hSyncOn */
    975,       /* hSyncOff */
    3,         /* vSyncOn */
    622,       /* vSyncOff */
    158,       /* hBackPorch */
    21,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    247,       /* memOffset */
    26,        /* tilesInX */
    21,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    49.5F,     /* clkFreq16bpp */
    99.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_800X600_85 = {
    63,        /* hSyncOn */
    983,       /* hSyncOff */
    3,         /* vSyncOn */
    628,       /* vSyncOff */
    150,       /* hBackPorch */
    27,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    247,       /* memOffset */
    26,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.25F,    /* clkFreq16bpp */
    112.5F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_800X600_120 = {
    87,        /* hSyncOn */
    999,       /* hSyncOff */
    3,         /* vSyncOn */
    640,       /* vSyncOff */
    142,       /* hBackPorch */
    39,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    120,       /* refreshRate */
    0,         /* miscCtrl */
    247,       /* memOffset */
    26,        /* tilesInX */
    17,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    83.950F,   /* clkFreq16bpp */
    83.950F    /* clkFreq24bpp -- unsupported */
};

// 856x480 requires 896x480 amount of memory usage...
sst1VideoTimingStruct SST_VREZ_856X480_60 = {
    136,       /* hSyncOn */
    1008,      /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    100,       /* hBackPorch */
    23,        /* vBackPorch */
    856,       /* xDimension */
    480,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    210,       /* memOffset */
    28,        /* tilesInX */
    16,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    36.0F,     /* clkFreq16bpp */
    72.0F      /* clkFreq24bpp */
};

// 960x720 requires 960x736 amount of memory usage...
sst1VideoTimingStruct SST_VREZ_960X720_60 = {
    103,       /* hSyncOn */
    1151,      /* hSyncOff */
    3,         /* vSyncOn */
    743,       /* vSyncOff */
    142,       /* hBackPorch */
    22,        /* vBackPorch */
    960,       /* xDimension */
    720,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    345,       /* memOffset */
    30,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.219F,   /* clkFreq16bpp */
    112.437F   /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_960X720_75 = {
    103,       /* hSyncOn */
    1183,      /* hSyncOff */
    3,         /* vSyncOn */
    749,       /* vSyncOff */
    158,       /* hBackPorch */
    28,        /* vBackPorch */
    960,       /* xDimension */
    720,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    345,       /* memOffset */
    30,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    72.643F,   /* clkFreq16bpp */
    72.643F    /* clkFreq24bpp -- unsupported */
};

sst1VideoTimingStruct SST_VREZ_960X720_85 = {
    103,       /* hSyncOn */
    1199,      /* hSyncOff */
    3,         /* vSyncOn */
    753,       /* vSyncOff */
    166,       /* hBackPorch */
    32,        /* vBackPorch */
    960,       /* xDimension */
    720,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    345,       /* memOffset */
    30,        /* tilesInX */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    83.795F,   /* clkFreq16bpp */
    83.795F    /* clkFreq24bpp -- unsupported */
};

sst1VideoTimingStruct SST_VREZ_1024X768_60 = {
    136,       /* hSyncOn */
    1208,      /* hSyncOff */
    6,         /* vSyncOn */
    800,       /* vSyncOff */
    160,       /* hBackPorch */
    29,        /* vBackPorch */
    1024,      /* xDimension */
    768,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    384,       /* memOffset */
    32,        /* tilesInX */
    16,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    65.0F,     /* clkFreq16bpp */
    130.0F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_1024X768_75 = {
    96,        /* hSyncOn */
    1216,      /* hSyncOff */
    3,         /* vSyncOn */
    797,       /* vSyncOff */
    176,       /* hBackPorch */
    28,        /* vBackPorch */
    1024,      /* xDimension */
    768,       /* yDimension */
    75,        /* refreshRate */
    0,         /* miscCtrl */
    384,       /* memOffset */
    32,        /* tilesInX */
    16,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    78.75F,    /* clkFreq16bpp */
    78.75F     /* clkFreq24bpp -- unsupported */
};

sst1VideoTimingStruct SST_VREZ_1024X768_85 = {
    96,        /* hSyncOn */
    1280,      /* hSyncOff */
    3,         /* vSyncOn */
    805,       /* vSyncOff */
    208,       /* hBackPorch */
    36,        /* vBackPorch */
    1024,      /* xDimension */
    768,       /* yDimension */
    85,        /* refreshRate */
    0,         /* miscCtrl */
    384,       /* memOffset */
    32,        /* tilesInX */
    16,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXFALSE,   /* video24BPPIsOK */
    94.5F,     /* clkFreq16bpp */
    94.5F      /* clkFreq24bpp -- unsupported */
};

#else /* SST1INIT_VIDEO_ALLOCATE */


#ifdef __cplusplus
extern "C" {
#endif

extern sst1VideoTimingStruct SST_VREZ_640X480_60;
extern sst1VideoTimingStruct SST_VREZ_800X600_60;

#ifdef __cplusplus
}
#endif

#endif /* SST1INIT_VIDEO_ALLOCATE */

#endif /* !__SST1INIT_H__ */
