#include "stdafx.h"
#include <Project64\AppInit.h>
#include "Multilanguage\LanguageSelector.h"

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszArgs*/, int /*nWinMode*/)
{
    try
    {
        CoInitialize(NULL);
        AppInit(&Notify());
        if (!g_Lang->IsLanguageLoaded())
        {
            CLanguageSelector().Select();
        }

        //Create the main window with Menu
        WriteTrace(TraceDebug, __FUNCTION__ ": Create Main Window");
        CMainGui  MainWindow(true, stdstr_f("Project64 %s", VER_FILE_VERSION_STR).c_str()), HiddenWindow(false);
        CMainMenu MainMenu(&MainWindow);
        g_Plugins->SetRenderWindows(&MainWindow, &HiddenWindow);
        Notify().SetMainWindow(&MainWindow);

        if (__argc > 1)
        {
            WriteTraceF(TraceDebug, __FUNCTION__ ": Cmd line found \"%s\"", __argv[1]);
            MainWindow.Show(true);	//Show the main window
            CN64System::RunFileImage(__argv[1]);
        }
        else
        {
            if (g_Settings->LoadDword(RomBrowser_Enabled))
            {
                WriteTrace(TraceDebug, __FUNCTION__ ": Show Rom Browser");
                //Display the rom browser
                MainWindow.ShowRomList();
                MainWindow.Show(true);	//Show the main window
                MainWindow.HighLightLastRom();
            }
            else {
                WriteTrace(TraceDebug, __FUNCTION__ ": Show Main Window");
                MainWindow.Show(true);	//Show the main window
            }
        }

        //Process Messages till program is closed
        WriteTrace(TraceDebug, __FUNCTION__ ": Entering Message Loop");
        MainWindow.ProcessAllMessages();
        WriteTrace(TraceDebug, __FUNCTION__ ": Message Loop Finished");

        if (g_BaseSystem)
        {
            g_BaseSystem->CloseCpu();
            delete g_BaseSystem;
            g_BaseSystem = NULL;
        }
        WriteTrace(TraceDebug, __FUNCTION__ ": System Closed");
    }
    catch (...)
    {
        WriteTraceF(TraceError, __FUNCTION__ ": Exception caught (File: \"%s\" Line: %d)", __FILE__, __LINE__);
        MessageBox(NULL, stdstr_f("Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__).c_str(), "Exception", MB_OK);
    }
    AppCleanup();
    CoUninitialize();
    return true;
}