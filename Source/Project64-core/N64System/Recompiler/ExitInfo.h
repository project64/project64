#pragma once

#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/N64Types.h>

struct CExitInfo
{
    CExitInfo(CCodeBlock & CodeBlock);

    enum EXIT_REASON
    {
        Normal = 0,
        Normal_NoSysCheck = 1,
        DoCPU_Action = 2,
        COP1_Unuseable = 3,
        DoSysCall = 4,
        TLBReadMiss = 5,
        TLBWriteMiss = 6,
        ExitResetRecompCode = 8,
    };

    std::string Name;
    uint32_t ID;
    uint32_t TargetPC;
    CRegInfo ExitRegSet;
    EXIT_REASON reason;
    PIPELINE_STAGE PipelineStage;
    uint32_t * JumpLoc; // 32-bit jump
};

typedef std::list<CExitInfo> EXIT_LIST;
