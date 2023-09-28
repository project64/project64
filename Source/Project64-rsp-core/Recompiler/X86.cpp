#include <stdio.h>

#include "x86.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <Settings/Settings.h>

#pragma warning(disable : 4152) // Non-standard extension, function/data pointer conversion in expression

#define PUTDST8(dest, value)                   \
    (*((uint8_t *)(dest)) = (uint8_t)(value)); \
    dest += 1;
#define PUTDST16(dest, value)                    \
    (*((uint16_t *)(dest)) = (uint16_t)(value)); \
    dest += 2;
#define PUTDST32(dest, value)                    \
    (*((uint32_t *)(dest)) = (uint32_t)(value)); \
    dest += 4;
#define PUTDSTPTR(dest, value)          \
    *(void **)(dest) = (void *)(value); \
    dest += sizeof(void *);

char * x86_Strings[8] = {
    "eax", "ebx", "ecx", "edx",
    "esi", "edi", "ebp", "esp"};

char * x86_ByteStrings[8] = {
    "al", "bl", "cl", "dl",
    "?4", "?5", "?6", "?7"};

char * x86_HalfStrings[8] = {
    "ax", "bx", "cx", "dx",
    "si", "di", "bp", "sp"};

extern bool ConditionalMove;

#define x86Byte_Name(Reg) (x86_ByteStrings[(Reg)])
#define x86Half_Name(Reg) (x86_HalfStrings[(Reg)])

void AdcX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      adc %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x0013; break;
    case x86_EBX: x86Command = 0x0313; break;
    case x86_ECX: x86Command = 0x0113; break;
    case x86_EDX: x86Command = 0x0213; break;
    case x86_ESI: x86Command = 0x0613; break;
    case x86_EDI: x86Command = 0x0713; break;
    case x86_ESP: x86Command = 0x0413; break;
    case x86_EBP: x86Command = 0x0513; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void AdcX86regToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      adc dword ptr [%s], %s", VariableName, x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0511); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D11); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D11); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1511); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3511); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D11); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2511); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D11); break;
    default:
        g_Notify->DisplayError("AddVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AdcX86regHalfToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      adc word ptr [%s], %s", VariableName, x86Half_Name(x86reg));

    PUTDST8(RecompPos, 0x66);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0511); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D11); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D11); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1511); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3511); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D11); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2511); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D11); break;
    default:
        g_Notify->DisplayError("AdcX86regHalfToVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AdcConstToVariable(void * Variable, char * VariableName, uint8_t Constant)
{
    CPU_Message("      adc dword ptr [%s], %Xh", VariableName, Constant);
    PUTDST16(RecompPos, 0x1583);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Constant);
}

void AdcConstToX86reg(uint8_t Constant, int x86reg)
{
    CPU_Message("      adc %s, %Xh", x86_Name(x86reg), Constant);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xD083); break;
    case x86_EBX: PUTDST16(RecompPos, 0xD383); break;
    case x86_ECX: PUTDST16(RecompPos, 0xD183); break;
    case x86_EDX: PUTDST16(RecompPos, 0xD283); break;
    case x86_ESI: PUTDST16(RecompPos, 0xD683); break;
    case x86_EDI: PUTDST16(RecompPos, 0xD783); break;
    case x86_ESP: PUTDST16(RecompPos, 0xD483); break;
    case x86_EBP: PUTDST16(RecompPos, 0xD583); break;
    default:
        g_Notify->DisplayError("AdcConstantToX86reg\nUnknown x86 register");
    }
    PUTDST8(RecompPos, Constant);
}

void AddConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      add dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x0581);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void AddConstToX86Reg(int x86Reg, size_t Const)
{
    const size_t zero_extension_mask = 0x00000000000000007F;
    const size_t sign_extension_mask = ~(zero_extension_mask);
    const size_t extension_from_8bit = Const & sign_extension_mask;

    // TODO:  If 64-bit x86, then what if `Const' upper uint32_t set?
    CPU_Message("      add %s, %Xh", x86_Name(x86Reg), Const);
    if (extension_from_8bit != 0 && extension_from_8bit != sign_extension_mask)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xC081); break;
        case x86_EBX: PUTDST16(RecompPos, 0xC381); break;
        case x86_ECX: PUTDST16(RecompPos, 0xC181); break;
        case x86_EDX: PUTDST16(RecompPos, 0xC281); break;
        case x86_ESI: PUTDST16(RecompPos, 0xC681); break;
        case x86_EDI: PUTDST16(RecompPos, 0xC781); break;
        case x86_ESP: PUTDST16(RecompPos, 0xC481); break;
        case x86_EBP: PUTDST16(RecompPos, 0xC581); break;
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xC083); break;
        case x86_EBX: PUTDST16(RecompPos, 0xC383); break;
        case x86_ECX: PUTDST16(RecompPos, 0xC183); break;
        case x86_EDX: PUTDST16(RecompPos, 0xC283); break;
        case x86_ESI: PUTDST16(RecompPos, 0xC683); break;
        case x86_EDI: PUTDST16(RecompPos, 0xC783); break;
        case x86_ESP: PUTDST16(RecompPos, 0xC483); break;
        case x86_EBP: PUTDST16(RecompPos, 0xC583); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void AdcConstHalfToVariable(void * Variable, char * VariableName, uint8_t Constant)
{
    CPU_Message("      adc word ptr [%s], %Xh", VariableName, Constant);

    PUTDST8(RecompPos, 0x66);
    PUTDST8(RecompPos, 0x83);
    PUTDST8(RecompPos, 0x15);

    PUTDSTPTR(RecompPos, Variable);

    PUTDST8(RecompPos, Constant);
}

void AddVariableToX86reg(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      add %s, dword ptr [%s]", x86_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0503); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D03); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D03); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1503); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3503); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D03); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2503); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D03); break;
    default:
        g_Notify->DisplayError("AddVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AddX86regToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      add dword ptr [%s], %s", VariableName, x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0501); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D01); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D01); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1501); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3501); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D01); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2501); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D01); break;
    default:
        g_Notify->DisplayError("AddVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AddX86regHalfToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      add word ptr [%s], %s", VariableName, x86Half_Name(x86reg));

    PUTDST8(RecompPos, 0x66);

    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0501); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D01); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D01); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1501); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3501); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D01); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2501); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D01); break;
    default:
        g_Notify->DisplayError("AddVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AddX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      add %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x0003; break;
    case x86_EBX: x86Command = 0x0303; break;
    case x86_ECX: x86Command = 0x0103; break;
    case x86_EDX: x86Command = 0x0203; break;
    case x86_ESI: x86Command = 0x0603; break;
    case x86_EDI: x86Command = 0x0703; break;
    case x86_ESP: x86Command = 0x0403; break;
    case x86_EBP: x86Command = 0x0503; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void AndConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      and dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x2581);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void AndConstToX86Reg(int x86Reg, uint32_t Const)
{
    CPU_Message("      and %s, %Xh", x86_Name(x86Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xE081); break;
        case x86_EBX: PUTDST16(RecompPos, 0xE381); break;
        case x86_ECX: PUTDST16(RecompPos, 0xE181); break;
        case x86_EDX: PUTDST16(RecompPos, 0xE281); break;
        case x86_ESI: PUTDST16(RecompPos, 0xE681); break;
        case x86_EDI: PUTDST16(RecompPos, 0xE781); break;
        case x86_ESP: PUTDST16(RecompPos, 0xE481); break;
        case x86_EBP: PUTDST16(RecompPos, 0xE581); break;
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xE083); break;
        case x86_EBX: PUTDST16(RecompPos, 0xE383); break;
        case x86_ECX: PUTDST16(RecompPos, 0xE183); break;
        case x86_EDX: PUTDST16(RecompPos, 0xE283); break;
        case x86_ESI: PUTDST16(RecompPos, 0xE683); break;
        case x86_EDI: PUTDST16(RecompPos, 0xE783); break;
        case x86_ESP: PUTDST16(RecompPos, 0xE483); break;
        case x86_EBP: PUTDST16(RecompPos, 0xE583); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void AndVariableToX86Reg(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      and %s, dword ptr [%s]", x86_Name(x86Reg), VariableName);
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0523); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D23); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D23); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1523); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3523); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D23); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2523); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D23); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AndVariableToX86regHalf(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      and %s, word ptr [%s]", x86Half_Name(x86Reg), VariableName);
    PUTDST8(RecompPos, 0x66);
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0523); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D23); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D23); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1523); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3523); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D23); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2523); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D23); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AndX86RegToVariable(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      and dword ptr [%s], %s", VariableName, x86_Name(x86Reg));
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0521); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D21); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D21); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1521); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3521); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D21); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2521); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D21); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void AndX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      and %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Destination)
    {
    case x86_EAX: x86Command = 0x0021; break;
    case x86_EBX: x86Command = 0x0321; break;
    case x86_ECX: x86Command = 0x0121; break;
    case x86_EDX: x86Command = 0x0221; break;
    case x86_ESI: x86Command = 0x0621; break;
    case x86_EDI: x86Command = 0x0721; break;
    case x86_ESP: x86Command = 0x0421; break;
    case x86_EBP: x86Command = 0x0521; break;
    }
    switch (Source)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void AndX86RegHalfToX86RegHalf(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      and %s, %s", x86Half_Name(Destination), x86Half_Name(Source));
    PUTDST8(RecompPos, 0x66);

    switch (Destination)
    {
    case x86_EAX: x86Command = 0x0021; break;
    case x86_EBX: x86Command = 0x0321; break;
    case x86_ECX: x86Command = 0x0121; break;
    case x86_EDX: x86Command = 0x0221; break;
    case x86_ESI: x86Command = 0x0621; break;
    case x86_EDI: x86Command = 0x0721; break;
    case x86_ESP: x86Command = 0x0421; break;
    case x86_EBP: x86Command = 0x0521; break;
    }
    switch (Source)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void BreakPointNotification(const char * const FileName, const int LineNumber)
{
    g_Notify->DisplayError(stdstr_f("Break point found\nFile: %s\nLine: %d", FileName, LineNumber).c_str());
}

void X86BreakPoint(const char * FileName, int LineNumber)
{
    Pushad();
    PushImm32("LineNumber", LineNumber);
#if defined(_M_IX86)
    PushImm32("FileName", (uint32_t)FileName);
#else
    g_Notify->DisplayError("PushImm64\nUnimplemented.");
#endif
    Call_Direct(BreakPointNotification, "BreakPointNotification");
    AddConstToX86Reg(x86_ESP, 8);
    Popad();
    CPU_Message("      int 3");
    PUTDST8(RecompPos, 0xCC);
}

void BsrX86RegToX86Reg(int Destination, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      bsr %s, %s", x86_Name(Destination), x86_Name(Source));

    PUTDST16(RecompPos, 0xBD0F);
    switch (Source)
    {
    case x86_EAX: x86Command = 0x00; break;
    case x86_EBX: x86Command = 0x03; break;
    case x86_ECX: x86Command = 0x01; break;
    case x86_EDX: x86Command = 0x02; break;
    case x86_ESI: x86Command = 0x06; break;
    case x86_EDI: x86Command = 0x07; break;
    case x86_ESP: x86Command = 0x04; break;
    case x86_EBP: x86Command = 0x05; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC0; break;
    case x86_EBX: x86Command += 0xD8; break;
    case x86_ECX: x86Command += 0xC8; break;
    case x86_EDX: x86Command += 0xD0; break;
    case x86_ESI: x86Command += 0xF0; break;
    case x86_EDI: x86Command += 0xF8; break;
    case x86_ESP: x86Command += 0xE0; break;
    case x86_EBP: x86Command += 0xE8; break;
    }
    PUTDST8(RecompPos, x86Command);
}

void Call_Direct(void * FunctAddress, char * FunctName)
{
    CPU_Message("      call offset %s", FunctName);
    PUTDST8(RecompPos, 0xE8);
    PUTDSTPTR(RecompPos, (size_t)FunctAddress - (size_t)RecompPos - sizeof(void *));
}

void Call_Indirect(void * FunctAddress, char * FunctName)
{
    CPU_Message("      call [%s]", FunctName);
    PUTDST16(RecompPos, 0x15FF);
    PUTDSTPTR(RecompPos, FunctAddress);
}

void CondMoveEqual(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("   [*]cmove %s, %s", x86_Name(Destination), x86_Name(Source));

        JneLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmove %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x440F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CondMoveNotEqual(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("   [*]cmovne %s, %s", x86_Name(Destination), x86_Name(Source));

        JeLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmovne %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x450F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CondMoveGreater(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("    [*]cmovg %s, %s", x86_Name(Destination), x86_Name(Source));

        JleLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmovg %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x4F0F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CondMoveGreaterEqual(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("   [*]cmovge %s, %s", x86_Name(Destination), x86_Name(Source));

        JlLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmovge %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x4D0F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CondMoveLess(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("   [*]cmovl %s, %s", x86_Name(Destination), x86_Name(Source));

        JgeLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmovl %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x4C0F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CondMoveLessEqual(int Destination, int Source)
{
    if (ConditionalMove == false)
    {
        uint8_t * Jump;
        CPU_Message("   [*]cmovle %s, %s", x86_Name(Destination), x86_Name(Source));

        JgLabel8("label", 0);
        Jump = RecompPos - 1;
        MoveX86RegToX86Reg(Source, Destination);
        CPU_Message("     label:");
        x86_SetBranch8b(Jump, RecompPos);
    }
    else
    {
        uint8_t x86Command = 0;
        CPU_Message("      cmovle %s, %s", x86_Name(Destination), x86_Name(Source));

        PUTDST16(RecompPos, 0x4E0F);

        switch (Source)
        {
        case x86_EAX: x86Command = 0x00; break;
        case x86_EBX: x86Command = 0x03; break;
        case x86_ECX: x86Command = 0x01; break;
        case x86_EDX: x86Command = 0x02; break;
        case x86_ESI: x86Command = 0x06; break;
        case x86_EDI: x86Command = 0x07; break;
        case x86_ESP: x86Command = 0x04; break;
        case x86_EBP: x86Command = 0x05; break;
        }

        switch (Destination)
        {
        case x86_EAX: x86Command += 0xC0; break;
        case x86_EBX: x86Command += 0xD8; break;
        case x86_ECX: x86Command += 0xC8; break;
        case x86_EDX: x86Command += 0xD0; break;
        case x86_ESI: x86Command += 0xF0; break;
        case x86_EDI: x86Command += 0xF8; break;
        case x86_ESP: x86Command += 0xE0; break;
        case x86_EBP: x86Command += 0xE8; break;
        }

        PUTDST8(RecompPos, x86Command);
    }
}

void CompConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      cmp dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x3D81);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void CompConstHalfToVariable(uint16_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      cmp word ptr [%s], 0x%X", VariableName, Const);
    PUTDST8(RecompPos, 0x66);
    PUTDST8(RecompPos, 0x81);
    PUTDST8(RecompPos, 0x3D);

    PUTDSTPTR(RecompPos, Variable);
    PUTDST16(RecompPos, Const);
}

void CompConstToX86reg(int x86Reg, uint32_t Const)
{
    CPU_Message("      cmp %s, %Xh", x86_Name(x86Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xF881); break;
        case x86_EBX: PUTDST16(RecompPos, 0xFB81); break;
        case x86_ECX: PUTDST16(RecompPos, 0xF981); break;
        case x86_EDX: PUTDST16(RecompPos, 0xFA81); break;
        case x86_ESI: PUTDST16(RecompPos, 0xFE81); break;
        case x86_EDI: PUTDST16(RecompPos, 0xFF81); break;
        case x86_ESP: PUTDST16(RecompPos, 0xFC81); break;
        case x86_EBP: PUTDST16(RecompPos, 0xFD81); break;
        default:
            g_Notify->DisplayError("CompConstToX86reg\nUnknown x86 register");
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xF883); break;
        case x86_EBX: PUTDST16(RecompPos, 0xFB83); break;
        case x86_ECX: PUTDST16(RecompPos, 0xF983); break;
        case x86_EDX: PUTDST16(RecompPos, 0xFA83); break;
        case x86_ESI: PUTDST16(RecompPos, 0xFE83); break;
        case x86_EDI: PUTDST16(RecompPos, 0xFF83); break;
        case x86_ESP: PUTDST16(RecompPos, 0xFC83); break;
        case x86_EBP: PUTDST16(RecompPos, 0xFD83); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void CompX86regToVariable(int x86Reg, void * Variable, char * VariableName)
{
    CPU_Message("      cmp %s, dword ptr [%s]", x86_Name(x86Reg), VariableName);
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x053B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D3B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D3B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x153B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x353B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D3B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x253B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D3B); break;
    default:
        g_Notify->DisplayError("Unknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void CompVariableToX86reg(int x86Reg, void * Variable, char * VariableName)
{
    CPU_Message("      cmp dword ptr [%s], %s", VariableName, x86_Name(x86Reg));
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0539); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D39); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D39); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1539); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3539); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D39); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2539); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D39); break;
    default:
        g_Notify->DisplayError("Unknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void CompX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      cmp %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x003B; break;
    case x86_EBX: x86Command = 0x033B; break;
    case x86_ECX: x86Command = 0x013B; break;
    case x86_EDX: x86Command = 0x023B; break;
    case x86_ESI: x86Command = 0x063B; break;
    case x86_EDI: x86Command = 0x073B; break;
    case x86_ESP: x86Command = 0x043B; break;
    case x86_EBP: x86Command = 0x053B; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void Cwd(void)
{
    CPU_Message("      cwd");
    PUTDST16(RecompPos, 0x9966);
}

void Cwde(void)
{
    CPU_Message("      cwde");
    PUTDST8(RecompPos, 0x98);
}

void DecX86reg(int x86reg)
{
    CPU_Message("      dec %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xC8FF); break;
    case x86_EBX: PUTDST16(RecompPos, 0xCBFF); break;
    case x86_ECX: PUTDST16(RecompPos, 0xC9FF); break;
    case x86_EDX: PUTDST16(RecompPos, 0xCAFF); break;
    case x86_ESI: PUTDST16(RecompPos, 0xCEFF); break;
    case x86_EDI: PUTDST16(RecompPos, 0xCFFF); break;
    case x86_ESP: PUTDST8(RecompPos, 0x4C); break;
    case x86_EBP: PUTDST8(RecompPos, 0x4D); break;
    default:
        g_Notify->DisplayError("DecX86reg\nUnknown x86 register");
    }
}

void DivX86reg(int x86reg)
{
    CPU_Message("      div %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EBX: PUTDST16(RecompPos, 0xf3F7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xf1F7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xf2F7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xf6F7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xf7F7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xf4F7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xf5F7); break;
    default:
        g_Notify->DisplayError("divX86reg\nUnknown x86 register");
    }
}

void idivX86reg(int x86reg)
{
    CPU_Message("      idiv %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EBX: PUTDST16(RecompPos, 0xfbF7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xf9F7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xfaF7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xfeF7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xffF7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xfcF7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xfdF7); break;
    default:
        g_Notify->DisplayError("idivX86reg\nUnknown x86 register");
    }
}

void imulX86reg(int x86reg)
{
    CPU_Message("      imul %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE8F7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xEBF7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE9F7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xEAF7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xEEF7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xEFF7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xECF7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xEDF7); break;
    default:
        g_Notify->DisplayError("imulX86reg\nUnknown x86 register");
    }
}

void ImulX86RegToX86Reg(int Destination, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      imul %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x00; break;
    case x86_EBX: x86Command = 0x03; break;
    case x86_ECX: x86Command = 0x01; break;
    case x86_EDX: x86Command = 0x02; break;
    case x86_ESI: x86Command = 0x06; break;
    case x86_EDI: x86Command = 0x07; break;
    case x86_ESP: x86Command = 0x04; break;
    case x86_EBP: x86Command = 0x05; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC0; break;
    case x86_EBX: x86Command += 0xD8; break;
    case x86_ECX: x86Command += 0xC8; break;
    case x86_EDX: x86Command += 0xD0; break;
    case x86_ESI: x86Command += 0xF0; break;
    case x86_EDI: x86Command += 0xF8; break;
    case x86_ESP: x86Command += 0xE0; break;
    case x86_EBP: x86Command += 0xE8; break;
    }

    PUTDST16(RecompPos, 0xAF0F);
    PUTDST8(RecompPos, x86Command);
}

void IncX86reg(int x86Reg)
{
    CPU_Message("      inc %s", x86_Name(x86Reg));
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xC0FF); break;
    case x86_EBX: PUTDST16(RecompPos, 0xC3FF); break;
    case x86_ECX: PUTDST16(RecompPos, 0xC1FF); break;
    case x86_EDX: PUTDST16(RecompPos, 0xC2FF); break;
    case x86_ESI: PUTDST16(RecompPos, 0xC6FF); break;
    case x86_EDI: PUTDST16(RecompPos, 0xC7FF); break;
    case x86_ESP: PUTDST8(RecompPos, 0x44); break;
    case x86_EBP: PUTDST8(RecompPos, 0x45); break;
    default:
        g_Notify->DisplayError("IncX86reg\nUnknown x86 register");
    }
}

void JaeLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jae $%s", Label);
    PUTDST16(RecompPos, 0x830F);
    PUTDST32(RecompPos, Value);
}

void JaLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      ja $%s", Label);
    PUTDST8(RecompPos, 0x77);
    PUTDST8(RecompPos, Value);
}

void JaLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      ja $%s", Label);
    PUTDST16(RecompPos, 0x870F);
    PUTDST32(RecompPos, Value);
}

void JbLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jb $%s", Label);
    PUTDST8(RecompPos, 0x72);
    PUTDST8(RecompPos, Value);
}

void JbLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jb $%s", Label);
    PUTDST16(RecompPos, 0x820F);
    PUTDST32(RecompPos, Value);
}

void JeLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      je $%s", Label);
    PUTDST8(RecompPos, 0x74);
    PUTDST8(RecompPos, Value);
}

void JeLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      je $%s", Label);
    PUTDST16(RecompPos, 0x840F);
    PUTDST32(RecompPos, Value);
}

void JgeLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jge $%s", Label);
    PUTDST8(RecompPos, 0x7D);
    PUTDST8(RecompPos, Value);
}

void JgeLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jge $%s", Label);
    PUTDST16(RecompPos, 0x8D0F);
    PUTDST32(RecompPos, Value);
}

void JgLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jg $%s", Label);
    PUTDST8(RecompPos, 0x7F);
    PUTDST8(RecompPos, Value);
}

void JgLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jg $%s", Label);
    PUTDST16(RecompPos, 0x8F0F);
    PUTDST32(RecompPos, Value);
}

void JleLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jle $%s", Label);
    PUTDST8(RecompPos, 0x7E);
    PUTDST8(RecompPos, Value);
}

void JleLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jle $%s", Label);
    PUTDST16(RecompPos, 0x8E0F);
    PUTDST32(RecompPos, Value);
}

void JlLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jl $%s", Label);
    PUTDST8(RecompPos, 0x7C);
    PUTDST8(RecompPos, Value);
}

void JlLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jl $%s", Label);
    PUTDST16(RecompPos, 0x8C0F);
    PUTDST32(RecompPos, Value);
}

void JumpX86Reg(int x86reg)
{
    CPU_Message("      jmp %s", x86_Name(x86reg));

    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xe0ff); break;
    case x86_EBX: PUTDST16(RecompPos, 0xe3ff); break;
    case x86_ECX: PUTDST16(RecompPos, 0xe1ff); break;
    case x86_EDX: PUTDST16(RecompPos, 0xe2ff); break;
    case x86_ESI: PUTDST16(RecompPos, 0xe6ff); break;
    case x86_EDI: PUTDST16(RecompPos, 0xe7ff); break;
    default: g_Notify->DisplayError("JumpX86Reg: Unknown register");
    }
}

void JmpLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jmp $%s", Label);
    PUTDST8(RecompPos, 0xEB);
    PUTDST8(RecompPos, Value);
}

void JmpLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jmp $%s", Label);
    PUTDST8(RecompPos, 0xE9);
    PUTDST32(RecompPos, Value);
}

void JneLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jne $%s", Label);
    PUTDST8(RecompPos, 0x75);
    PUTDST8(RecompPos, Value);
}

void JneLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jne $%s", Label);
    PUTDST16(RecompPos, 0x850F);
    PUTDST32(RecompPos, Value);
}

void JnsLabel8(char * Label, uint8_t Value)
{
    CPU_Message("      jns $%s", Label);
    PUTDST8(RecompPos, 0x79);
    PUTDST8(RecompPos, Value);
}

void JnsLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      jns $%s", Label);
    PUTDST16(RecompPos, 0x890F);
    PUTDST32(RecompPos, Value);
}

void JsLabel32(char * Label, uint32_t Value)
{
    CPU_Message("      js $%s", Label);
    PUTDST16(RecompPos, 0x880F);
    PUTDST32(RecompPos, Value);
}

// TODO: Rewrite this?
// NOTE: This op can get really complex with muls
// If we need this, rewrite it into 1 function

void LeaSourceAndOffset(int x86DestReg, int x86SourceReg, size_t offset)
{
    uint16_t x86Command = 0;

    CPU_Message("      lea %s, [%s + %0Xh]", x86_Name(x86DestReg), x86_Name(x86SourceReg), offset);
    switch (x86DestReg)
    {
    case x86_EAX: x86Command = 0x808D; break;
    case x86_EBX: x86Command = 0x988D; break;
    case x86_ECX: x86Command = 0x888D; break;
    case x86_EDX: x86Command = 0x908D; break;
    case x86_ESI: x86Command = 0xB08D; break;
    case x86_EDI: x86Command = 0xB88D; break;
    case x86_ESP: x86Command = 0xA08D; break;
    case x86_EBP: x86Command = 0xA88D; break;
    default:
        g_Notify->DisplayError("LeaSourceAndOffset\nUnknown x86 register");
    }
    switch (x86SourceReg)
    {
    case x86_EAX: x86Command += 0x0000; break;
    case x86_EBX: x86Command += 0x0300; break;
    case x86_ECX: x86Command += 0x0100; break;
    case x86_EDX: x86Command += 0x0200; break;
    case x86_ESI: x86Command += 0x0600; break;
    case x86_EDI: x86Command += 0x0700; break;
    case x86_ESP: x86Command += 0x0400; break;
    case x86_EBP: x86Command += 0x0500; break;
    default:
        g_Notify->DisplayError("LeaSourceAndOffset\nUnknown x86 register");
    }

    // TODO: Check high uint32_t of offset for 64-bit x86
    if ((offset & 0x00000000FFFFFF80) != 0 && (offset & ~0x7F) != ~0x7F)
    {
        PUTDST16(RecompPos, x86Command);
        PUTDST32(RecompPos, offset);
    }
    else
    {
        PUTDST16(RecompPos, (x86Command & 0x7FFF) | 0x4000);
        PUTDST8(RecompPos, offset);
    }
}

void MoveConstByteToN64Mem(uint8_t Const, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov byte ptr [%s+Dmem], %Xh", x86_Name(AddrReg), Const);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x80C6; break;
    case x86_EBX: x86Command = 0x83C6; break;
    case x86_ECX: x86Command = 0x81C6; break;
    case x86_EDX: x86Command = 0x82C6; break;
    case x86_ESI: x86Command = 0x86C6; break;
    case x86_EDI: x86Command = 0x87C6; break;
    case x86_ESP: x86Command = 0x84C6; break;
    case x86_EBP: x86Command = 0x85C6; break;
    default:
        g_Notify->DisplayError("MoveConstByteToN64Mem\nUnknown x86 register");
    }

    PUTDST16(RecompPos, x86Command);
    PUTDST32(RecompPos, RSPInfo.DMEM);
    PUTDST8(RecompPos, Const);
}

void MoveConstHalfToN64Mem(uint16_t Const, int AddrReg)
{
    uint8_t x86Command = 0;

    CPU_Message("      mov word ptr [%s+Dmem], %Xh", x86_Name(AddrReg), Const);
    PUTDST16(RecompPos, 0xC766);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x80; break;
    case x86_EBX: x86Command = 0x83; break;
    case x86_ECX: x86Command = 0x81; break;
    case x86_EDX: x86Command = 0x82; break;
    case x86_ESI: x86Command = 0x86; break;
    case x86_EDI: x86Command = 0x87; break;
    case x86_ESP: x86Command = 0x84; break;
    case x86_EBP: x86Command = 0x85; break;
    default:
        g_Notify->DisplayError("MoveConstHalfToN64Mem\nUnknown x86 register");
    }

    PUTDST8(RecompPos, x86Command);
    PUTDST32(RecompPos, RSPInfo.DMEM);
    PUTDST16(RecompPos, Const);
}

void MoveConstByteToVariable(uint8_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      mov byte ptr [%s], %Xh", VariableName, Const);
    PUTDST16(RecompPos, 0x05C6);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Const);
}

void MoveConstHalfToVariable(uint16_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      mov word ptr [%s], %Xh", VariableName, Const);
    PUTDST8(RecompPos, 0x66);
    PUTDST16(RecompPos, 0x05C7);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST16(RecompPos, Const);
}

void MoveConstToN64Mem(uint32_t Const, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov dword ptr [%s+Dmem], %Xh", x86_Name(AddrReg), Const);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x80C7; break;
    case x86_EBX: x86Command = 0x83C7; break;
    case x86_ECX: x86Command = 0x81C7; break;
    case x86_EDX: x86Command = 0x82C7; break;
    case x86_ESI: x86Command = 0x86C7; break;
    case x86_EDI: x86Command = 0x87C7; break;
    case x86_ESP: x86Command = 0x84C7; break;
    case x86_EBP: x86Command = 0x85C7; break;
    default:
        g_Notify->DisplayError("MoveConstToN64Mem\nUnknown x86 register");
    }

    PUTDST16(RecompPos, x86Command);
    PUTDST32(RecompPos, RSPInfo.DMEM);
    PUTDST32(RecompPos, Const);
}

void MoveConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      mov dword ptr [%s], %Xh", VariableName, Const);
    PUTDST16(RecompPos, 0x05C7);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void MoveConstToX86reg(uint32_t Const, int x86reg)
{
    CPU_Message("      mov %s, %Xh", x86_Name(x86reg), Const);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xC0C7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xC3C7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xC1C7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xC2C7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xC6C7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xC7C7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xC4C7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xC5C7); break;
    default:
        g_Notify->DisplayError("MoveConstToX86reg\nUnknown x86 register");
    }
    PUTDST32(RecompPos, Const);
}

void MoveOffsetToX86reg(size_t Const, char * VariableName, int x86reg)
{
    CPU_Message("      mov %s, offset %s", x86_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xC0C7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xC3C7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xC1C7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xC2C7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xC6C7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xC7C7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xC4C7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xC5C7); break;
    default:
        g_Notify->DisplayError("MoveOffsetToX86reg\nUnknown x86 register");
    }
    PUTDST32(RecompPos, Const);
}

void MoveX86regPointerToX86regByte(int Destination, int AddrReg)
{
    uint8_t x86Command = 0;

    CPU_Message("      mov %s, byte ptr [%s]", x86Byte_Name(Destination), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00; break;
    case x86_EBX: x86Command = 0x03; break;
    case x86_ECX: x86Command = 0x01; break;
    case x86_EDX: x86Command = 0x02; break;
    case x86_ESI: x86Command = 0x06; break;
    case x86_EDI: x86Command = 0x07; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86regByte\nUnknown x86 register");
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0x00; break;
    case x86_EBX: x86Command += 0x18; break;
    case x86_ECX: x86Command += 0x08; break;
    case x86_EDX: x86Command += 0x10; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86regByte\nUnknown x86 register");
    }

    PUTDST8(RecompPos, 0x8A);
    PUTDST8(RecompPos, x86Command);
}

void MoveX86regPointerToX86regHalf(int Destination, int AddrReg)
{
    unsigned char x86Amb = 0;

    CPU_Message("      mov %s, word ptr [%s]", x86Half_Name(Destination), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86regHalf\nUnknown x86 register");
    }

    switch (Destination)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86regHalf\nUnknown x86 register");
    }

    PUTDST16(RecompPos, 0x8B66);
    PUTDST8(RecompPos, x86Amb);
}

void MoveX86regPointerToX86reg(int Destination, int AddrReg)
{
    uint8_t x86Amb = 0;
    CPU_Message("      mov %s, dword ptr [%s]", x86_Name(Destination), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: x86Amb = 0x04; break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86reg\nUnknown x86 register");
    }

    switch (Destination)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regPointerToX86reg\nUnknown x86 register");
    }

    PUTDST8(RecompPos, 0x8B);
    PUTDST8(RecompPos, x86Amb);
}

void MoveX86regByteToX86regPointer(int Source, int AddrReg)
{
    uint8_t x86Amb = 0;
    CPU_Message("      mov byte ptr [%s], %s", x86_Name(AddrReg), x86Byte_Name(Source));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: x86Amb = 0x04; break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    PUTDST8(RecompPos, 0x88);
    PUTDST8(RecompPos, x86Amb);
}

void MoveX86regHalfToX86regPointer(int Source, int AddrReg)
{
    uint8_t x86Amb = 0;

    CPU_Message("      mov word ptr [%s], %s", x86_Name(AddrReg), x86Half_Name(Source));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: x86Amb = 0x04; break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    PUTDST16(RecompPos, 0x8966);
    PUTDST8(RecompPos, x86Amb);
}

void MoveX86regHalfToX86regPointerDisp(int Source, int AddrReg, uint8_t Disp)
{
    uint8_t x86Amb = 0;

    CPU_Message("      mov word ptr [%s+%X], %s", x86_Name(AddrReg), Disp, x86Half_Name(Source));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: x86Amb = 0x04; break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regBytePointer\nUnknown x86 register");
    }

    PUTDST16(RecompPos, 0x8966);
    PUTDST8(RecompPos, x86Amb | 0x40);
    PUTDST8(RecompPos, Disp);
}

void MoveX86regToX86regPointer(int Source, int AddrReg)
{
    uint8_t x86Amb = 0;
    CPU_Message("      mov dword ptr [%s], %s", x86_Name(AddrReg), x86_Name(Source));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: x86Amb = 0x04; break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regToX86regPointer\nUnknown x86 register");
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regToX86regPointer\nUnknown x86 register");
    }

    PUTDST8(RecompPos, 0x89);
    PUTDST8(RecompPos, x86Amb);
}

void MoveX86RegToX86regPointerDisp(int Source, int AddrReg, uint8_t Disp)
{
    uint8_t x86Amb = 0;
    CPU_Message("      mov dword ptr [%s+%X], %s", x86_Name(AddrReg), Disp, x86_Name(Source));

    switch (AddrReg)
    {
    case x86_EAX: x86Amb = 0x00; break;
    case x86_EBX: x86Amb = 0x03; break;
    case x86_ECX: x86Amb = 0x01; break;
    case x86_EDX: x86Amb = 0x02; break;
    case x86_ESI: x86Amb = 0x06; break;
    case x86_EDI: x86Amb = 0x07; break;
    case x86_ESP: g_Notify->DisplayError("MoveX86RegToX86regPointerDisp: ESP is invalid"); break;
    case x86_EBP: x86Amb = 0x05; break;
    default: g_Notify->DisplayError("MoveX86regToX86regPointer\nUnknown x86 register");
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    default: g_Notify->DisplayError("MoveX86regToX86regPointer\nUnknown x86 register");
    }

    PUTDST8(RecompPos, 0x89);
    PUTDST8(RecompPos, x86Amb | 0x40);
    PUTDST8(RecompPos, Disp);
}

void MoveN64MemDispToX86reg(int x86reg, int AddrReg, uint8_t Disp)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov %s, dword ptr [%s+Dmem+%Xh]", x86_Name(x86reg), x86_Name(AddrReg), Disp);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM + Disp);
}

void MoveN64MemToX86reg(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov %s, dword ptr [%s+Dmem]", x86_Name(x86reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveN64MemToX86regByte(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov %s, byte ptr [%s+Dmem]", x86Byte_Name(x86reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008A; break;
    case x86_EBX: x86Command = 0x038A; break;
    case x86_ECX: x86Command = 0x018A; break;
    case x86_EDX: x86Command = 0x028A; break;
    case x86_ESI: x86Command = 0x068A; break;
    case x86_EDI: x86Command = 0x078A; break;
    case x86_ESP: x86Command = 0x048A; break;
    case x86_EBP: x86Command = 0x058A; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    default:
        g_Notify->DisplayError("MoveN64MemToX86regByte\nInvalid x86 register");
        break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveN64MemToX86regHalf(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov %s, word ptr [%s+Dmem]", x86Half_Name(x86reg), x86_Name(AddrReg));

    PUTDST8(RecompPos, 0x66);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveX86regByteToN64Mem(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov byte ptr [%s+Dmem], %s", x86_Name(AddrReg), x86Byte_Name(x86reg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0088; break;
    case x86_EBX: x86Command = 0x0388; break;
    case x86_ECX: x86Command = 0x0188; break;
    case x86_EDX: x86Command = 0x0288; break;
    case x86_ESI: x86Command = 0x0688; break;
    case x86_EDI: x86Command = 0x0788; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveX86regHalfToN64Mem(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov word ptr [%s+Dmem], %s", x86_Name(AddrReg), x86Half_Name(x86reg));
    PUTDST8(RecompPos, 0x66);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveX86regToN64Mem(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov dword ptr [%s+N64mem], %s", x86_Name(AddrReg), x86_Name(x86reg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveX86regToN64MemDisp(int x86reg, int AddrReg, uint8_t Disp)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov dword ptr [%s+N64mem+%d], %s", x86_Name(AddrReg), Disp, x86_Name(x86reg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM + Disp);
}

void MoveVariableToX86reg(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      mov %s, dword ptr [%s]", x86_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x058B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D8B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D8B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x158B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x358B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D8B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x258B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D8B); break;
    default: g_Notify->DisplayError("MoveVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveVariableToX86regByte(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      mov %s, byte ptr [%s]", x86Byte_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x058A); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D8A); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D8A); break;
    case x86_EDX: PUTDST16(RecompPos, 0x158A); break;
    default: g_Notify->DisplayError("MoveVariableToX86regByte\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveVariableToX86regHalf(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      mov %s, word ptr [%s]", x86Half_Name(x86reg), VariableName);
    PUTDST8(RecompPos, 0x66);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x058B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D8B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D8B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x158B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x358B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D8B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x258B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D8B); break;
    default: g_Notify->DisplayError("MoveVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveX86regByteToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      mov byte ptr [%s], %s", VariableName, x86Byte_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0588); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D88); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D88); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1588); break;
    default:
        g_Notify->DisplayError("MoveX86regByteToVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveX86regHalfToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      mov word ptr [%s], %s", VariableName, x86Half_Name(x86reg));
    PUTDST8(RecompPos, 0x66);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0589); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D89); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D89); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1589); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3589); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D89); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2589); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D89); break;
    default:
        g_Notify->DisplayError("MoveX86regHalfToVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveX86regToVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      mov dword ptr [%s], %s", VariableName, x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0589); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D89); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D89); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1589); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3589); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D89); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2589); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D89); break;
    default:
        g_Notify->DisplayError("MoveX86regToVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveX86RegToX86Reg(int Source, int Destination)
{
    uint16_t x86Command = 0;

    CPU_Message("      mov %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Destination)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    }

    switch (Source)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void MoveSxX86RegHalfToX86Reg(int Source, int Destination)
{
    uint16_t x86Command = 0;

    CPU_Message("      movsx %s, %s", x86_Name(Destination), x86Half_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x00BF; break;
    case x86_EBX: x86Command = 0x03BF; break;
    case x86_ECX: x86Command = 0x01BF; break;
    case x86_EDX: x86Command = 0x02BF; break;
    case x86_ESI: x86Command = 0x06BF; break;
    case x86_EDI: x86Command = 0x07BF; break;
    case x86_ESP: x86Command = 0x04BF; break;
    case x86_EBP: x86Command = 0x05BF; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
}

void MoveSxX86RegPtrDispToX86RegHalf(int AddrReg, uint8_t Disp, int Destination)
{
    uint8_t x86Command = 0;

    CPU_Message("      movsx %s, [%s+%X]", x86_Name(Destination), x86_Name(AddrReg), Disp);

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00; break;
    case x86_EBX: x86Command = 0x03; break;
    case x86_ECX: x86Command = 0x01; break;
    case x86_EDX: x86Command = 0x02; break;
    case x86_ESI: x86Command = 0x06; break;
    case x86_EDI: x86Command = 0x07; break;
    case x86_ESP: x86Command = 0x04; break;
    case x86_EBP: x86Command = 0x05; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0x40; break;
    case x86_EBX: x86Command += 0x58; break;
    case x86_ECX: x86Command += 0x48; break;
    case x86_EDX: x86Command += 0x50; break;
    case x86_ESI: x86Command += 0x70; break;
    case x86_EDI: x86Command += 0x78; break;
    case x86_ESP: x86Command += 0x60; break;
    case x86_EBP: x86Command += 0x68; break;
    }
    PUTDST16(RecompPos, 0xBF0F);
    PUTDST8(RecompPos, x86Command);
    PUTDST8(RecompPos, Disp);
}

void MoveSxVariableToX86regByte(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      movsx %s, byte ptr [%s]", x86_Name(x86reg), VariableName);

    PUTDST16(RecompPos, 0xbe0f);

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x05); break;
    case x86_EBX: PUTDST8(RecompPos, 0x1D); break;
    case x86_ECX: PUTDST8(RecompPos, 0x0D); break;
    case x86_EDX: PUTDST8(RecompPos, 0x15); break;
    case x86_ESI: PUTDST8(RecompPos, 0x35); break;
    case x86_EDI: PUTDST8(RecompPos, 0x3D); break;
    case x86_ESP: PUTDST8(RecompPos, 0x25); break;
    case x86_EBP: PUTDST8(RecompPos, 0x2D); break;
    default: g_Notify->DisplayError("MoveSxVariableToX86regByte\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveSxVariableToX86regHalf(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      movsx %s, word ptr [%s]", x86_Name(x86reg), VariableName);

    PUTDST16(RecompPos, 0xbf0f);

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x05); break;
    case x86_EBX: PUTDST8(RecompPos, 0x1D); break;
    case x86_ECX: PUTDST8(RecompPos, 0x0D); break;
    case x86_EDX: PUTDST8(RecompPos, 0x15); break;
    case x86_ESI: PUTDST8(RecompPos, 0x35); break;
    case x86_EDI: PUTDST8(RecompPos, 0x3D); break;
    case x86_ESP: PUTDST8(RecompPos, 0x25); break;
    case x86_EBP: PUTDST8(RecompPos, 0x2D); break;
    default: g_Notify->DisplayError("MoveSxVariableToX86regHalf\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveSxN64MemToX86regByte(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      movsx %s, byte ptr [%s+Dmem]", x86_Name(x86reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00BE; break;
    case x86_EBX: x86Command = 0x03BE; break;
    case x86_ECX: x86Command = 0x01BE; break;
    case x86_EDX: x86Command = 0x02BE; break;
    case x86_ESI: x86Command = 0x06BE; break;
    case x86_EDI: x86Command = 0x07BE; break;
    case x86_ESP: x86Command = 0x04BE; break;
    case x86_EBP: x86Command = 0x05BE; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    default:
        g_Notify->DisplayError("MoveSxN64MemToX86regByte\nInvalid x86 register");
        break;
    }
    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveSxN64MemToX86regHalf(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      movsx %s, word ptr [%s+Dmem]", x86_Name(x86reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00BF; break;
    case x86_EBX: x86Command = 0x03BF; break;
    case x86_ECX: x86Command = 0x01BF; break;
    case x86_EDX: x86Command = 0x02BF; break;
    case x86_ESI: x86Command = 0x06BF; break;
    case x86_EDI: x86Command = 0x07BF; break;
    case x86_ESP: x86Command = 0x04BF; break;
    case x86_EBP: x86Command = 0x05BF; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }

    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveZxX86RegHalfToX86Reg(int Source, int Destination)
{
    uint16_t x86Command = 0;

    CPU_Message("      movzx %s, %s", x86_Name(Destination), x86Half_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x00B7; break;
    case x86_EBX: x86Command = 0x03B7; break;
    case x86_ECX: x86Command = 0x01B7; break;
    case x86_EDX: x86Command = 0x02B7; break;
    case x86_ESI: x86Command = 0x06B7; break;
    case x86_EDI: x86Command = 0x07B7; break;
    case x86_ESP: x86Command = 0x04B7; break;
    case x86_EBP: x86Command = 0x05B7; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
}

void MoveZxX86RegPtrDispToX86RegHalf(int AddrReg, uint8_t Disp, int Destination)
{
    uint8_t x86Command = 0;

    CPU_Message("      movzx %s, [%s+%X]", x86_Name(Destination), x86_Name(AddrReg), Disp);

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00; break;
    case x86_EBX: x86Command = 0x03; break;
    case x86_ECX: x86Command = 0x01; break;
    case x86_EDX: x86Command = 0x02; break;
    case x86_ESI: x86Command = 0x06; break;
    case x86_EDI: x86Command = 0x07; break;
    case x86_ESP: x86Command = 0x04; break;
    case x86_EBP: x86Command = 0x05; break;
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0x40; break;
    case x86_EBX: x86Command += 0x58; break;
    case x86_ECX: x86Command += 0x48; break;
    case x86_EDX: x86Command += 0x50; break;
    case x86_ESI: x86Command += 0x70; break;
    case x86_EDI: x86Command += 0x78; break;
    case x86_ESP: x86Command += 0x60; break;
    case x86_EBP: x86Command += 0x68; break;
    }
    PUTDST16(RecompPos, 0xB70F);
    PUTDST8(RecompPos, x86Command);
    PUTDST8(RecompPos, Disp);
}

void MoveZxVariableToX86regByte(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      movzx %s, byte ptr [%s]", x86_Name(x86reg), VariableName);

    PUTDST16(RecompPos, 0xb60f);

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x05); break;
    case x86_EBX: PUTDST8(RecompPos, 0x1D); break;
    case x86_ECX: PUTDST8(RecompPos, 0x0D); break;
    case x86_EDX: PUTDST8(RecompPos, 0x15); break;
    case x86_ESI: PUTDST8(RecompPos, 0x35); break;
    case x86_EDI: PUTDST8(RecompPos, 0x3D); break;
    case x86_ESP: PUTDST8(RecompPos, 0x25); break;
    case x86_EBP: PUTDST8(RecompPos, 0x2D); break;
    default: g_Notify->DisplayError("MoveZxVariableToX86regByte\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveZxVariableToX86regHalf(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      movzx %s, word ptr [%s]", x86_Name(x86reg), VariableName);

    PUTDST16(RecompPos, 0xb70f);

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x05); break;
    case x86_EBX: PUTDST8(RecompPos, 0x1D); break;
    case x86_ECX: PUTDST8(RecompPos, 0x0D); break;
    case x86_EDX: PUTDST8(RecompPos, 0x15); break;
    case x86_ESI: PUTDST8(RecompPos, 0x35); break;
    case x86_EDI: PUTDST8(RecompPos, 0x3D); break;
    case x86_ESP: PUTDST8(RecompPos, 0x25); break;
    case x86_EBP: PUTDST8(RecompPos, 0x2D); break;
    default: g_Notify->DisplayError("MoveZxVariableToX86regHalf\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void MoveZxN64MemToX86regByte(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      movzx %s, byte ptr [%s+Dmem]", x86_Name(x86reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00B6; break;
    case x86_EBX: x86Command = 0x03B6; break;
    case x86_ECX: x86Command = 0x01B6; break;
    case x86_EDX: x86Command = 0x02B6; break;
    case x86_ESI: x86Command = 0x06B6; break;
    case x86_EDI: x86Command = 0x07B6; break;
    case x86_ESP: x86Command = 0x04B6; break;
    case x86_EBP: x86Command = 0x05B6; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    default:
        g_Notify->DisplayError("MoveZxN64MemToX86regByte\nInvalid x86 register");
        break;
    }
    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MoveZxN64MemToX86regHalf(int x86reg, int AddrReg)
{
    uint16_t x86Command = 0;

    CPU_Message("      movzx %s, word ptr [%s+Dmem]", x86_Name(x86reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00B7; break;
    case x86_EBX: x86Command = 0x03B7; break;
    case x86_ECX: x86Command = 0x01B7; break;
    case x86_EDX: x86Command = 0x02B7; break;
    case x86_ESI: x86Command = 0x06B7; break;
    case x86_EDI: x86Command = 0x07B7; break;
    case x86_ESP: x86Command = 0x04B7; break;
    case x86_EBP: x86Command = 0x05B7; break;
    }
    switch (x86reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }

    PUTDST8(RecompPos, 0x0f);
    PUTDST16(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void MulX86reg(int x86reg)
{
    CPU_Message("      mul %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE0F7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xE3F7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE1F7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xE2F7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xE6F7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xE7F7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xE4F7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xE5F7); break;
    default:
        g_Notify->DisplayError("MulX86reg\nUnknown x86 register");
    }
}

void NegateX86reg(int x86reg)
{
    CPU_Message("      neg %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xd8f7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xdbf7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xd9f7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xdaf7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xdef7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xdff7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xdcf7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xddf7); break;
    default:
        g_Notify->DisplayError("NegateX86reg\nUnknown x86 register");
    }
}

void NotX86reg(int x86reg)
{
    CPU_Message("      not %s", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xd0f7); break;
    case x86_EBX: PUTDST16(RecompPos, 0xd3f7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xd1f7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xd2f7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xd6f7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xd7f7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xd4f7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xd5f7); break;
    default:
        g_Notify->DisplayError("NotX86reg\nUnknown x86 register");
    }
}

void OrConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      or dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x0D81);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void OrConstToX86Reg(uint32_t Const, int x86Reg)
{
    CPU_Message("      or %s, %Xh", x86_Name(x86Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xC881); break;
        case x86_EBX: PUTDST16(RecompPos, 0xCB81); break;
        case x86_ECX: PUTDST16(RecompPos, 0xC981); break;
        case x86_EDX: PUTDST16(RecompPos, 0xCA81); break;
        case x86_ESI: PUTDST16(RecompPos, 0xCE81); break;
        case x86_EDI: PUTDST16(RecompPos, 0xCF81); break;
        case x86_ESP: PUTDST16(RecompPos, 0xCC81); break;
        case x86_EBP: PUTDST16(RecompPos, 0xCD81); break;
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xC883); break;
        case x86_EBX: PUTDST16(RecompPos, 0xCB83); break;
        case x86_ECX: PUTDST16(RecompPos, 0xC983); break;
        case x86_EDX: PUTDST16(RecompPos, 0xCA83); break;
        case x86_ESI: PUTDST16(RecompPos, 0xCE83); break;
        case x86_EDI: PUTDST16(RecompPos, 0xCF83); break;
        case x86_ESP: PUTDST16(RecompPos, 0xCC83); break;
        case x86_EBP: PUTDST16(RecompPos, 0xCD83); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void OrVariableToX86Reg(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      or %s, dword ptr [%s]", x86_Name(x86Reg), VariableName);
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x050B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D0B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D0B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x150B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x350B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D0B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x250B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D0B); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void OrVariableToX86regHalf(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      or %s, word ptr [%s]", x86Half_Name(x86Reg), VariableName);
    PUTDST8(RecompPos, 0x66);
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x050B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D0B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D0B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x150B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x350B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D0B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x250B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D0B); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void OrX86RegToVariable(void * Variable, char * VariableName, int x86Reg)
{
    CPU_Message("      or dword ptr [%s], %s", VariableName, x86_Name(x86Reg));
    switch (x86Reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0509); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D09); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D09); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1509); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3509); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D09); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2509); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D09); break;
    }
    PUTDSTPTR(RecompPos, Variable);
}

void OrX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      or %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x000B; break;
    case x86_EBX: x86Command = 0x030B; break;
    case x86_ECX: x86Command = 0x010B; break;
    case x86_EDX: x86Command = 0x020B; break;
    case x86_ESI: x86Command = 0x060B; break;
    case x86_EDI: x86Command = 0x070B; break;
    case x86_ESP: x86Command = 0x040B; break;
    case x86_EBP: x86Command = 0x050B; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void Popad(void)
{
    CPU_Message("      popad");
    PUTDST8(RecompPos, 0x61);
}

void Pushad(void)
{
    CPU_Message("      pushad");
    PUTDST8(RecompPos, 0x60);
}

void Push(int x86reg)
{
    CPU_Message("      push %s", x86_Name(x86reg));

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x50); break;
    case x86_EBX: PUTDST8(RecompPos, 0x53); break;
    case x86_ECX: PUTDST8(RecompPos, 0x51); break;
    case x86_EDX: PUTDST8(RecompPos, 0x52); break;
    case x86_ESI: PUTDST8(RecompPos, 0x56); break;
    case x86_EDI: PUTDST8(RecompPos, 0x57); break;
    case x86_ESP: PUTDST8(RecompPos, 0x54); break;
    case x86_EBP: PUTDST8(RecompPos, 0x55); break;
    }
}

void Pop(int x86reg)
{
    CPU_Message("      pop %s", x86_Name(x86reg));

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0x58); break;
    case x86_EBX: PUTDST8(RecompPos, 0x5B); break;
    case x86_ECX: PUTDST8(RecompPos, 0x59); break;
    case x86_EDX: PUTDST8(RecompPos, 0x5A); break;
    case x86_ESI: PUTDST8(RecompPos, 0x5E); break;
    case x86_EDI: PUTDST8(RecompPos, 0x5F); break;
    case x86_ESP: PUTDST8(RecompPos, 0x5C); break;
    case x86_EBP: PUTDST8(RecompPos, 0x5D); break;
    }
}

void PushImm32(char * String, uint32_t Value)
{
    CPU_Message("      push %s", String);
    PUTDST8(RecompPos, 0x68);
    PUTDST32(RecompPos, Value);
}

void Ret(void)
{
    CPU_Message("      ret");
    PUTDST8(RecompPos, 0xC3);
}

void Setl(int x86reg)
{
    CPU_Message("      setl %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x9C0F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Setl\nUnknown x86 register");
    }
}

void SetlVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setl byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x9C0F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void SetleVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setle byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x9E0F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void Setb(int x86reg)
{
    CPU_Message("      setb %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x920F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Setb\nUnknown x86 register");
    }
}

void SetbVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setb byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x920F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void Setg(int x86reg)
{
    CPU_Message("      setg %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x9F0F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Setg\nUnknown x86 register");
    }
}

void SetgVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setg byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x9F0F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void SetgeVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setge byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x9D0F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void Seta(int x86reg)
{
    CPU_Message("      seta %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x970F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Seta\nUnknown x86 register");
    }
}

void SetaVariable(void * Variable, char * VariableName)
{
    CPU_Message("      seta byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x970F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void Setae(int x86reg)
{
    CPU_Message("      setae %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x930F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Seta\nUnknown x86 register");
    }
}

void Setz(int x86reg)
{
    CPU_Message("      setz %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x940F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Setz\nUnknown x86 register");
    }
}

void SetzVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setz byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x940F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void Setnz(int x86reg)
{
    CPU_Message("      setnz %s", x86Byte_Name(x86reg));
    PUTDST16(RecompPos, 0x950F);
    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xC0); break;
    case x86_EBX: PUTDST8(RecompPos, 0xC3); break;
    case x86_ECX: PUTDST8(RecompPos, 0xC1); break;
    case x86_EDX: PUTDST8(RecompPos, 0xC2); break;
    default:
        g_Notify->DisplayError("Setnz\nUnknown x86 register");
    }
}

void SetnzVariable(void * Variable, char * VariableName)
{
    CPU_Message("      setnz byte ptr [%s]", VariableName);
    PUTDST16(RecompPos, 0x950F);
    PUTDST8(RecompPos, 0x05);
    PUTDSTPTR(RecompPos, Variable);
}

void ShiftLeftDoubleImmed(int Destination, int Source, uint8_t Immediate)
{
    uint8_t x86Amb = 0xC0;

    CPU_Message("      shld %s, %s, %Xh", x86_Name(Destination), x86_Name(Source), Immediate);
    PUTDST16(RecompPos, 0xA40F);

    switch (Destination)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_EBX: x86Amb += 0x03; break;
    case x86_ECX: x86Amb += 0x01; break;
    case x86_EDX: x86Amb += 0x02; break;
    case x86_ESI: x86Amb += 0x06; break;
    case x86_EDI: x86Amb += 0x07; break;
    case x86_ESP: x86Amb += 0x04; break;
    case x86_EBP: x86Amb += 0x05; break;
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    }

    PUTDST8(RecompPos, x86Amb);
    PUTDST8(RecompPos, Immediate);
}

void ShiftRightDoubleImmed(int Destination, int Source, uint8_t Immediate)
{
    uint8_t x86Amb = 0xC0;

    CPU_Message("      shrd %s, %s, %Xh", x86_Name(Destination), x86_Name(Source), Immediate);
    PUTDST16(RecompPos, 0xAC0F);

    switch (Destination)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_EBX: x86Amb += 0x03; break;
    case x86_ECX: x86Amb += 0x01; break;
    case x86_EDX: x86Amb += 0x02; break;
    case x86_ESI: x86Amb += 0x06; break;
    case x86_EDI: x86Amb += 0x07; break;
    case x86_ESP: x86Amb += 0x04; break;
    case x86_EBP: x86Amb += 0x05; break;
    }

    switch (Source)
    {
    case x86_EAX: x86Amb += 0x00; break;
    case x86_EBX: x86Amb += 0x18; break;
    case x86_ECX: x86Amb += 0x08; break;
    case x86_EDX: x86Amb += 0x10; break;
    case x86_ESI: x86Amb += 0x30; break;
    case x86_EDI: x86Amb += 0x38; break;
    case x86_ESP: x86Amb += 0x20; break;
    case x86_EBP: x86Amb += 0x28; break;
    }

    PUTDST8(RecompPos, x86Amb);
    PUTDST8(RecompPos, Immediate);
}

void ShiftLeftSign(int x86reg)
{
    CPU_Message("      shl %s, cl", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE0D3); break;
    case x86_EBX: PUTDST16(RecompPos, 0xE3D3); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE1D3); break;
    case x86_EDX: PUTDST16(RecompPos, 0xE2D3); break;
    case x86_ESI: PUTDST16(RecompPos, 0xE6D3); break;
    case x86_EDI: PUTDST16(RecompPos, 0xE7D3); break;
    case x86_ESP: PUTDST16(RecompPos, 0xE4D3); break;
    case x86_EBP: PUTDST16(RecompPos, 0xE5D3); break;
    }
}

void ShiftLeftSignImmed(int x86reg, uint8_t Immediate)
{
    CPU_Message("      shl %s, %Xh", x86_Name(x86reg), Immediate);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE0C1); break;
    case x86_EBX: PUTDST16(RecompPos, 0xE3C1); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE1C1); break;
    case x86_EDX: PUTDST16(RecompPos, 0xE2C1); break;
    case x86_ESI: PUTDST16(RecompPos, 0xE6C1); break;
    case x86_EDI: PUTDST16(RecompPos, 0xE7C1); break;
    case x86_ESP: PUTDST16(RecompPos, 0xE4C1); break;
    case x86_EBP: PUTDST16(RecompPos, 0xE5C1); break;
    }
    PUTDST8(RecompPos, Immediate);
}

void ShiftLeftSignVariableImmed(void * Variable, char * VariableName, uint8_t Immediate)
{
    CPU_Message("      shl dword ptr [%s], %Xh", VariableName, Immediate);

    PUTDST16(RecompPos, 0x25C1)
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Immediate);
}

void ShiftRightSignImmed(int x86reg, uint8_t Immediate)
{
    CPU_Message("      sar %s, %Xh", x86_Name(x86reg), Immediate);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xF8C1); break;
    case x86_EBX: PUTDST16(RecompPos, 0xFBC1); break;
    case x86_ECX: PUTDST16(RecompPos, 0xF9C1); break;
    case x86_EDX: PUTDST16(RecompPos, 0xFAC1); break;
    case x86_ESI: PUTDST16(RecompPos, 0xFEC1); break;
    case x86_EDI: PUTDST16(RecompPos, 0xFFC1); break;
    case x86_ESP: PUTDST16(RecompPos, 0xFCC1); break;
    case x86_EBP: PUTDST16(RecompPos, 0xFDC1); break;
    default:
        g_Notify->DisplayError("ShiftRightSignImmed\nUnknown x86 register");
    }
    PUTDST8(RecompPos, Immediate);
}

void ShiftRightSignVariableImmed(void * Variable, char * VariableName, uint8_t Immediate)
{
    CPU_Message("      sar dword ptr [%s], %Xh", VariableName, Immediate);

    PUTDST16(RecompPos, 0x3DC1)
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Immediate);
}

void ShiftRightUnsign(int x86reg)
{
    CPU_Message("      shr %s, cl", x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE8D3); break;
    case x86_EBX: PUTDST16(RecompPos, 0xEBD3); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE9D3); break;
    case x86_EDX: PUTDST16(RecompPos, 0xEAD3); break;
    case x86_ESI: PUTDST16(RecompPos, 0xEED3); break;
    case x86_EDI: PUTDST16(RecompPos, 0xEFD3); break;
    case x86_ESP: PUTDST16(RecompPos, 0xECD3); break;
    case x86_EBP: PUTDST16(RecompPos, 0xEDD3); break;
    }
}

void ShiftRightUnsignImmed(int x86reg, uint8_t Immediate)
{
    CPU_Message("      shr %s, %Xh", x86_Name(x86reg), Immediate);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0xE8C1); break;
    case x86_EBX: PUTDST16(RecompPos, 0xEBC1); break;
    case x86_ECX: PUTDST16(RecompPos, 0xE9C1); break;
    case x86_EDX: PUTDST16(RecompPos, 0xEAC1); break;
    case x86_ESI: PUTDST16(RecompPos, 0xEEC1); break;
    case x86_EDI: PUTDST16(RecompPos, 0xEFC1); break;
    case x86_ESP: PUTDST16(RecompPos, 0xECC1); break;
    case x86_EBP: PUTDST16(RecompPos, 0xEDC1); break;
    }
    PUTDST8(RecompPos, Immediate);
}

void ShiftRightUnsignVariableImmed(void * Variable, char * VariableName, uint8_t Immediate)
{
    CPU_Message("      shr dword ptr [%s], %Xh", VariableName, Immediate);

    PUTDST16(RecompPos, 0x2DC1)
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Immediate);
}

void SubConstFromVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      sub dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x2D81);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void SubConstFromX86Reg(int x86Reg, uint32_t Const)
{
    CPU_Message("      sub %s, %Xh", x86_Name(x86Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xE881); break;
        case x86_EBX: PUTDST16(RecompPos, 0xEB81); break;
        case x86_ECX: PUTDST16(RecompPos, 0xE981); break;
        case x86_EDX: PUTDST16(RecompPos, 0xEA81); break;
        case x86_ESI: PUTDST16(RecompPos, 0xEE81); break;
        case x86_EDI: PUTDST16(RecompPos, 0xEF81); break;
        case x86_ESP: PUTDST16(RecompPos, 0xEC81); break;
        case x86_EBP: PUTDST16(RecompPos, 0xED81); break;
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xE883); break;
        case x86_EBX: PUTDST16(RecompPos, 0xEB83); break;
        case x86_ECX: PUTDST16(RecompPos, 0xE983); break;
        case x86_EDX: PUTDST16(RecompPos, 0xEA83); break;
        case x86_ESI: PUTDST16(RecompPos, 0xEE83); break;
        case x86_EDI: PUTDST16(RecompPos, 0xEF83); break;
        case x86_ESP: PUTDST16(RecompPos, 0xEC83); break;
        case x86_EBP: PUTDST16(RecompPos, 0xED83); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void SubVariableFromX86reg(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      sub %s, dword ptr [%s]", x86_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x052B); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D2B); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D2B); break;
    case x86_EDX: PUTDST16(RecompPos, 0x152B); break;
    case x86_ESI: PUTDST16(RecompPos, 0x352B); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D2B); break;
    case x86_ESP: PUTDST16(RecompPos, 0x252B); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D2B); break;
    default:
        g_Notify->DisplayError("SubVariableFromX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void SubX86regFromVariable(int x86reg, void * Variable, char * VariableName)
{
    CPU_Message("      sub dword ptr [%s], %s", VariableName, x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0529); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D29); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D29); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1529); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3529); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D29); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2529); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D29); break;
    default:
        g_Notify->DisplayError("SubX86regFromVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void SubX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;
    CPU_Message("      sub %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x002B; break;
    case x86_EBX: x86Command = 0x032B; break;
    case x86_ECX: x86Command = 0x012B; break;
    case x86_EDX: x86Command = 0x022B; break;
    case x86_ESI: x86Command = 0x062B; break;
    case x86_EDI: x86Command = 0x072B; break;
    case x86_ESP: x86Command = 0x042B; break;
    case x86_EBP: x86Command = 0x052B; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void SbbX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;
    CPU_Message("      sbb %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x001B; break;
    case x86_EBX: x86Command = 0x031B; break;
    case x86_ECX: x86Command = 0x011B; break;
    case x86_EDX: x86Command = 0x021B; break;
    case x86_ESI: x86Command = 0x061B; break;
    case x86_EDI: x86Command = 0x071B; break;
    case x86_ESP: x86Command = 0x041B; break;
    case x86_EBP: x86Command = 0x051B; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void TestConstToVariable(uint32_t Const, void * Variable, char * VariableName)
{
    CPU_Message("      test dword ptr [%s], 0x%X", VariableName, Const);
    PUTDST16(RecompPos, 0x05F7);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void TestConstToX86Reg(uint32_t Const, int x86reg)
{
    CPU_Message("      test %s, 0x%X", x86_Name(x86reg), Const);

    switch (x86reg)
    {
    case x86_EAX: PUTDST8(RecompPos, 0xA9); break;
    case x86_EBX: PUTDST16(RecompPos, 0xC3F7); break;
    case x86_ECX: PUTDST16(RecompPos, 0xC1F7); break;
    case x86_EDX: PUTDST16(RecompPos, 0xC2F7); break;
    case x86_ESI: PUTDST16(RecompPos, 0xC6F7); break;
    case x86_EDI: PUTDST16(RecompPos, 0xC7F7); break;
    case x86_ESP: PUTDST16(RecompPos, 0xC4F7); break;
    case x86_EBP: PUTDST16(RecompPos, 0xC5F7); break;
    }
    PUTDST32(RecompPos, Const);
}

void TestX86RegToX86Reg(int Destination, int Source)
{
    uint16_t x86Command = 0;

    CPU_Message("      test %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x0085; break;
    case x86_EBX: x86Command = 0x0385; break;
    case x86_ECX: x86Command = 0x0185; break;
    case x86_EDX: x86Command = 0x0285; break;
    case x86_ESI: x86Command = 0x0685; break;
    case x86_EDI: x86Command = 0x0785; break;
    case x86_ESP: x86Command = 0x0485; break;
    case x86_EBP: x86Command = 0x0585; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void XorConstToX86Reg(int x86Reg, uint32_t Const)
{
    CPU_Message("      xor %s, %Xh", x86_Name(x86Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xF081); break;
        case x86_EBX: PUTDST16(RecompPos, 0xF381); break;
        case x86_ECX: PUTDST16(RecompPos, 0xF181); break;
        case x86_EDX: PUTDST16(RecompPos, 0xF281); break;
        case x86_ESI: PUTDST16(RecompPos, 0xF681); break;
        case x86_EDI: PUTDST16(RecompPos, 0xF781); break;
        case x86_ESP: PUTDST16(RecompPos, 0xF481); break;
        case x86_EBP: PUTDST16(RecompPos, 0xF581); break;
        }
        PUTDST32(RecompPos, Const);
    }
    else
    {
        switch (x86Reg)
        {
        case x86_EAX: PUTDST16(RecompPos, 0xF083); break;
        case x86_EBX: PUTDST16(RecompPos, 0xF383); break;
        case x86_ECX: PUTDST16(RecompPos, 0xF183); break;
        case x86_EDX: PUTDST16(RecompPos, 0xF283); break;
        case x86_ESI: PUTDST16(RecompPos, 0xF683); break;
        case x86_EDI: PUTDST16(RecompPos, 0xF783); break;
        case x86_ESP: PUTDST16(RecompPos, 0xF483); break;
        case x86_EBP: PUTDST16(RecompPos, 0xF583); break;
        }
        PUTDST8(RecompPos, Const);
    }
}

void XorConstToVariable(void * Variable, char * VariableName, uint32_t Const)
{

    CPU_Message("      xor dword ptr [%s], 0x%X", VariableName, Const);

    PUTDST16(RecompPos, 0x3581);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST32(RecompPos, Const);
}

void XorX86RegToX86Reg(int Source, int Destination)
{
    uint16_t x86Command = 0;

    CPU_Message("      xor %s, %s", x86_Name(Source), x86_Name(Destination));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x0031; break;
    case x86_EBX: x86Command = 0x0331; break;
    case x86_ECX: x86Command = 0x0131; break;
    case x86_EDX: x86Command = 0x0231; break;
    case x86_ESI: x86Command = 0x0631; break;
    case x86_EDI: x86Command = 0x0731; break;
    case x86_ESP: x86Command = 0x0431; break;
    case x86_EBP: x86Command = 0x0531; break;
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    PUTDST16(RecompPos, x86Command);
}

void XorVariableToX86reg(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      xor %s, dword ptr [%s]", x86_Name(x86reg), VariableName);
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0533); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D33); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D33); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1533); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3533); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D33); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2533); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D33); break;
    default: g_Notify->DisplayError("XorVariableToX86reg\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void XorX86RegToVariable(void * Variable, char * VariableName, int x86reg)
{
    CPU_Message("      xor dword ptr [%s], %s", VariableName, x86_Name(x86reg));
    switch (x86reg)
    {
    case x86_EAX: PUTDST16(RecompPos, 0x0531); break;
    case x86_EBX: PUTDST16(RecompPos, 0x1D31); break;
    case x86_ECX: PUTDST16(RecompPos, 0x0D31); break;
    case x86_EDX: PUTDST16(RecompPos, 0x1531); break;
    case x86_ESI: PUTDST16(RecompPos, 0x3531); break;
    case x86_EDI: PUTDST16(RecompPos, 0x3D31); break;
    case x86_ESP: PUTDST16(RecompPos, 0x2531); break;
    case x86_EBP: PUTDST16(RecompPos, 0x2D31); break;
    default: g_Notify->DisplayError("XorX86RegToVariable\nUnknown x86 register");
    }
    PUTDSTPTR(RecompPos, Variable);
}

void * GetAddressOf_(int value, ...)
{
    void * Address;

    va_list ap;
    va_start(ap, value);
    Address = va_arg(ap, void *);
    va_end(ap);

    return Address;
}
