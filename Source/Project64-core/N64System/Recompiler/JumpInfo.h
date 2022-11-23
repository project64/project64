#pragma once
#include <Project64-core/N64System/Recompiler/ExitInfo.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>

struct CJumpInfo
{
    CJumpInfo(CCodeBlock & CodeBlock);

    uint32_t TargetPC;
    uint32_t JumpPC;
    std::string BranchLabel;
    asmjit::Label LinkLocation;
    asmjit::Label LinkLocation2;
    bool FallThrough;
    bool PermLoop;
    bool DoneDelaySlot;
    CRegInfo RegSet;
    ExitReason Reason;
};
