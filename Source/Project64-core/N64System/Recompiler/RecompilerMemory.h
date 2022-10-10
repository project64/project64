#pragma once
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>

class CRecompMemory
{
protected:
    CRecompMemory();
    ~CRecompMemory();

    bool AllocateMemory();
    void CheckRecompMem();
    void Reset();
    void ShowMemUsed();

public:
    uint8_t ** RecompPos()
    {
        return &m_RecompPos;
    }

private:
    CRecompMemory(const CRecompMemory &);
    CRecompMemory & operator=(const CRecompMemory &);

    uint8_t * m_RecompCode;
    uint32_t m_RecompSize;
    uint8_t * m_RecompPos;

    enum
    {
        MaxCompileBufferSize = 0x03C00000
    };
    enum
    {
        InitialCompileBufferSize = 0x00500000
    };
    enum
    {
        IncreaseCompileBufferSize = 0x00100000
    };
};
