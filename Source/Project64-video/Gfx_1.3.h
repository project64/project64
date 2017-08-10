/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#pragma once

#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <stddef.h>		// offsetof
#include <Common/MemTest.h>
#include "rdp.h"
#include "Config.h"
#include "Settings.h"

#if defined __VISUALC__
#define GLIDE64_TRY __try
#define GLIDE64_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
#else
#define GLIDE64_TRY try
#define GLIDE64_CATCH catch (...)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

    //********
    // Logging

    // ********************************
    // ** TAKE OUT BEFORE RELEASE!!! **
    //#define LOG_UCODE

    //  note that some of these things are inserted/removed
    //  from within the code & may not be changed by this define.

    // ********************************

    //#define CATCH_EXCEPTIONS	// catch exceptions so it doesn't freeze and will report
    // "The gfx plugin has caused an exception" instead.

    //#define SHOW_FULL_TEXVIEWER	// shows the entire contents of the texture in the cache viewer,
    // usually used to debug clamping issues.

    // Usually enabled
#define LARGE_TEXTURE_HANDLING	// allow large-textured objects to be split?

    // Simulations
    //#define SIMULATE_VOODOO1
    //#define SIMULATE_BANSHEE
    //********

    extern unsigned int BMASK;
#define WMASK	0x3FFFFF
#define DMASK	0x1FFFFF

    extern uint32_t update_screen_count;

    extern int GfxInitDone;
    extern bool g_romopen;
    extern int to_fullscreen;

    extern int evoodoo;
    extern int ev_fullscreen;

    extern int exception;

    int InitGfx();
    void ReleaseGfx();

    // The highest 8 bits are the segment # (1-16), and the lower 24 bits are the offset to
    // add to it.
    __inline uint32_t segoffset(uint32_t so)
    {
        return (rdp.segment[(so >> 24) & 0x0f] + (so&BMASK))&BMASK;
    }

    /* Plugin types */
#define PLUGIN_TYPE_GFX				2

#ifdef _WIN32
#define EXPORT      extern "C" __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      __attribute__((visibility("default")))
#define CALL
#endif

    /***** Structures *****/
    typedef struct
    {
        uint16_t Version;        /* Set to 0x0103 */
        uint16_t Type;           /* Set to PLUGIN_TYPE_GFX */
        char Name[100];      /* Name of the DLL */

        /* If DLL supports memory these memory options then set them to TRUE or FALSE
        if it does not support it */
        int NormalMemory;    /* a normal uint8_t array */
        int MemoryBswaped;  /* a normal uint8_t array where the memory has been pre
                            bswap on a dword (32 bits) boundry */
    } PLUGIN_INFO;

    typedef struct
    {
        void * hWnd;			/* Render window */
        void * hStatusBar;    /* if render window does not have a status bar then this is NULL */

        int32_t MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
        //   bswap on a dword (32 bits) boundry
        //	eg. the first 8 bytes are stored like this:
        //        4 3 2 1   8 7 6 5

        uint8_t * HEADER;	// This is the rom header (first 40h bytes of the rom
        // This will be in the same memory format as the rest of the memory.
        uint8_t * RDRAM;
        uint8_t * DMEM;
        uint8_t * IMEM;

        uint32_t * MI_INTR_REG;

        uint32_t * DPC_START_REG;
        uint32_t * DPC_END_REG;
        uint32_t * DPC_CURRENT_REG;
        uint32_t * DPC_STATUS_REG;
        uint32_t * DPC_CLOCK_REG;
        uint32_t * DPC_BUFBUSY_REG;
        uint32_t * DPC_PIPEBUSY_REG;
        uint32_t * DPC_TMEM_REG;

        uint32_t * VI_STATUS_REG;
        uint32_t * VI_ORIGIN_REG;
        uint32_t * VI_WIDTH_REG;
        uint32_t * VI_INTR_REG;
        uint32_t * VI_V_CURRENT_LINE_REG;
        uint32_t * VI_TIMING_REG;
        uint32_t * VI_V_SYNC_REG;
        uint32_t * VI_H_SYNC_REG;
        uint32_t * VI_LEAP_REG;
        uint32_t * VI_H_START_REG;
        uint32_t * VI_V_START_REG;
        uint32_t * VI_V_BURST_REG;
        uint32_t * VI_X_SCALE_REG;
        uint32_t * VI_Y_SCALE_REG;

        void(*CheckInterrupts)(void);
#ifdef ANDROID
        void(CALL *SwapBuffers)(void);
#endif
    } GFX_INFO;

    extern GFX_INFO gfx;
    extern bool no_dlist;

    /******************************************************************
    Function: CaptureScreen
    Purpose:  This function dumps the current frame to a file
    input:    pointer to the directory to save the file to
    output:   none
    *******************************************************************/
    EXPORT void CALL CaptureScreen(char * Directory);

    /******************************************************************
    Function: ChangeWindow
    Purpose:  to change the window between fullscreen and window
    mode. If the window was in fullscreen this should
    change the screen to window mode and vice vesa.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ChangeWindow(void);

    /******************************************************************
    Function: CloseDLL
    Purpose:  This function is called when the emulator is closing
    down allowing the dll to de-initialise.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL CloseDLL(void);

    /******************************************************************
    Function: DllAbout
    Purpose:  This function is optional function that is provided
    to give further information about the DLL.
    input:    a handle to the window that calls this function
    output:   none
    *******************************************************************/
    EXPORT void CALL DllAbout(void * hParent);

    /******************************************************************
    Function: DllConfig
    Purpose:  This function is optional function that is provided
    to allow the user to configure the dll
    input:    a handle to the window that calls this function
    output:   none
    *******************************************************************/
    EXPORT void CALL DllConfig(void * hParent);

    /******************************************************************
    Function: DllTest
    Purpose:  This function is optional function that is provided
    to allow the user to test the dll
    input:    a handle to the window that calls this function
    output:   none
    *******************************************************************/
    EXPORT void CALL DllTest(void * hParent);

    /******************************************************************
    Function: DrawScreen
    Purpose:  This function is called when the emulator receives a
    WM_PAINT message. This allows the gfx to fit in when
    it is being used in the desktop.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL DrawScreen(void);

    /******************************************************************
    Function: GetDllInfo
    Purpose:  This function allows the emulator to gather information
    about the dll by filling in the PluginInfo structure.
    input:    a pointer to a PLUGIN_INFO stucture that needs to be
    filled by the function. (see def above)
    output:   none
    *******************************************************************/
    EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo);

    /******************************************************************
    Function: InitiateGFX
    Purpose:  This function is called when the DLL is started to give
    information from the emulator that the n64 graphics
    uses. This is not called from the emulation thread.
    Input:    Gfx_Info is passed to this function which is defined
    above.
    Output:   TRUE on success
    FALSE on failure to initialise

    ** note on interrupts **:
    To generate an interrupt set the appropriate bit in MI_INTR_REG
    and then call the function CheckInterrupts to tell the emulator
    that there is a waiting interrupt.
    *******************************************************************/
    EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info);

    /******************************************************************
    Function: MoveScreen
    Purpose:  This function is called in response to the emulator
    receiving a WM_MOVE passing the xpos and ypos passed
    from that message.
    input:    xpos - the x-coordinate of the upper-left corner of the
    client area of the window.
    ypos - y-coordinate of the upper-left corner of the
    client area of the window.
    output:   none
    *******************************************************************/
    EXPORT void CALL MoveScreen(int xpos, int ypos);

    /******************************************************************
    Function: ProcessDList
    Purpose:  This function is called when there is a Dlist to be
    processed. (High level GFX list)
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ProcessDList(void);

    /******************************************************************
    Function: ProcessRDPList
    Purpose:  This function is called when there is a Dlist to be
    processed. (Low level GFX list)
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ProcessRDPList(void);

    /******************************************************************
    Function: RomClosed
    Purpose:  This function is called when a rom is closed.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL RomClosed(void);

    /******************************************************************
    Function: RomOpen
    Purpose:  This function is called when a rom is open. (from the
    emulation thread)
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL RomOpen(void);

    /******************************************************************
    Function: ShowCFB
    Purpose:  Useally once Dlists are started being displayed, cfb is
    ignored. This function tells the dll to start displaying
    them again.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ShowCFB(void);

    /******************************************************************
    Function: UpdateScreen
    Purpose:  This function is called in response to a vsync of the
    screen were the VI bit in MI_INTR_REG has already been
    set
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL UpdateScreen(void);

    /******************************************************************
    Function: ViStatusChanged
    Purpose:  This function is called to notify the dll that the
    ViStatus registers value has been changed.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ViStatusChanged(void);

    /******************************************************************
    Function: ViWidthChanged
    Purpose:  This function is called to notify the dll that the
    ViWidth registers value has been changed.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL ViWidthChanged(void);

#ifdef ANDROID
    /******************************************************************
    Function: SurfaceCreated
    Purpose:  this function is called when the surface is created.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL SurfaceCreated(void);
    /******************************************************************
    Function: SurfaceChanged
    Purpose:  this function is called when the surface is has changed.
    input:    none
    output:   none
    *******************************************************************/
    EXPORT void CALL SurfaceChanged(int width, int height);
#endif
    /******************************************************************
    Function: FrameBufferWrite
    Purpose:  This function is called to notify the dll that the
    frame buffer has been modified by CPU at the given address.
    input:    addr		rdram address
    val			val
    size		1 = uint8_t, 2 = uint16_t, 4 = uint32_t
    output:   none
    *******************************************************************/
    EXPORT void CALL FBWrite(uint32_t, uint32_t);

    typedef struct
    {
        uint32_t addr;
        uint32_t val;
        uint32_t size;				// 1 = uint8_t, 2 = uint16_t, 4=uint32_t
    } FrameBufferModifyEntry;

    /******************************************************************
    Function: FrameBufferWriteList
    Purpose:  This function is called to notify the dll that the
    frame buffer has been modified by CPU at the given address.
    input:    FrameBufferModifyEntry *plist
    size = size of the plist, max = 1024
    output:   none
    *******************************************************************/
    EXPORT void CALL FBWList(FrameBufferModifyEntry *plist, uint32_t size);

    /******************************************************************
    Function: FrameBufferRead
    Purpose:  This function is called to notify the dll that the
    frame buffer memory is beening read at the given address.
    DLL should copy content from its render buffer to the frame buffer
    in N64 RDRAM
    DLL is responsible to maintain its own frame buffer memory addr list
    DLL should copy 4KB block content back to RDRAM frame buffer.
    Emulator should not call this function again if other memory
    is read within the same 4KB range
    input:    addr		rdram address
    val			val
    size		1 = uint8_t, 2 = uint16_t, 4 = uint32_t
    output:   none
    *******************************************************************/
    EXPORT void CALL FBRead(uint32_t addr);

    /************************************************************************
    Function: FBGetFrameBufferInfo
    Purpose:  This function is called by the emulator core to retrieve depth
    buffer information from the video plugin in order to be able
    to notify the video plugin about CPU depth buffer read/write
    operations

    size:
    = 1		byte
    = 2		word (16 bit) <-- this is N64 default depth buffer format
    = 4		dword (32 bit)

    when depth buffer information is not available yet, set all values
    in the FrameBufferInfo structure to 0

    input:    FrameBufferInfo *pinfo
    pinfo is pointed to a FrameBufferInfo structure which to be
    filled in by this function
    output:   Values are return in the FrameBufferInfo structure
    ************************************************************************/
    EXPORT void CALL FBGetFrameBufferInfo(void *pinfo);

    EXPORT void CALL PluginLoaded(void);

#if defined(__cplusplus)
}
#endif
