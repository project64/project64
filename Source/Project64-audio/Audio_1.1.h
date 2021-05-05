// Common audio plugin spec, version 1.1

/*
Notes:
Setting the appropriate bits in the MI_INTR_REG and calling CheckInterrupts which
are both passed to the DLL in InitiateAudio will generate an Interrupt from with in
the plugin.
*/

#pragma once

#include <stdint.h>

enum { PLUGIN_TYPE_AUDIO = 3 };

#if defined(_WIN32)
#define EXPORT      extern "C" __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      extern "C" __attribute__((visibility("default")))
#define CALL
#endif

enum
{
    SYSTEM_NTSC = 0,
    SYSTEM_PAL = 1,
    SYSTEM_MPAL = 2,
};

enum
{
    AI_STATUS_FIFO_FULL = 0x80000000,	// Bit 31: full
    AI_STATUS_DMA_BUSY = 0x40000000,	// Bit 30: busy

    MI_INTR_AI = 0x04,		// Bit 2: AI INTR
    AI_CONTROL_DMA_ON = 0x01,
    AI_CONTROL_DMA_OFF = 0x00,
};

// Structures

typedef struct
{
    uint16_t Version;        // Should be set to 0x0101
    uint16_t Type;           // Set to PLUGIN_TYPE_AUDIO
    char Name[100];          // Name of the DLL
    int32_t NormalMemory;
    int32_t MemoryBswaped;
} PLUGIN_INFO;

typedef struct
{
    void * hwnd;
    void * hinst;

    int32_t MemoryBswaped;    // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary
    //	eg. the first 8 bytes are stored like this:
    //  4 3 2 1   8 7 6 5
    uint8_t * HEADER;	// This is the ROM header (first 40h bytes of the ROM)
    // This will be in the same memory format as the rest of the memory.
    uint8_t * RDRAM;
    uint8_t * DMEM;
    uint8_t * IMEM;

    uint32_t * MI_INTR_REG;

    uint32_t * AI_DRAM_ADDR_REG;
    uint32_t * AI_LEN_REG;
    uint32_t * AI_CONTROL_REG;
    uint32_t * AI_STATUS_REG;
    uint32_t * AI_DACRATE_REG;
    uint32_t * AI_BITRATE_REG;

    void(CALL *CheckInterrupts)(void);
} AUDIO_INFO;

/*
Function: AiDacrateChanged
Purpose: This function is called to notify the DLL that the
AiDacrate registers value has been changed.
Input: The system type:
SYSTEM_NTSC	0
SYSTEM_PAL	1
SYSTEM_MPAL	2
Output: None
*/

EXPORT void CALL AiDacrateChanged(int32_t SystemType);

/*
Function: AiLenChanged
Purpose: This function is called to notify the DLL that the
AiLen registers value has been changed.
Input: None
Output: None
*/

EXPORT void CALL AiLenChanged(void);

/*
Function: AiReadLength
Purpose: This function is called to allow the DLL to return the
value that AI_LEN_REG should equal
Input: None
Output: The amount of bytes still left to play.
*/

EXPORT uint32_t CALL AiReadLength(void);

/*
Function: AiUpdate
Purpose: This function is called to allow the DLL to update
things on a regular basis (check how long to sound to
go, copy more stuff to the buffer, anything you like).
The function is designed to go in to the message loop
of the main window...but can be placed anywhere you
like.
Input: If wait is set to true, then this function should wait
till there is a message in its message queue.
Output: None
*/

EXPORT void CALL AiUpdate(int32_t Wait);

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input: None
Output: None
*/

EXPORT void CALL CloseDLL(void);

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

EXPORT void CALL DllAbout(void * hParent);

/*
Function: DllConfig
Purpose: This function is optional function that is provided
to allow the user to configure the DLL
Input: A handle to the window that calls this function
Output: None
*/

EXPORT void CALL DllConfig(void * hParent);

/*
Function: DllTest
Purpose: This function is optional function that is provided
to allow the user to test the DLL
Input: A handle to the window that calls this function
Output: None
*/

EXPORT void CALL DllTest(void * hParent);

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input: A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output: None
*/

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo);

/*
Function: InitiateSound
Purpose:  This function is called when the DLL is started to give
information from the emulator that the N64 audio
interface needs
Input: Audio_Info is passed to this function which is defined
above.
Output: True on success
FALSE on failure to initialize
Note on interrupts:
To generate an interrupt set the appropriate bit in MI_INTR_REG
and then call the function CheckInterrupts to tell the emulator
that there is a waiting interrupt.
*/

EXPORT int32_t CALL InitiateAudio(AUDIO_INFO Audio_Info);

/*
Function: ProcessAList
Purpose: This function is called when there is a Alist to be
processed. The DLL will have to work out all the info
about the AList itself.
Input: None
Output: None
*/

EXPORT void CALL ProcessAList(void);

/*
Function: RomClosed
Purpose: This function is called when a ROM is closed.
Input: None
Output: None
*/

EXPORT void CALL RomClosed(void);
