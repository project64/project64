#include "stdafx.h"
#include <Project64-core/AppInit.h>
#include "Multilanguage/LanguageSelector.h"
#include "Settings/UISettings.h"

#if defined(_WIN32)
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
        CMainGui  MainWindow(true, stdstr_f("Project64 %s", VER_FILE_VERSION_STR).c_str()), HiddenWindow(false);
        CMainMenu MainMenu(&MainWindow);
        g_Plugins->SetRenderWindows(&MainWindow, &HiddenWindow);
        Notify().SetMainWindow(&MainWindow);

        if (g_Settings->LoadStringVal(Cmd_RomFile).length() > 0)
        {
            MainWindow.Show(true);	//Show the main window
            //N64 ROM or 64DD Disk
            stdstr ext = CPath(g_Settings->LoadStringVal(Cmd_RomFile)).GetExtension();
            if (!(_stricmp(ext.c_str(), "ndd") == 0))
            {
                //File Extension is not *.ndd so it should be a N64 ROM
                CN64System::RunFileImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str());
            }
            else
            {
                //Ext is *.ndd, so it should be a disk file.
                if (CN64System::RunDiskImage(g_Settings->LoadStringVal(Cmd_RomFile).c_str()))
                {
                    stdstr IPLROM = g_Settings->LoadStringVal(File_DiskIPLPath);
                    if ((IPLROM.length() <= 0) || (!CN64System::RunFileImage(IPLROM.c_str())))
                    {
                        // Open DDROM
                        OPENFILENAME openfilename;
                        char FileName[_MAX_PATH], Directory[_MAX_PATH];
                        memset(&FileName, 0, sizeof(FileName));
                        memset(&openfilename, 0, sizeof(openfilename));

                        openfilename.lStructSize = sizeof(openfilename);
                        //openfilename.hwndOwner = (HWND)hWnd;
                        openfilename.lpstrFilter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
                        openfilename.lpstrFile = FileName;
                        openfilename.lpstrInitialDir = Directory;
                        openfilename.nMaxFile = MAX_PATH;
                        openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                        if (GetOpenFileName(&openfilename))
                        {
                            CN64System::RunFileImage(FileName);
                        }
                    }
                }
            }
        }
        else
        {
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
#else
int main(int argc, char* argv[])
{
    printf("%s:  %i arguments\n", argv[0], argc);
    return 0;
}
#endif

