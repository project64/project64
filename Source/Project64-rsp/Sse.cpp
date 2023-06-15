#include "RSP registers.h"
#include "Rsp.h"
#include "log.h"
#include "memory.h"
#include "x86.h"
#include <stdio.h>
#include <windows.h>

#define PUTDST8(dest, value)             \
    (*((BYTE *)(dest)) = (BYTE)(value)); \
    dest += 1;
#define PUTDST16(dest, value)            \
    (*((WORD *)(dest)) = (WORD)(value)); \
    dest += 2;
#define PUTDST32(dest, value)              \
    (*((DWORD *)(dest)) = (DWORD)(value)); \
    dest += 4;
#define PUTDSTPTR(dest, value)          \
    *(void **)(dest) = (void *)(value); \
    dest += sizeof(void *);

char * sse_Strings[8] = {
    "xmm0", "xmm1", "xmm2", "xmm3",
    "xmm4", "xmm5", "xmm6", "xmm7"};

#define sse_Name(Reg) (sse_Strings[(Reg)])

void SseMoveAlignedVariableToReg(void * Variable, char * VariableName, int sseReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movaps %s, xmmword ptr [%s]", sse_Name(sseReg), VariableName);

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x05; break;
    case x86_XMM1: x86Command = 0x0D; break;
    case x86_XMM2: x86Command = 0x15; break;
    case x86_XMM3: x86Command = 0x1D; break;
    case x86_XMM4: x86Command = 0x25; break;
    case x86_XMM5: x86Command = 0x2D; break;
    case x86_XMM6: x86Command = 0x35; break;
    case x86_XMM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x280f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void SseMoveAlignedN64MemToReg(int sseReg, int AddrReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movaps %s, xmmword ptr [Dmem+%s]", sse_Name(sseReg), x86_Name(AddrReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x80; break;
    case x86_XMM1: x86Command = 0x88; break;
    case x86_XMM2: x86Command = 0x90; break;
    case x86_XMM3: x86Command = 0x98; break;
    case x86_XMM4: x86Command = 0xA0; break;
    case x86_XMM5: x86Command = 0xA8; break;
    case x86_XMM6: x86Command = 0xB0; break;
    case x86_XMM7: x86Command = 0xB8; break;
    }
    switch (AddrReg)
    {
    case x86_EAX: x86Command += 0x00; break;
    case x86_EBX: x86Command += 0x03; break;
    case x86_ECX: x86Command += 0x01; break;
    case x86_EDX: x86Command += 0x02; break;
    case x86_ESI: x86Command += 0x06; break;
    case x86_EDI: x86Command += 0x07; break;
    case x86_ESP: x86Command += 0x04; break;
    case x86_EBP: x86Command += 0x05; break;
    }

    PUTDST16(RecompPos, 0x280f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void SseMoveAlignedRegToVariable(int sseReg, void * Variable, char * VariableName)
{
    BYTE x86Command = 0;

    CPU_Message("      movaps xmmword ptr [%s], %s", VariableName, sse_Name(sseReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x05; break;
    case x86_XMM1: x86Command = 0x0D; break;
    case x86_XMM2: x86Command = 0x15; break;
    case x86_XMM3: x86Command = 0x1D; break;
    case x86_XMM4: x86Command = 0x25; break;
    case x86_XMM5: x86Command = 0x2D; break;
    case x86_XMM6: x86Command = 0x35; break;
    case x86_XMM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x290f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void SseMoveAlignedRegToN64Mem(int sseReg, int AddrReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movaps xmmword ptr [Dmem+%s], %s", x86_Name(AddrReg), sse_Name(sseReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x80; break;
    case x86_XMM1: x86Command = 0x88; break;
    case x86_XMM2: x86Command = 0x90; break;
    case x86_XMM3: x86Command = 0x98; break;
    case x86_XMM4: x86Command = 0xA0; break;
    case x86_XMM5: x86Command = 0xA8; break;
    case x86_XMM6: x86Command = 0xB0; break;
    case x86_XMM7: x86Command = 0xB8; break;
    }
    switch (AddrReg)
    {
    case x86_EAX: x86Command += 0x00; break;
    case x86_EBX: x86Command += 0x03; break;
    case x86_ECX: x86Command += 0x01; break;
    case x86_EDX: x86Command += 0x02; break;
    case x86_ESI: x86Command += 0x06; break;
    case x86_EDI: x86Command += 0x07; break;
    case x86_ESP: x86Command += 0x04; break;
    case x86_EBP: x86Command += 0x05; break;
    }

    PUTDST16(RecompPos, 0x290f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void SseMoveUnalignedVariableToReg(void * Variable, char * VariableName, int sseReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movups %s, xmmword ptr [%s]", sse_Name(sseReg), VariableName);

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x05; break;
    case x86_XMM1: x86Command = 0x0D; break;
    case x86_XMM2: x86Command = 0x15; break;
    case x86_XMM3: x86Command = 0x1D; break;
    case x86_XMM4: x86Command = 0x25; break;
    case x86_XMM5: x86Command = 0x2D; break;
    case x86_XMM6: x86Command = 0x35; break;
    case x86_XMM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x100f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void SseMoveUnalignedN64MemToReg(int sseReg, int AddrReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movups %s, xmmword ptr [Dmem+%s]", sse_Name(sseReg), x86_Name(AddrReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x80; break;
    case x86_XMM1: x86Command = 0x88; break;
    case x86_XMM2: x86Command = 0x90; break;
    case x86_XMM3: x86Command = 0x98; break;
    case x86_XMM4: x86Command = 0xA0; break;
    case x86_XMM5: x86Command = 0xA8; break;
    case x86_XMM6: x86Command = 0xB0; break;
    case x86_XMM7: x86Command = 0xB8; break;
    }
    switch (AddrReg)
    {
    case x86_EAX: x86Command += 0x00; break;
    case x86_EBX: x86Command += 0x03; break;
    case x86_ECX: x86Command += 0x01; break;
    case x86_EDX: x86Command += 0x02; break;
    case x86_ESI: x86Command += 0x06; break;
    case x86_EDI: x86Command += 0x07; break;
    case x86_ESP: x86Command += 0x04; break;
    case x86_EBP: x86Command += 0x05; break;
    }

    PUTDST16(RecompPos, 0x100f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void SseMoveUnalignedRegToVariable(int sseReg, void * Variable, char * VariableName)
{
    BYTE x86Command = 0;

    CPU_Message("      movups xmmword ptr [%s], %s", VariableName, sse_Name(sseReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x05; break;
    case x86_XMM1: x86Command = 0x0D; break;
    case x86_XMM2: x86Command = 0x15; break;
    case x86_XMM3: x86Command = 0x1D; break;
    case x86_XMM4: x86Command = 0x25; break;
    case x86_XMM5: x86Command = 0x2D; break;
    case x86_XMM6: x86Command = 0x35; break;
    case x86_XMM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x110f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void SseMoveUnalignedRegToN64Mem(int sseReg, int AddrReg)
{
    BYTE x86Command = 0;

    CPU_Message("      movups xmmword ptr [Dmem+%s], %s", x86_Name(AddrReg), sse_Name(sseReg));

    switch (sseReg)
    {
    case x86_XMM0: x86Command = 0x80; break;
    case x86_XMM1: x86Command = 0x88; break;
    case x86_XMM2: x86Command = 0x90; break;
    case x86_XMM3: x86Command = 0x98; break;
    case x86_XMM4: x86Command = 0xA0; break;
    case x86_XMM5: x86Command = 0xA8; break;
    case x86_XMM6: x86Command = 0xB0; break;
    case x86_XMM7: x86Command = 0xB8; break;
    }
    switch (AddrReg)
    {
    case x86_EAX: x86Command += 0x00; break;
    case x86_EBX: x86Command += 0x03; break;
    case x86_ECX: x86Command += 0x01; break;
    case x86_EDX: x86Command += 0x02; break;
    case x86_ESI: x86Command += 0x06; break;
    case x86_EDI: x86Command += 0x07; break;
    case x86_ESP: x86Command += 0x04; break;
    case x86_EBP: x86Command += 0x05; break;
    }

    PUTDST16(RecompPos, 0x110f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, RSPInfo.DMEM);
}

void SseMoveRegToReg(int Dest, int Source)
{
    BYTE x86Command = 0;

    CPU_Message("      movaps %s, %s", sse_Name(Dest), sse_Name(Source));

    switch (Dest)
    {
    case x86_XMM0: x86Command = 0x00; break;
    case x86_XMM1: x86Command = 0x08; break;
    case x86_XMM2: x86Command = 0x10; break;
    case x86_XMM3: x86Command = 0x18; break;
    case x86_XMM4: x86Command = 0x20; break;
    case x86_XMM5: x86Command = 0x28; break;
    case x86_XMM6: x86Command = 0x30; break;
    case x86_XMM7: x86Command = 0x38; break;
    }
    switch (Source)
    {
    case x86_XMM0: x86Command += 0x00; break;
    case x86_XMM1: x86Command += 0x01; break;
    case x86_XMM2: x86Command += 0x02; break;
    case x86_XMM3: x86Command += 0x03; break;
    case x86_XMM4: x86Command += 0x04; break;
    case x86_XMM5: x86Command += 0x05; break;
    case x86_XMM6: x86Command += 0x06; break;
    case x86_XMM7: x86Command += 0x07; break;
    }

    PUTDST16(RecompPos, 0x280f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void SseXorRegToReg(int Dest, int Source)
{
    BYTE x86Command = 0;

    CPU_Message("      xorps %s, %s", sse_Name(Dest), sse_Name(Source));

    switch (Dest)
    {
    case x86_XMM0: x86Command = 0x00; break;
    case x86_XMM1: x86Command = 0x08; break;
    case x86_XMM2: x86Command = 0x10; break;
    case x86_XMM3: x86Command = 0x18; break;
    case x86_XMM4: x86Command = 0x20; break;
    case x86_XMM5: x86Command = 0x28; break;
    case x86_XMM6: x86Command = 0x30; break;
    case x86_XMM7: x86Command = 0x38; break;
    }
    switch (Source)
    {
    case x86_XMM0: x86Command += 0x00; break;
    case x86_XMM1: x86Command += 0x01; break;
    case x86_XMM2: x86Command += 0x02; break;
    case x86_XMM3: x86Command += 0x03; break;
    case x86_XMM4: x86Command += 0x04; break;
    case x86_XMM5: x86Command += 0x05; break;
    case x86_XMM6: x86Command += 0x06; break;
    case x86_XMM7: x86Command += 0x07; break;
    }
    PUTDST16(RecompPos, 0x570f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void SseShuffleReg(int Dest, int Source, BYTE Immed)
{
    BYTE x86Command = 0;

    CPU_Message("      shufps %s, %s, %02X", sse_Name(Dest), sse_Name(Source), Immed);

    switch (Dest)
    {
    case x86_XMM0: x86Command = 0x00; break;
    case x86_XMM1: x86Command = 0x08; break;
    case x86_XMM2: x86Command = 0x10; break;
    case x86_XMM3: x86Command = 0x18; break;
    case x86_XMM4: x86Command = 0x20; break;
    case x86_XMM5: x86Command = 0x28; break;
    case x86_XMM6: x86Command = 0x30; break;
    case x86_XMM7: x86Command = 0x38; break;
    }
    switch (Source)
    {
    case x86_XMM0: x86Command += 0x00; break;
    case x86_XMM1: x86Command += 0x01; break;
    case x86_XMM2: x86Command += 0x02; break;
    case x86_XMM3: x86Command += 0x03; break;
    case x86_XMM4: x86Command += 0x04; break;
    case x86_XMM5: x86Command += 0x05; break;
    case x86_XMM6: x86Command += 0x06; break;
    case x86_XMM7: x86Command += 0x07; break;
    }
    PUTDST16(RecompPos, 0xC60f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
    PUTDST8(RecompPos, Immed);
}
