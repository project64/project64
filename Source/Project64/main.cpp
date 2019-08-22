#include "stdafx.h"
#include <Project64-core/AppInit.h>
#include "Multilanguage\LanguageSelector.h"
#include "Settings/UISettings.h"

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszArgs*/, int /*nWinMode*/)
{
    try
    {
        CoInitialize(NULL);
        AppInit(&Notify(), CPath(CPath::MODULE_DIRECTORY), __argc, __argv);
        if (!g_Lang->IsLanguageLoaded())
        {
            CLanguageSelector().Select();
        }

        //Create the main window with Menu
        WriteTrace(TraceUserInterface, TraceDebug, "Create Main Window");
        CMainGui MainWindow(true, stdstr_f("Project64 %s", VER_FILE_VERSION_STR).c_str()), HiddenWindow(false);
        CMainMenu MainMenu(&MainWindow);
        CDebuggerUI Debugger;
        g_Debugger = &Debugger;
        g_Plugins->SetRenderWindows(&MainWindow, &HiddenWindow);
        Notify().SetMainWindow(&MainWindow);
        CSupportWindow SupportWindow;
        bool isROMLoaded = false;

        if (g_Settings->LoadStringVal(Cmd_RomFile).length() > 0 && g_Settings->LoadStringVal(Cmd_ComboDiskFile).length() > 0)
        {
            //Handle Combo Loading (N64 ROM AND 64DD Disk)

            MainWindow.Show(true);	//Show the main window

            stdstr extcombo = CPath(g_Settings->LoadStringVal(Cmd_ComboDiskFile)).GetExtension();
            stdstr ext = CPath(g_Settings->LoadStringVal(Cmd_RomFile)).GetExtension();

            if (g_Settings->LoadStringVal(Cmd_ComboDiskFile).length() > 0
                && ((_stricmp(extcombo.c_str(), "ndd") == 0) || (_stricmp(extcombo.c_str(), "d64") == 0)))
            {
                if ((!(_stricmp(ext.c_str(), "ndd") == 0)) && (!(_stricmp(ext.c_str(), "d64") == 0)))
                {
                    //Cmd_ComboDiskFile must be a 64DD disk image
                    //Cmd_RomFile must be a N64 ROM image
                    isROMLoaded = CN64System::RunDiskComboImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str(), g_Settings->LoadStringVal(Cmd_ComboDiskFile).c_str());
                }
            }
        }
        else if (g_Settings->LoadStringVal(Cmd_RomFile).length() > 0)
        {
            //Handle Single Game (N64 ROM or 64DD Disk)

            MainWindow.Show(true);	//Show the main window

            stdstr ext = CPath(g_Settings->LoadStringVal(Cmd_RomFile)).GetExtension();
            if ((!(_stricmp(ext.c_str(), "ndd") == 0)) && (!(_stricmp(ext.c_str(), "d64") == 0)))
            {
                //File Extension is not *.ndd/*.d64 so it should be a N64 ROM
                isROMLoaded = CN64System::RunFileImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str());
            }
            else
            {
                //Ext is *.ndd/*.d64, so it should be a disk file.
                isROMLoaded = CN64System::RunDiskImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str());
            }
        }

        //Handle Main Window if ROM is not loaded and running
        if (!isROMLoaded)
        {
            SupportWindow.Show(reinterpret_cast<HWND>(MainWindow.GetWindowHandle()));
            if (UISettingsLoadBool(RomBrowser_Enabled))
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Show Rom Browser");
                //Display the rom browser
                MainWindow.ShowRomList();
                MainWindow.Show(true);	//Show the main window
                MainWindow.HighLightLastRom();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Show Main Window");
                MainWindow.Show(true);	//Show the main window
            }
        }

        //Process Messages till program is closed
        WriteTrace(TraceUserInterface, TraceDebug, "Entering Message Loop");
        MainWindow.ProcessAllMessages();
        WriteTrace(TraceUserInterface, TraceDebug, "Message Loop Finished");

        if (g_BaseSystem)
        {
            g_BaseSystem->CloseCpu();
            delete g_BaseSystem;
            g_BaseSystem = NULL;
        }
        WriteTrace(TraceUserInterface, TraceDebug, "System Closed");
    }
    catch (...)
    {
        WriteTrace(TraceUserInterface, TraceError, "Exception caught (File: \"%s\" Line: %d)", __FILE__, __LINE__);
        MessageBox(NULL, stdstr_f("Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__).c_str(), "Exception", MB_OK);
    }
    AppCleanup();
    CoUninitialize();
    return true;
}