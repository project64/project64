#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>

class CCodeBlock;

enum ExitReason
{
    ExitReason_Normal,
    ExitReason_NormalNoSysCheck,
    ExitReason_DoCPUAction,
    ExitReason_COP1Unuseable,
    ExitReason_DoSysCall,
    ExitReason_Break,
    ExitReason_TLBReadMiss,
    ExitReason_TLBWriteMiss,
    ExitReason_ResetRecompCode,
    ExitReason_ExceptionOverflow,
};

struct CExitInfo
{
    CExitInfo(CCodeBlock & CodeBlock);

    std::string Name;
    uint32_t ID;
    uint32_t TargetPC;
    CRegInfo ExitRegSet;
    ExitReason Reason;
    PIPELINE_STAGE PipelineStage;
    uint32_t * JumpLoc; // 32-bit jump
};

typedef std::list<CExitInfo> EXIT_LIST;
