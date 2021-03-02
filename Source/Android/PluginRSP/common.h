// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// Copyright(C) 2009 Richard Goedeken
// Copyright(C) 2002 Hacktarux
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#pragma once

/* macro for unused variable warning suppression */
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) /* x */
#endif

/* macro for inline keyword */
#ifdef _MSC_VER
#define inline __inline
#endif

/* Dll function linking */
#if defined(_WIN32)
#define EXPORT      extern "C" __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      extern "C" __attribute__((visibility("default")))
#define CALL
#endif

/* Plugin types */
enum
{
    PLUGIN_TYPE_RSP = 1,
    PLUGIN_TYPE_GFX = 2,
    PLUGIN_TYPE_AUDIO = 3,
    PLUGIN_TYPE_CONTROLLER = 4,
};

/***** Structures *****/

typedef struct
{
    uint16_t Version;        /* Should be set to 0x0101 */
    uint16_t Type;           /* Set to PLUGIN_TYPE_RSP */
    char Name[100];      /* Name of the DLL */

    /* If DLL supports memory these memory options then set them to TRUE or FALSE
       if it does not support it */
    int NormalMemory;   /* a normal BYTE array */
    int MemoryBswaped;  /* a normal BYTE array where the memory has been pre
                              bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;
