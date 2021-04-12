#include "stdafx.h"
#include <Project64-core/AppInit.h>
#include "UserInterface/WelcomeScreen.h"
#include "Settings/UISettings.h"

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszArgs*/, int /*nWinMode*/)
{
    try
    {
        CoInitialize(nullptr);
        AppInit(&Notify(), CPath(CPath::MODULE_DIRECTORY), __argc, __argv);
        if (!g_Lang->IsLanguageLoaded())
        {
            WelcomeScreen().DoModal();
        }

        // Create the main window with menu
		
        WriteTrace(TraceUserInterface, TraceDebug, "Create main window");
        CMainGui MainWindow(true, stdstr_f("Project64 %s", VER_FILE_VERSION_STR).c_str()), HiddenWindow(false);
        CMainMenu MainMenu(&MainWindow);
        CDebuggerUI Debugger;
        g_Debugger = &Debugger;
        g_Plugins->SetRenderWindows(&MainWindow, &HiddenWindow);
        Notify().SetMainWindow(&MainWindow);
        bool isROMLoaded = false;

        if (g_Settings->LoadStringVal(Cmd_RomFile).length() > 0 && g_Settings->LoadStringVal(Cmd_ComboDiskFile).length() > 0)
        {
            // Handle combo loading (N64 ROM and 64DD Disk)

            MainWindow.Show(true);	// Show the main window

            stdstr extcombo = CPath(g_Settings->LoadStringVal(Cmd_ComboDiskFile)).GetExtension();
            stdstr ext = CPath(g_Settings->LoadStringVal(Cmd_RomFile)).GetExtension();

            if (g_Settings->LoadStringVal(Cmd_ComboDiskFile).length() > 0
                && ((_stricmp(extcombo.c_str(), "ndd") == 0) || (_stricmp(extcombo.c_str(), "d64") == 0)))
            {
                if ((!(_stricmp(ext.c_str(), "ndd") == 0)) && (!(_stricmp(ext.c_str(), "d64") == 0)))
                {
                    // Cmd_ComboDiskFile must be a 64DD disk image
                    // Cmd_RomFile must be an N64 ROM image
                    isROMLoaded = CN64System::RunDiskComboImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str(), g_Settings->LoadStringVal(Cmd_ComboDiskFile).c_str());
                }
            }
        }
        else if (g_Settings->LoadStringVal(Cmd_RomFile).length() > 0)
        {
            // Handle single game (N64 ROM or 64DD Disk)

            MainWindow.Show(true);	// Show the main window

            stdstr ext = CPath(g_Settings->LoadStringVal(Cmd_RomFile)).GetExtension();
            if ((!(_stricmp(ext.c_str(), "ndd") == 0)) && (!(_stricmp(ext.c_str(), "d64") == 0)))
            {
                // File extension is not *.ndd/*.d64 so it should be an N64 ROM
                isROMLoaded = CN64System::RunFileImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str());
            }
            else
            {
                // File extension is *.ndd/*.d64, so it should be an N64 disk image
                isROMLoaded = CN64System::RunDiskImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str());
            }
        }

        if (!isROMLoaded)
        {
            CSupportWindow(MainWindow.Support()).Show((HWND)MainWindow.GetWindowHandle(), true);
            if (UISettingsLoadBool(RomBrowser_Enabled))
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Show ROM browser");
                MainWindow.ShowRomList();
                MainWindow.Show(true);
                MainWindow.HighLightLastRom();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Show main window");
                MainWindow.Show(true);
            }
        }

        WriteTrace(TraceUserInterface, TraceDebug, "Entering message loop");
        MainWindow.ProcessAllMessages();
        WriteTrace(TraceUserInterface, TraceDebug, "Message loop finished");

        if (g_BaseSystem)
        {
            g_BaseSystem->CloseCpu();
            delete g_BaseSystem;
            g_BaseSystem = nullptr;
        }
        WriteTrace(TraceUserInterface, TraceDebug, "System closed");
    }
    catch (...)
    {
        WriteTrace(TraceUserInterface, TraceError, "Exception caught (File: \"%s\" Line: %d)", __FILE__, __LINE__);
        MessageBox(nullptr, stdstr_f("Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__).ToUTF16().c_str(), L"Exception", MB_OK);
    }
    AppCleanup();
    CoUninitialize();
    return true;
}
