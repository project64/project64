#include "stdafx.h"

#include "GFXPlugin.h"
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>

CGfxPlugin::CGfxPlugin() :
    CaptureScreen(nullptr),
    ChangeWindow(nullptr),
    DrawScreen(nullptr),
    DrawStatus(nullptr),
    MoveScreen(nullptr),
    ProcessDList(nullptr),
    ProcessRDPList(nullptr),
    ShowCFB(nullptr),
    UpdateScreen(nullptr),
    ViStatusChanged(nullptr),
    ViWidthChanged(nullptr),
    SoftReset(nullptr),
    GetRomBrowserMenu(nullptr),
    OnRomBrowserMenuItem(nullptr),
    GetDebugInfo(nullptr),
    InitiateDebugger(nullptr)
{
    memset(&m_GFXDebug, 0, sizeof(m_GFXDebug));
}

CGfxPlugin::~CGfxPlugin()
{
    WriteTrace(TraceVideoPlugin, TraceDebug, "Start");
    Close(nullptr);
    UnloadPlugin();
    WriteTrace(TraceVideoPlugin, TraceDebug, "Done");
}

bool CGfxPlugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    int32_t(CALL * InitiateGFX)(void * Gfx_Info);
    LoadFunction(InitiateGFX);
    LoadFunction(ChangeWindow);
    LoadFunction(DrawScreen);
    LoadFunction(MoveScreen);
    LoadFunction(ProcessDList);
    LoadFunction(UpdateScreen);
    LoadFunction(ViStatusChanged);
    LoadFunction(ViWidthChanged);
    LoadFunction(SoftReset);
#ifdef ANDROID
    LoadFunction(SurfaceCreated);
    LoadFunction(SurfaceChanged);
#endif

    // Version 0x104 functions
    _LoadFunction("DrawFullScreenStatus", DrawStatus);

    // ROM browser
    LoadFunction(GetRomBrowserMenu);
    LoadFunction(OnRomBrowserMenuItem);

    // Make sure DLL had all needed functions
    if (ChangeWindow == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (DrawScreen == nullptr)
    {
        DrawScreen = DummyDrawScreen;
    }
    if (InitiateGFX == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (MoveScreen == nullptr)
    {
        MoveScreen = DummyMoveScreen;
    }
    if (ProcessDList == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (UpdateScreen == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (ViStatusChanged == nullptr)
    {
        ViStatusChanged = DummyViStatusChanged;
    }
    if (ViWidthChanged == nullptr)
    {
        ViWidthChanged = DummyViWidthChanged;
    }
    if (SoftReset == nullptr)
    {
        SoftReset = DummySoftReset;
    }

    if (m_PluginInfo.Version >= 0x0103)
    {
        LoadFunction(ProcessRDPList);
        LoadFunction(CaptureScreen);
        LoadFunction(ShowCFB);
        LoadFunction(GetDebugInfo);
        _LoadFunction("InitiateGFXDebugger", InitiateDebugger);

        if (ProcessRDPList == nullptr)
        {
            UnloadPlugin();
            return false;
        }
        if (CaptureScreen == nullptr)
        {
            UnloadPlugin();
            return false;
        }
        if (ShowCFB == nullptr)
        {
            UnloadPlugin();
            return false;
        }
    }

    if (m_PluginInfo.Version >= 0x0104)
    {
        if (PluginOpened == nullptr)
        {
            UnloadPlugin();
            return false;
        }
    }

    if (GetDebugInfo != nullptr)
    {
        GetDebugInfo(&m_GFXDebug);
    }
    return true;
}

bool CGfxPlugin::Initiate(CN64System * System, RenderWindow * Window)
{
    WriteTrace(TraceVideoPlugin, TraceDebug, "Start");
    if (m_Initialized)
    {
        Close(Window);
        if (PluginOpened)
        {
            WriteTrace(PluginTraceType(), TraceDebug, "Before plugin opened");
            PluginOpened();
            WriteTrace(PluginTraceType(), TraceDebug, "After plugin opened");
        }
    }

    typedef struct
    {
        void * hWnd;       // Render window
        void * hStatusBar; // If render window does not have a status bar then this is NULL

        int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary
        //	eg. the first 8 bytes are stored like this:
        //  4 3 2 1   8 7 6 5

        uint8_t * HEADER; // This is the ROM header (first 40h bytes of the ROM)
        // This will be in the same memory format as the rest of the memory
        uint8_t * RDRAM;
        uint8_t * DMEM;
        uint8_t * IMEM;

        uint32_t * MI__INTR_REG;

        uint32_t * DPC__START_REG;
        uint32_t * DPC__END_REG;
        uint32_t * DPC__CURRENT_REG;
        uint32_t * DPC__STATUS_REG;
        uint32_t * DPC__CLOCK_REG;
        uint32_t * DPC__BUFBUSY_REG;
        uint32_t * DPC__PIPEBUSY_REG;
        uint32_t * DPC__TMEM_REG;

        uint32_t * VI__STATUS_REG;
        uint32_t * VI__ORIGIN_REG;
        uint32_t * VI__WIDTH_REG;
        uint32_t * VI__INTR_REG;
        uint32_t * VI__V_CURRENT_LINE_REG;
        uint32_t * VI__TIMING_REG;
        uint32_t * VI__V_SYNC_REG;
        uint32_t * VI__H_SYNC_REG;
        uint32_t * VI__LEAP_REG;
        uint32_t * VI__H_START_REG;
        uint32_t * VI__V_START_REG;
        uint32_t * VI__V_BURST_REG;
        uint32_t * VI__X_SCALE_REG;
        uint32_t * VI__Y_SCALE_REG;

        void(CALL * CheckInterrupts)(void);
#ifdef ANDROID
        void(CALL * SwapBuffers)(void);
#endif
    } GFX_INFO;

    // Get function from DLL
    int32_t(CALL * InitiateGFX)(GFX_INFO Gfx_Info);
    _LoadFunction("InitiateGFX", InitiateGFX);
    if (InitiateGFX == nullptr)
    {
        WriteTrace(TraceVideoPlugin, TraceDebug, "Failed to find InitiateGFX");
        return false;
    }

    GFX_INFO Info = {0};

    Info.MemoryBswaped = true;
#if defined(ANDROID) || defined(__ANDROID__)
    Info.SwapBuffers = SwapBuffers;
#endif
    Info.hWnd = nullptr;
    Info.hStatusBar = nullptr;
#ifdef _WIN32
    if (Window != nullptr)
    {
        Info.hWnd = Window->GetWindowHandle();
        Info.hStatusBar = Window->GetStatusBar();
    }
#endif
    Info.CheckInterrupts = DummyCheckInterrupts;

    // We are initializing the plugin before any ROM is loaded so we do not have any correct
    // parameters here, it's just needed so we can config the DLL
    WriteTrace(TraceVideoPlugin, TraceDebug, "System = %X", System);
    if (System == nullptr)
    {
        static uint8_t Buffer[100];
        static uint32_t Value = 0;

        Info.HEADER = Buffer;
        Info.RDRAM = Buffer;
        Info.DMEM = Buffer;
        Info.IMEM = Buffer;
        Info.MI__INTR_REG = &Value;
        Info.VI__STATUS_REG = &Value;
        Info.VI__ORIGIN_REG = &Value;
        Info.VI__WIDTH_REG = &Value;
        Info.VI__INTR_REG = &Value;
        Info.VI__V_CURRENT_LINE_REG = &Value;
        Info.VI__TIMING_REG = &Value;
        Info.VI__V_SYNC_REG = &Value;
        Info.VI__H_SYNC_REG = &Value;
        Info.VI__LEAP_REG = &Value;
        Info.VI__H_START_REG = &Value;
        Info.VI__V_START_REG = &Value;
        Info.VI__V_BURST_REG = &Value;
        Info.VI__X_SCALE_REG = &Value;
        Info.VI__Y_SCALE_REG = &Value;
    }
    // Send initialization information to the DLL
    else
    {
        CMipsMemoryVM & MMU = System->m_MMU_VM;
        CRegisters & Reg = System->m_Reg;

        if (g_Rom->IsLoadedRomDDIPL() && g_Disk != nullptr)
            Info.HEADER = g_Disk->GetDiskHeader();
        else
            Info.HEADER = g_Rom->GetRomAddress();
        Info.RDRAM = MMU.Rdram();
        Info.DMEM = MMU.Dmem();
        Info.IMEM = MMU.Imem();
        Info.MI__INTR_REG = &Reg.m_GfxIntrReg;
        Info.DPC__START_REG = &Reg.DPC_START_REG;
        Info.DPC__END_REG = &Reg.DPC_END_REG;
        Info.DPC__CURRENT_REG = &Reg.DPC_CURRENT_REG;
        Info.DPC__STATUS_REG = &Reg.DPC_STATUS_REG;
        Info.DPC__CLOCK_REG = &Reg.DPC_CLOCK_REG;
        Info.DPC__BUFBUSY_REG = &Reg.DPC_BUFBUSY_REG;
        Info.DPC__PIPEBUSY_REG = &Reg.DPC_PIPEBUSY_REG;
        Info.DPC__TMEM_REG = &Reg.DPC_TMEM_REG;
        Info.VI__STATUS_REG = &Reg.VI_STATUS_REG;
        Info.VI__ORIGIN_REG = &Reg.VI_ORIGIN_REG;
        Info.VI__WIDTH_REG = &Reg.VI_WIDTH_REG;
        Info.VI__INTR_REG = &Reg.VI_INTR_REG;
        Info.VI__V_CURRENT_LINE_REG = &Reg.VI_CURRENT_REG;
        Info.VI__TIMING_REG = &Reg.VI_TIMING_REG;
        Info.VI__V_SYNC_REG = &Reg.VI_V_SYNC_REG;
        Info.VI__H_SYNC_REG = &Reg.VI_H_SYNC_REG;
        Info.VI__LEAP_REG = &Reg.VI_LEAP_REG;
        Info.VI__H_START_REG = &Reg.VI_H_START_REG;
        Info.VI__V_START_REG = &Reg.VI_V_START_REG;
        Info.VI__V_BURST_REG = &Reg.VI_V_BURST_REG;
        Info.VI__X_SCALE_REG = &Reg.VI_X_SCALE_REG;
        Info.VI__Y_SCALE_REG = &Reg.VI_Y_SCALE_REG;
    }

    WriteTrace(TraceVideoPlugin, TraceDebug, "Calling InitiateGFX");
    m_Initialized = InitiateGFX(Info) != 0;

    WriteTrace(TraceVideoPlugin, TraceDebug, "Done (res: %s)", m_Initialized ? "true" : "false");
    return m_Initialized;
}

void CGfxPlugin::UnloadPluginDetails(void)
{
    WriteTrace(TraceVideoPlugin, TraceDebug, "Start");
    if (m_LibHandle != nullptr)
    {
        DynamicLibraryClose(m_LibHandle);
        m_LibHandle = nullptr;
    }
    memset(&m_GFXDebug, 0, sizeof(m_GFXDebug));

    ChangeWindow = nullptr;
    GetDebugInfo = nullptr;
    DrawScreen = nullptr;
    DrawStatus = nullptr;
    InitiateDebugger = nullptr;
    MoveScreen = nullptr;
    ProcessDList = nullptr;
    ProcessRDPList = nullptr;
    ShowCFB = nullptr;
    UpdateScreen = nullptr;
    ViStatusChanged = nullptr;
    ViWidthChanged = nullptr;
    GetRomBrowserMenu = nullptr;
    OnRomBrowserMenuItem = nullptr;
    WriteTrace(TraceVideoPlugin, TraceDebug, "Done");
}

void CGfxPlugin::ProcessMenuItem(int32_t id)
{
    if (m_GFXDebug.ProcessMenuItem)
    {
        m_GFXDebug.ProcessMenuItem(id);
    }
}

#ifdef ANDROID
void CGfxPlugin::SwapBuffers(void)
{
    RenderWindow * render = g_Plugins ? g_Plugins->MainWindow() : nullptr;
    WriteTrace(TraceVideoPlugin, TraceDebug, "Start (render: %p)", render);
    if (render != nullptr)
    {
        render->SwapWindow();
    }
    WriteTrace(TraceVideoPlugin, TraceDebug, "Done");
}
#endif
