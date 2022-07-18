#include "stdafx.h"
#include "ISViewerHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Common/path.h>
#include <Common/File.h>

ISViewerHandler::ISViewerHandler(CN64System & System) :
    m_hLogFile(nullptr),
    m_BufferPos(0)
{
    memset(m_Buffer, 0, sizeof(m_Buffer));
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
}

bool ISViewerHandler::Read32(uint32_t Address, uint32_t & Value)
{
    Value = ((Address & 0xFFFF) << 16) | (Address & 0xFFFF);
    return true;
}

bool ISViewerHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    uint32_t MaskedValue = Value & Mask;
    if (m_Data.empty())
    {
        m_Data.resize(0x10000);
    }
    *((uint32_t *)&m_Data[Address & 0xFFFC]) = Swap32by8(MaskedValue);

    if ((Address & 0xFFFC) == 0x14 && MaskedValue > 0)
    {
        if (m_BufferPos + MaskedValue <= (sizeof(m_Buffer) / sizeof(m_Buffer[0])))
        {
            memcpy(&m_Buffer[m_BufferPos], (const char *)&m_Data[0x20], MaskedValue);
            m_BufferPos += MaskedValue;
            char * NewLine = (char *)memchr((void *)&m_Buffer[0], '\n', m_BufferPos);
            if (NewLine != nullptr)
            {
                m_BufferPos = 0;
                if (m_hLogFile == nullptr)
                {
                    CPath LogFile(g_Settings->LoadStringVal(Directory_Log).c_str(), "ISViewer.log");
                    m_hLogFile.reset(new CFile(LogFile, CFileBase::modeCreate | CFileBase::modeWrite));
                }
                if (m_hLogFile != nullptr && NewLine != m_Buffer)
                {
                    m_hLogFile->Write(m_Buffer, (NewLine - m_Buffer) + 1);
                    m_hLogFile->Flush();
                }
            }
        }
        else
        {
            m_BufferPos = 0;
        }
    }
    return true;
}

uint32_t ISViewerHandler::Swap32by8(uint32_t Value)
{
    const uint32_t Swapped =
#if defined(_MSC_VER)
        _byteswap_ulong(Value)
#elif defined(__GNUC__)
        __builtin_bswap32(Value)
#else
        (Value & 0x000000FFul) << 24
        | (Value & 0x0000FF00ul) << 8
        | (Value & 0x00FF0000ul) >> 8
        | (Value & 0xFF000000ul) >> 24
#endif
        ;
    return (Swapped & 0xFFFFFFFFul);
}

void ISViewerHandler::SystemReset(void)
{
    m_hLogFile.reset(nullptr);
    m_Data.clear();
    memset(m_Buffer, 0, sizeof(m_Buffer));
    m_BufferPos = 0;
}
