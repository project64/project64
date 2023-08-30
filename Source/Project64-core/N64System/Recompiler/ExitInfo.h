#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Recompiler/asmjit.h>

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
    ExitReason_AddressErrorExceptionRead32,
    ExitReason_AddressErrorExceptionRead64,
    ExitReason_IllegalInstruction,
    ExitReason_Exception,
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
    asmjit::Label JumpLabel;
};

typedef std::list<CExitInfo> EXIT_LIST;
