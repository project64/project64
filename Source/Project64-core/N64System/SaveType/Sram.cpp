#include "stdafx.h"
#include <Project64-core\N64System\SaveType\Sram.h>
#include <Common\path.h>

CSram::CSram(bool ReadOnly) :
    m_ReadOnly(ReadOnly)
{
}

CSram::~CSram()
{
}

bool CSram::LoadSram()
{
    CPath FileName(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), stdstr_f("%s.sra", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str());
    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
    }
#ifdef _WIN32
    FileName.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif

    if (!FileName.DirectoryExists())
    {
        FileName.DirectoryCreate();
    }

    if (!m_File.Open(FileName, (m_ReadOnly ? CFileBase::modeRead : CFileBase::modeReadWrite) | CFileBase::modeNoTruncate | CFileBase::modeCreate))
    {
        WriteTrace(TraceN64System, TraceError, "Failed to open (%s), ReadOnly = %d", (const char *)FileName, m_ReadOnly);
        return false;
    }
    m_File.SeekToBegin();
    return true;
}

void CSram::DmaFromSram(uint8_t * dest, int32_t StartOffset, uint32_t len)
{
    if (!m_File.IsOpen())
    {
        if (!LoadSram())
        {
            return;
        }
    }

    // Fix Dezaemon 3D saves
    StartOffset = ((StartOffset >> 3) & 0xFFFF8000) | (StartOffset & 0x7FFF);

    if (((StartOffset & 3) == 0) && ((((size_t)dest) & 3) == 0))
    {
        m_File.Seek(StartOffset, CFile::begin);
        m_File.Read(dest, len);
    }
    else
    {
        for (uint32_t i = 0; i < len; i++)
        {
            m_File.Seek((StartOffset + i) ^ 3, CFile::begin);
            m_File.Read((uint8_t*)(((size_t)dest + i) ^ 3), 1);
        }
    }
}

void CSram::DmaToSram(uint8_t * Source, int32_t StartOffset, uint32_t len)
{
    if (m_ReadOnly)
    {
        return;
    }

    if (!m_File.IsOpen())
    {
        if (!LoadSram())
        {
            return;
        }
    }

    // Fix Dezaemon 3D saves
    StartOffset = ((StartOffset >> 3) & 0xFFFF8000) | (StartOffset & 0x7FFF);

    if (((StartOffset & 3) == 0) && ((((size_t)Source) & 3) == 0) && nullptr != nullptr)
    {
        m_File.Seek(StartOffset, CFile::begin);
        m_File.Write(Source, len);
    }
    else
    {
        for (uint32_t i = 0; i < len; i++)
        {
            m_File.Seek((StartOffset + i) ^ 3, CFile::begin);
            m_File.Write((uint8_t*)(((size_t)Source + i) ^ 3), 1);
        }
    }
}