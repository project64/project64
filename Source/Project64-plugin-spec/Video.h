#pragma once
#include "Base.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    void * hWnd;
    void * hStatusBar;

    int32_t Reserved;

    uint8_t * HEADER;
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

/*
Function: CaptureScreen
Purpose: This function dumps the current frame to a file
Input: pointer to the directory to save the file to
Output: none
*/
EXPORT void CALL CaptureScreen(const char * Directory);

/*
Function: ChangeWindow
Purpose: to change the window between fullscreen and window
mode. If the window was in fullscreen this should
change the screen to window mode and vice vesa.
Input: none
Output: none
*/
EXPORT void CALL ChangeWindow(void);

/*
Function: DrawScreen
Purpose: This function is called when the emulator receives a
WM_PAINT message. This allows the gfx to fit in when
it is being used in the desktop.
Input: none
Output: none
*/
EXPORT void CALL DrawScreen(void);

/*
Function: InitiateGFX
Purpose: This function is called when the DLL is started to give
information from the emulator that the n64 graphics
uses. This is not called from the emulation thread.
Input: Gfx_Info is passed to this function which is defined
above.
Output: TRUE on success
FALSE on failure to initialise

** note on interrupts **:
To generate an interrupt set the appropriate bit in MI_INTR_REG
and then call the function CheckInterrupts to tell the emulator
that there is a waiting interrupt.
*/
EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info);

/*
Function: MoveScreen
Purpose: This function is called in response to the emulator
receiving a WM_MOVE passing the xpos and ypos passed
from that message.
Input: xpos - the x-coordinate of the upper-left corner of the
client area of the window.
ypos - y-coordinate of the upper-left corner of the
client area of the window.
Output: none
*/
EXPORT void CALL MoveScreen(int xpos, int ypos);

/*
Function: ProcessDList
Purpose: This function is called when there is a Dlist to be
processed. (High level GFX list)
Input: none
Output: none
*/
EXPORT void CALL ProcessDList(void);

/*
Function: ProcessRDPList
Purpose: This function is called when there is a Dlist to be
processed. (Low level GFX list)
Input: none
Output: none
*/
EXPORT void CALL ProcessRDPList(void);

/*
Function: ShowCFB
Purpose: Useally once Dlists are started being displayed, cfb is
ignored. This function tells the dll to start displaying
them again.
Input: none
Output: none
*/
EXPORT void CALL ShowCFB(void);

/*
Function: UpdateScreen
Purpose: This function is called in response to a vsync of the
screen were the VI bit in MI_INTR_REG has already been
set
Input: none
Output: none
*/
EXPORT void CALL UpdateScreen(void);

/*
Function: ViStatusChanged
Purpose: This function is called to notify the dll that the
ViStatus registers value has been changed.
Input: none
Output: none
*/
EXPORT void CALL ViStatusChanged(void);

/*
Function: ViWidthChanged
Purpose: This function is called to notify the dll that the
ViWidth registers value has been changed.
Input: none
Output: none
*/
EXPORT void CALL ViWidthChanged(void);

#ifdef ANDROID
/*
Function: SurfaceCreated
Purpose: this function is called when the surface is created.
Input: none
Output: none
*/
EXPORT void CALL SurfaceCreated(void);

/*
Function: SurfaceChanged
Purpose: this function is called when the surface is has changed.
Input: none
Output: none
*/
EXPORT void CALL SurfaceChanged(int width, int height);
#endif

#if defined(__cplusplus)
}
#endif
