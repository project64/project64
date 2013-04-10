#include <stdio.h>

#ifdef _WIN32 //Windows, duh!
#include <objbase.h>
#include "dxdiag.h"
#include "main.h"
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dxguid.lib")
#define SAFE_RELEASE(x) { if (x != NULL) { x->Release(); x = NULL; } }

IDxDiagProvider* Provider;
IDxDiagContainer* RootContainer;
bool ComInitialized;

static void InitCom()
{
  try
  {
    Provider = NULL;
    RootContainer = NULL;

    // Init COM
    ComInitialized = SUCCEEDED(CoInitialize(NULL));

    // Create a provider interface
    if (FAILED(CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (void**)&Provider)))
      throw "Unable to create provider instance";

    // Initialize the provider
    DXDIAG_INIT_PARAMS initParams;
    ZeroMemory(&initParams, sizeof(DXDIAG_INIT_PARAMS));
    initParams.dwSize = sizeof(DXDIAG_INIT_PARAMS);
    initParams.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
    if (FAILED(Provider->Initialize(&initParams)))
      throw "Unable to initialize provider";

    // Get the root container
    if (FAILED(Provider->GetRootContainer(&RootContainer)))
      throw "Unable to get root container";
  }
  catch (const char* msg)
  {
    LOG("\nDxDiag Error: %s", msg);
  }
}

static IDxDiagContainer* GetContainer(IDxDiagContainer* parent, const WCHAR* name)
{
  IDxDiagContainer* container;
  if (SUCCEEDED(parent->GetChildContainer(name, &container)))
    return container;

  return NULL;
}

static void GetPropertyValue(IDxDiagContainer* container, const WCHAR* name, WCHAR* value, int maxValueLen)
{
  VARIANT var;
  VariantInit(&var);
  if (SUCCEEDED(container->GetProp(name, &var)))
  {
    // Assuming an integer or bstring value here
    // @@ Handle all the VT_* types properly...
    if (var.vt != VT_BSTR)
      wsprintf((LPSTR)value, "%i", var.iVal);
    else
      wcsncpy(value, var.bstrVal, maxValueLen - 1);

    value[maxValueLen - 1] = 0;
    VariantClear(&var);
  }
  else
  {
    value[0] = 0;
  }
}

static int GetTotalVideoMemory()
{
  if (RootContainer != NULL)
  {
    // Get device container
    IDxDiagContainer* container = GetContainer(RootContainer, L"DxDiag_DisplayDevices");
    if (container != NULL)
    {
      // Get device name
      container = GetContainer(container, L"0");
      if (container != NULL)
      {
        const int bufferLength = 256;
        WCHAR buffer[bufferLength];
        GetPropertyValue(container, L"szDisplayMemoryLocalized", buffer, bufferLength);
        // Value in MB is first token in string
        return _wtoi(buffer);
      }
    }
  }

  // No good!
  return -1;
}

int getVRAMSize()
{
  static int mem;
  if (!mem) {
    InitCom();
    mem = GetTotalVideoMemory();
  }
  return mem * 1024 * 1024;
}
#endif


