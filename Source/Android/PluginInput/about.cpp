#ifdef _WIN32
#include <Windows.h>
#endif

void ShowAboutWindow (void * hParent)
{
#ifdef _WIN32
    MessageBox((HWND)hParent,"Android Input Plugin","Dll About",MB_OK);
#endif
}