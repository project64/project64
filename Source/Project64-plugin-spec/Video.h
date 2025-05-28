#pragma once
#include "Base.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    void * hWnd;       // Render window
    void * hStatusBar; // If render window does not have a status bar then this is NULL

    int32_t Reserved;

    uint8_t * HEADER; // This is the ROM header (first 40h bytes of the ROM)
    uint8_t * RDRAM;
    uint8_t * DMEM;
    uint8_t * IMEM;

    uint32_t RDRAM_SIZE;

    uint32_t * MI_INTR_REG;

    uint32_t * SP_MEM_ADDR_REG;
    uint32_t * SP_DRAM_ADDR_REG;
    uint32_t * SP_RD_LEN_REG;
    uint32_t * SP_WR_LEN_REG;
    uint32_t * SP_STATUS_REG;
    uint32_t * SP_DMA_FULL_REG;
    uint32_t * SP_DMA_BUSY_REG;
    uint32_t * SP_PC_REG;
    uint32_t * SP_SEMAPHORE_REG;

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
Function: DrawStatus
Purpose: This function displays a status string on the screen, 
optionally right-aligned.
Input: lpString - pointer to the status string to display
RightAlign - nonzero to right-align the string,
zero for left alignment
Output: none
*/
EXPORT void CALL DrawStatus(const char * lpString, int32_t RightAlign);

/*
Function: InitiateGFX
Purpose: This function is called when the DLL is started to give
information from the emulator that the N64 graphics
uses. This is not called from the emulation thread.
Input: GFX_INFO is passed to this function which is defined above.
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
Function: OnRomBrowserMenuItem
Purpose: Callback when the ROM Browser menu supplied by the
caller is invoked
Input: MenuID - ID of the selected menu item
hParent - handle to the parent window
HEADER - pointer to the ROM header
Output: none
*/
extern void (CALL * OnRomBrowserMenuItem)(int32_t MenuID, void * hParent, uint8_t * HEADER);
extern void * (CALL * GetRomBrowserMenu)(void); // Items should have an ID between 4101 and 4200
    
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
ignored. This function tells the DLL to start displaying
them again.
Input: none
Output: none
*/
EXPORT void CALL ShowCFB(void);

/*
Function: SoftReset
Purpose: This function is called to notify the plugin that a
soft reset has occurred.
Input: none
Output: none
*/
EXPORT void CALL SoftReset(void);

/*
Function: UpdateScreen
Purpose: This function is called in response to a vsync of the
screen were the VI bit in MI_INTR_REG has already been set
Input: none
Output: none
*/
EXPORT void CALL UpdateScreen(void);

/*
Function: ViStatusChanged
Purpose: This function is called to notify the DLL that the
ViStatus registers value has been changed.
Input: none
Output: none
*/
EXPORT void CALL ViStatusChanged(void);

/*
Function: ViWidthChanged
Purpose: This function is called to notify the DLL that the
ViWidth registers value has been changed.
Input: none
Output: none
*/
EXPORT void CALL ViWidthChanged(void);

typedef struct
{
    long left, top, right, bottom;
} rectangle; // <windows.h> equivalent: RECT

typedef struct
{
    void * hdc;
    int32_t fErase;
    rectangle rcPaint;
    int32_t fRestore;
    int32_t fIncUpdate;
    uint8_t rgbReserved[32];
} window_paint; // <windows.h> equivalent: PAINTSTRUCT

typedef struct
{
    // Menu
    // Items should have an ID between 5101 and 5200
    void * hGFXMenu;
    void (*ProcessMenuItem)(int32_t ID);

    // Breakpoints
    int32_t UseBPoints;
    char BPPanelName[20];
    void (*Add_BPoint)(void);
    void (*CreateBPPanel)(void * hDlg, rectangle rcBox);
    void (*HideBPPanel)(void);
    void (*PaintBPPanel)(window_paint ps);
    void (*ShowBPPanel)(void);
    void (*RefreshBpoints)(void * hList);
    void (*RemoveBpoint)(void * hList, int index);
    void (*RemoveAllBpoint)(void);

    // GFX command window
    void (*Enter_GFX_Commands_Window)(void);
} GFXDEBUG_INFO;

typedef struct
{
    void (*UpdateBreakPoints)(void);
    void (*UpdateMemory)(void);
    void (*UpdateR4300iRegisters)(void);
    void (*Enter_BPoint_Window)(void);
    void (*Enter_R4300i_Commands_Window)(void);
    void (*Enter_R4300i_Register_Window)(void);
    void (*Enter_GFX_Commands_Window)(void);
    void (*Enter_Memory_Window)(void);
} DEBUG_INFO;

/*
Function: InitiateDebugger
Purpose: This function is called when the DLL is started to be
given information from the emulator that the N64 GFX
interface needs to intergrate the debugger with the
rest of the emulator.
Input: DEBUG_INFO is passed to this function which is defined
above.
Output: none
*/ 
EXPORT void CALL InitiateDebugger(DEBUG_INFO DebugInfo);

/*
Function: GetDebugInfo
Purpose: This function allows the emulator to gather information
about the debug capabilities of the DLL by filling in
the DEBUG_INFO structure.
Input: a pointer to a GFXDEBUG_INFO stucture that needs to be
filled by the function. (see def above)
Output: none
*/ 
EXPORT void CALL GetDebugInfo(GFXDEBUG_INFO * GFXDebugInfo);

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
