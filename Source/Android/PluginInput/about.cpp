#ifdef _WIN32
#include <Windows.h>
#endif

void ShowAboutWindow (void * hParent)
{
#ifdef _WIN32
    MessageBox((HWND)hParent,L"Android input plugin",L"Dll About",MB_OK);
#endif
}
