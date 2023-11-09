#include "X86.h"
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>

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

char * mmx_Strings[8] = {
    "mm0", "mm1", "mm2", "mm3",
    "mm4", "mm5", "mm6", "mm7"};

#define mmx_Name(Reg) (mmx_Strings[(Reg)])

void MmxEmptyMultimediaState(void)
{
    CPU_Message("      emms");
    PUTDST16(RecompPos, 0x770f);
}

void MmxMoveRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      movq %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0; break;
    case x86_MM1: x86Command = 1; break;
    case x86_MM2: x86Command = 2; break;
    case x86_MM3: x86Command = 3; break;
    case x86_MM4: x86Command = 4; break;
    case x86_MM5: x86Command = 5; break;
    case x86_MM6: x86Command = 6; break;
    case x86_MM7: x86Command = 7; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0 << 3; break;
    case x86_MM1: x86Command |= 1 << 3; break;
    case x86_MM2: x86Command |= 2 << 3; break;
    case x86_MM3: x86Command |= 3 << 3; break;
    case x86_MM4: x86Command |= 4 << 3; break;
    case x86_MM5: x86Command |= 5 << 3; break;
    case x86_MM6: x86Command |= 6 << 3; break;
    case x86_MM7: x86Command |= 7 << 3; break;
    }
    PUTDST16(RecompPos, 0x7f0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxMoveQwordVariableToReg(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      movq %s, qword ptr [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x6f0f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxMoveQwordRegToVariable(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      movq qword ptr [%s], %s", VariableName, mmx_Name(Dest));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x7f0f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPorRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      por %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xeb0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPorVariableToReg(void * Variable, const char * VariableName, int Dest)
{
    uint8_t x86Command = 0;

    CPU_Message("      por %s, qword ptr [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0xeb0f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPandRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pand %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xdb0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPandVariableToReg(void * Variable, const char * VariableName, int Dest)
{
    uint8_t x86Command = 0;

    CPU_Message("      pand %s, qword ptr [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0xdb0f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPandnRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pandn %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xdf0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxXorRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pxor %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x00; break;
    case x86_MM1: x86Command = 0x08; break;
    case x86_MM2: x86Command = 0x10; break;
    case x86_MM3: x86Command = 0x18; break;
    case x86_MM4: x86Command = 0x20; break;
    case x86_MM5: x86Command = 0x28; break;
    case x86_MM6: x86Command = 0x30; break;
    case x86_MM7: x86Command = 0x38; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command += 0x00; break;
    case x86_MM1: x86Command += 0x01; break;
    case x86_MM2: x86Command += 0x02; break;
    case x86_MM3: x86Command += 0x03; break;
    case x86_MM4: x86Command += 0x04; break;
    case x86_MM5: x86Command += 0x05; break;
    case x86_MM6: x86Command += 0x06; break;
    case x86_MM7: x86Command += 0x07; break;
    }
    PUTDST16(RecompPos, 0xef0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxShuffleMemoryToReg(int Dest, void * Variable, const char * VariableName, uint8_t Immed)
{
    uint8_t x86Command = 0;

    CPU_Message("      pshufw %s, [%s], %02X", mmx_Name(Dest), VariableName, Immed);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0x700f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
    PUTDST8(RecompPos, Immed);
}

void MmxPcmpeqwRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("     pcmpeqw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0x750f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPmullwRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pmullw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xd50f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPmullwVariableToReg(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      pmullw %s, [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }
    PUTDST16(RecompPos, 0xd50f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPmulhuwRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pmulhuw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xe40f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPmulhwRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pmulhw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xe50f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPmulhwRegToVariable(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      pmulhw %s, [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }
    PUTDST16(RecompPos, 0xe50f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPsrlwImmed(int Dest, uint8_t Immed)
{
    uint8_t x86Command = 0;

    CPU_Message("      psrlw %s, %i", mmx_Name(Dest), Immed);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0xD0; break;
    case x86_MM1: x86Command = 0xD1; break;
    case x86_MM2: x86Command = 0xD2; break;
    case x86_MM3: x86Command = 0xD3; break;
    case x86_MM4: x86Command = 0xD4; break;
    case x86_MM5: x86Command = 0xD5; break;
    case x86_MM6: x86Command = 0xD6; break;
    case x86_MM7: x86Command = 0xD7; break;
    }

    PUTDST16(RecompPos, 0x710f);
    PUTDST8(RecompPos, x86Command);
    PUTDST8(RecompPos, Immed);
}

void MmxPsrawImmed(int Dest, uint8_t Immed)
{
    uint8_t x86Command = 0;

    CPU_Message("      psraw %s, %i", mmx_Name(Dest), Immed);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0xE0; break;
    case x86_MM1: x86Command = 0xE1; break;
    case x86_MM2: x86Command = 0xE2; break;
    case x86_MM3: x86Command = 0xE3; break;
    case x86_MM4: x86Command = 0xE4; break;
    case x86_MM5: x86Command = 0xE5; break;
    case x86_MM6: x86Command = 0xE6; break;
    case x86_MM7: x86Command = 0xE7; break;
    }

    PUTDST16(RecompPos, 0x710f);
    PUTDST8(RecompPos, x86Command);
    PUTDST8(RecompPos, Immed);
}

void MmxPsllwImmed(int Dest, uint8_t Immed)
{
    uint8_t x86Command = 0;

    CPU_Message("      psllw %s, %i", mmx_Name(Dest), Immed);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0xF0; break;
    case x86_MM1: x86Command = 0xF1; break;
    case x86_MM2: x86Command = 0xF2; break;
    case x86_MM3: x86Command = 0xF3; break;
    case x86_MM4: x86Command = 0xF4; break;
    case x86_MM5: x86Command = 0xF5; break;
    case x86_MM6: x86Command = 0xF6; break;
    case x86_MM7: x86Command = 0xF7; break;
    }

    PUTDST16(RecompPos, 0x710f);
    PUTDST8(RecompPos, x86Command);
    PUTDST8(RecompPos, Immed);
}

void MmxPaddswRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      paddsw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xed0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPsubswRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      psubsw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xe90f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPaddswVariableToReg(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      paddsw %s, [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0xed0f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPsubswVariableToReg(int Dest, void * Variable, const char * VariableName)
{
    uint8_t x86Command = 0;

    CPU_Message("      psubsw %s, [%s]", mmx_Name(Dest), VariableName);

    switch (Dest)
    {
    case x86_MM0: x86Command = 0x05; break;
    case x86_MM1: x86Command = 0x0D; break;
    case x86_MM2: x86Command = 0x15; break;
    case x86_MM3: x86Command = 0x1D; break;
    case x86_MM4: x86Command = 0x25; break;
    case x86_MM5: x86Command = 0x2D; break;
    case x86_MM6: x86Command = 0x35; break;
    case x86_MM7: x86Command = 0x3D; break;
    }

    PUTDST16(RecompPos, 0xe90f);
    PUTDST8(RecompPos, x86Command);
    PUTDSTPTR(RecompPos, Variable);
}

void MmxPaddwRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      paddw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0xfd0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxPackSignedDwords(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      packssdw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0x6b0f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxUnpackLowWord(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      punpcklwd %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0x610f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxUnpackHighWord(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      punpckhwd %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0x690f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}

void MmxCompareGreaterWordRegToReg(int Dest, int Source)
{
    uint8_t x86Command = 0;

    CPU_Message("      pcmpgtw %s, %s", mmx_Name(Dest), mmx_Name(Source));

    switch (Dest)
    {
    case x86_MM0: x86Command = 0 << 3; break;
    case x86_MM1: x86Command = 1 << 3; break;
    case x86_MM2: x86Command = 2 << 3; break;
    case x86_MM3: x86Command = 3 << 3; break;
    case x86_MM4: x86Command = 4 << 3; break;
    case x86_MM5: x86Command = 5 << 3; break;
    case x86_MM6: x86Command = 6 << 3; break;
    case x86_MM7: x86Command = 7 << 3; break;
    }
    switch (Source)
    {
    case x86_MM0: x86Command |= 0; break;
    case x86_MM1: x86Command |= 1; break;
    case x86_MM2: x86Command |= 2; break;
    case x86_MM3: x86Command |= 3; break;
    case x86_MM4: x86Command |= 4; break;
    case x86_MM5: x86Command |= 5; break;
    case x86_MM6: x86Command |= 6; break;
    case x86_MM7: x86Command |= 7; break;
    }
    PUTDST16(RecompPos, 0x650f);
    PUTDST8(RecompPos, 0xC0 | x86Command);
}
