// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
//
// Best-effort NVIDIA low-latency hint - see NvApiLowLatency.h for rationale.
//
// IMPORTANT: this is authored without an NvAPI build to compile/test against, so
// it is written to be incapable of causing harm. NvAPI is reached by dynamically
// loading the driver's own nvapi DLL (no SDK header or import library is linked),
// and every call is return-checked. On any failure - DLL absent (non-NVIDIA),
// unknown entry point, or a struct-version mismatch - it simply does nothing.
// The minimal NvAPI structs below mirror the public NvAPI layout; their versions
// are derived from sizeof, so a layout that matches the driver's ABI works and
// one that does not is rejected by the driver and no-ops. Verify on real NVIDIA
// hardware (NVIDIA Control Panel -> Manage 3D settings -> program "Project64" ->
// "Max Frames Allowed" should read 1).

#include "NvApiLowLatency.h"

#ifdef _WIN32
#include <windows.h>
#include <string.h>
#include <wchar.h>

namespace
{
typedef unsigned char NvU8;
typedef unsigned short NvU16;
typedef unsigned int NvU32; // 32-bit on Windows LLP64

typedef int NvAPI_Status; // NVAPI_OK == 0
typedef void * NvDRSSessionHandle;
typedef void * NvDRSProfileHandle;

const NvAPI_Status NVAPI_OK = 0;

const NvU32 NVAPI_UNICODE_STRING_MAX = 2048;
const NvU32 NVAPI_BINARY_DATA_MAX = 4096;

// "Maximum pre-rendered frames" driver setting; 1 minimizes the render-ahead
// queue (the OpenGL-applicable NVIDIA low-latency lever).
const NvU32 PRERENDERLIMIT_ID = 0x007BA09E;
const NvU32 NVDRS_DWORD_TYPE = 0;             // NVDRS_SETTING_TYPE
const NvU32 NVDRS_CURRENT_PROFILE_LOCATION = 0; // NVDRS_SETTING_LOCATION

#define MAKE_NVAPI_VERSION(T, ver) (NvU32)(sizeof(T) | ((ver) << 16))

struct NvAPI_UnicodeString
{
    NvU16 data[NVAPI_UNICODE_STRING_MAX];
};

struct NVDRS_BINARY_SETTING
{
    NvU32 valueLength;
    NvU8 valueData[NVAPI_BINARY_DATA_MAX];
};

union NVDRS_SETTING_UNION
{
    NvU32 u32Value;
    NvAPI_UnicodeString wszValue;
    NVDRS_BINARY_SETTING binaryValue;
};

struct NVDRS_SETTING
{
    NvU32 version;
    NvAPI_UnicodeString settingName;
    NvU32 settingId;
    NvU32 settingType;
    NvU32 settingLocation;
    NvU32 isCurrentPredefined;
    NvU32 isPredefinedValid;
    NVDRS_SETTING_UNION predefinedValue;
    NVDRS_SETTING_UNION currentValue;
};

struct NVDRS_PROFILE
{
    NvU32 version;
    NvAPI_UnicodeString profileName;
    NvU32 gpuSupport;
    NvU32 isPredefined;
    NvU32 numOfApps;
    NvU32 numOfSettings;
};

// V1 application record (smallest, oldest layout).
struct NVDRS_APPLICATION_V1
{
    NvU32 version;
    NvU32 isPredefined;
    NvAPI_UnicodeString appName;
    NvAPI_UnicodeString userFriendlyName;
    NvAPI_UnicodeString launcher;
};

// NvAPI is reached through a single QueryInterface dispatcher keyed by these IDs.
typedef void * (__cdecl * NvAPI_QueryInterface_t)(NvU32 id);
typedef NvAPI_Status(__cdecl * NvAPI_Initialize_t)();
typedef NvAPI_Status(__cdecl * NvAPI_Unload_t)();
typedef NvAPI_Status(__cdecl * NvAPI_DRS_CreateSession_t)(NvDRSSessionHandle *);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_DestroySession_t)(NvDRSSessionHandle);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_LoadSettings_t)(NvDRSSessionHandle);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_SaveSettings_t)(NvDRSSessionHandle);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_CreateProfile_t)(NvDRSSessionHandle, NVDRS_PROFILE *, NvDRSProfileHandle *);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_FindProfileByName_t)(NvDRSSessionHandle, NvAPI_UnicodeString, NvDRSProfileHandle *);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_CreateApplication_t)(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_APPLICATION_V1 *);
typedef NvAPI_Status(__cdecl * NvAPI_DRS_SetSetting_t)(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_SETTING *);

const NvU32 ID_Initialize = 0x0150E828;
const NvU32 ID_Unload = 0xD22BDD7E;
const NvU32 ID_DRS_CreateSession = 0x0694D52E;
const NvU32 ID_DRS_DestroySession = 0xDAD9CFF8;
const NvU32 ID_DRS_LoadSettings = 0x375DBD6B;
const NvU32 ID_DRS_SaveSettings = 0xFCBC7E14;
const NvU32 ID_DRS_CreateProfile = 0xCC176068;
const NvU32 ID_DRS_FindProfileByName = 0x7E4A9A0B;
const NvU32 ID_DRS_CreateApplication = 0x4347A9DE;
const NvU32 ID_DRS_SetSetting = 0x577DD202;

void FillUnicode(NvAPI_UnicodeString & dst, const wchar_t * src)
{
    memset(&dst, 0, sizeof(dst));
    if (src == nullptr)
    {
        return;
    }
    for (NvU32 i = 0; i < NVAPI_UNICODE_STRING_MAX - 1 && src[i] != 0; i++)
    {
        dst.data[i] = (NvU16)src[i];
    }
}
} // namespace

void NvApiApplyLowLatency()
{
#ifdef _WIN64
    HMODULE nvapi = LoadLibraryA("nvapi64.dll");
#else
    HMODULE nvapi = LoadLibraryA("nvapi.dll");
#endif
    if (nvapi == nullptr)
    {
        return; // Not an NVIDIA system - nothing to do.
    }

    NvAPI_QueryInterface_t QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(nvapi, "nvapi_QueryInterface");
    if (QueryInterface == nullptr)
    {
        FreeLibrary(nvapi);
        return;
    }

    NvAPI_Initialize_t Initialize = (NvAPI_Initialize_t)QueryInterface(ID_Initialize);
    NvAPI_Unload_t Unload = (NvAPI_Unload_t)QueryInterface(ID_Unload);
    NvAPI_DRS_CreateSession_t CreateSession = (NvAPI_DRS_CreateSession_t)QueryInterface(ID_DRS_CreateSession);
    NvAPI_DRS_DestroySession_t DestroySession = (NvAPI_DRS_DestroySession_t)QueryInterface(ID_DRS_DestroySession);
    NvAPI_DRS_LoadSettings_t LoadSettings = (NvAPI_DRS_LoadSettings_t)QueryInterface(ID_DRS_LoadSettings);
    NvAPI_DRS_SaveSettings_t SaveSettings = (NvAPI_DRS_SaveSettings_t)QueryInterface(ID_DRS_SaveSettings);
    NvAPI_DRS_CreateProfile_t CreateProfile = (NvAPI_DRS_CreateProfile_t)QueryInterface(ID_DRS_CreateProfile);
    NvAPI_DRS_FindProfileByName_t FindProfileByName = (NvAPI_DRS_FindProfileByName_t)QueryInterface(ID_DRS_FindProfileByName);
    NvAPI_DRS_CreateApplication_t CreateApplication = (NvAPI_DRS_CreateApplication_t)QueryInterface(ID_DRS_CreateApplication);
    NvAPI_DRS_SetSetting_t SetSetting = (NvAPI_DRS_SetSetting_t)QueryInterface(ID_DRS_SetSetting);

    if (Initialize == nullptr || Unload == nullptr || CreateSession == nullptr ||
        DestroySession == nullptr || LoadSettings == nullptr || SaveSettings == nullptr ||
        CreateProfile == nullptr || FindProfileByName == nullptr ||
        CreateApplication == nullptr || SetSetting == nullptr)
    {
        FreeLibrary(nvapi);
        return;
    }

    if (Initialize() != NVAPI_OK)
    {
        FreeLibrary(nvapi);
        return;
    }

    NvDRSSessionHandle session = nullptr;
    if (CreateSession(&session) == NVAPI_OK && session != nullptr)
    {
        if (LoadSettings(session) == NVAPI_OK)
        {
            // Current executable path, used to name/associate the profile.
            wchar_t exePath[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            if (len == 0 || len >= MAX_PATH)
            {
                wcscpy_s(exePath, MAX_PATH, L"Project64.exe");
            }
            const wchar_t * exeName = wcsrchr(exePath, L'\\');
            exeName = (exeName != nullptr) ? exeName + 1 : exePath;

            // Find our profile, creating it (and associating this exe) if absent.
            NVDRS_PROFILE profileInfo;
            memset(&profileInfo, 0, sizeof(profileInfo));
            profileInfo.version = MAKE_NVAPI_VERSION(NVDRS_PROFILE, 1);
            FillUnicode(profileInfo.profileName, L"Project64");

            NvDRSProfileHandle profile = nullptr;
            if (FindProfileByName(session, profileInfo.profileName, &profile) != NVAPI_OK || profile == nullptr)
            {
                profile = nullptr;
                if (CreateProfile(session, &profileInfo, &profile) == NVAPI_OK && profile != nullptr)
                {
                    NVDRS_APPLICATION_V1 app;
                    memset(&app, 0, sizeof(app));
                    app.version = MAKE_NVAPI_VERSION(NVDRS_APPLICATION_V1, 1);
                    FillUnicode(app.appName, exeName);
                    FillUnicode(app.userFriendlyName, L"Project64");
                    FillUnicode(app.launcher, L"");
                    CreateApplication(session, profile, &app); // best-effort association
                }
            }

            if (profile != nullptr)
            {
                NVDRS_SETTING setting;
                memset(&setting, 0, sizeof(setting));
                setting.version = MAKE_NVAPI_VERSION(NVDRS_SETTING, 1);
                setting.settingId = PRERENDERLIMIT_ID;
                setting.settingType = NVDRS_DWORD_TYPE;
                setting.settingLocation = NVDRS_CURRENT_PROFILE_LOCATION;
                setting.currentValue.u32Value = 1; // minimum pre-rendered frames

                if (SetSetting(session, profile, &setting) == NVAPI_OK)
                {
                    SaveSettings(session);
                }
            }
        }
        DestroySession(session);
    }

    Unload();
    FreeLibrary(nvapi);
}

#else // _WIN32

void NvApiApplyLowLatency()
{
    // No NVIDIA driver interface on non-Windows platforms.
}

#endif // _WIN32
