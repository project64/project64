#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Multilanguage.h>

class CN64Rom :
    protected CDebugSettings
{
public:
    CN64Rom();
    ~CN64Rom();

    bool LoadN64Image(const char * FileLoc, bool LoadBootCodeOnly = false);
    bool LoadN64ImageIPL(const char * FileLoc, bool LoadBootCodeOnly = false);
    static bool IsValidRomImage(uint8_t Test[4]);
    bool IsLoadedRomDDIPL();
    void SaveRomSettingID(bool temp);
    void ClearRomSettingID();
    CICChip CicChipID();
    uint8_t * GetRomAddress() const { return m_ROMImage; }
    uint32_t GetRomSize() const { return m_RomFileSize; }
    const std::string & GetRomMD5() const { return m_MD5; }
    const std::string & GetRomName() const { return m_RomName; }
    const std::string & GetFileName() const { return m_FileName; }
    Country GetCountry() const { return m_Country; }
    bool IsPal();
    void UnallocateRomImage();

    // Get a message ID for the reason that you failed to load the ROM
    LanguageStringID GetError() const { return m_ErrorMsg; }
    static CICChip GetCicChipID(uint8_t * RomData, uint64_t * CRC = nullptr);
    static void CleanRomName(char * RomName, bool byteswap = true);

private:
    bool AllocateRomImage(uint32_t RomFileSize);
    bool AllocateAndLoadN64Image(const char * FileLoc, bool LoadBootCodeOnly);
    bool AllocateAndLoadZipImage(const char * FileLoc, bool LoadBootCodeOnly);
    void ByteSwapRom();
    void SetError(LanguageStringID ErrorMsg);
    void CalculateCicChip();
    void CalculateRomCrc();

    static void NotificationCB(const char * Status, CN64Rom * _this);

    // Constant values
    enum { ReadFromRomSection = 0x400000 };

    // Class variables
    CFile m_RomFile;
    uint8_t * m_ROMImage;
    uint8_t * m_ROMImageBase;
    uint32_t m_RomFileSize;
    Country m_Country;
    CICChip m_CicChip;
    LanguageStringID m_ErrorMsg;
    std::string m_RomName, m_FileName, m_MD5, m_RomIdent;
};
