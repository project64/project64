#include "RSPInfo.h"
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/Settings/RspSettingsID.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Settings/Settings.h>

RSP_INFO RSPInfo;

void InitilizeRSP(RSP_INFO & Rsp_Info)
{
    RSPInfo = Rsp_Info;

    AudioHle = Set_AudioHle != 0 ? GetSystemSetting(Set_AudioHle) != 0 : false;
    GraphicsHle = Set_GraphicsHle != 0 ? GetSystemSetting(Set_GraphicsHle) != 0 : true;

    AllocateMemory();
    InitilizeRSPRegisters();
    Build_RSP();
#ifdef GenerateLog
    Start_Log();
#endif
}
