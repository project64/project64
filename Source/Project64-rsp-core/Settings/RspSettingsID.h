#pragma once

enum
{
    Set_BreakOnStart,
    Set_AccurateEmulation,
    Set_CPUCore,
    Set_LogRDP,
    Set_LogX86Code,
    Set_Profiling,
    Set_IndvidualBlock,
    Set_ShowErrors,

    // Compiler settings
    Set_CheckDest,
    Set_Accum,
    Set_Mmx,
    Set_Mmx2,
    Set_Sse,
    Set_Sections,
    Set_ReOrdering,
    Set_GPRConstants,
    Set_Flags,
    Set_AlignVector,

    // Game settings
    Set_JumpTableSize,
    Set_Mfc0Count,
    Set_SemaphoreExit
};