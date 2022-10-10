#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <stdint.h>

class CN64Disk
{
public:
    CN64Disk();
    ~CN64Disk();

    bool LoadDiskImage(const char * FileLoc);
    bool SaveDiskImage();
    void SwapDiskImage(const char * FileLoc);
    static bool IsValidDiskImage(uint8_t Test[0x20]);
    void SaveDiskSettingID(bool temp);
    void ClearDiskSettingID();
    uint8_t * GetDiskAddress()
    {
        return m_DiskImage;
    }
    uint8_t * GetDiskAddressBuffer()
    {
        return m_DiskImage + m_DiskBufAddress;
    }
    uint8_t * GetDiskAddressSys()
    {
        return m_DiskImage + m_DiskSysAddress;
    }
    uint8_t * GetDiskAddressID()
    {
        return m_DiskImage + m_DiskIDAddress;
    }
    uint8_t * GetDiskAddressRom()
    {
        return m_DiskImage + m_DiskRomAddress;
    }
    uint8_t * GetDiskAddressRam()
    {
        return m_DiskImage + m_DiskRamAddress;
    }
    uint32_t GetDiskSize() const
    {
        return m_DiskFileSize;
    }
    uint8_t * GetDiskHeader()
    {
        return m_DiskHeader;
    }
    void SetDiskAddressBuffer(uint32_t address)
    {
        m_DiskBufAddress = address;
    }
    uint32_t GetDiskAddressBlock(uint16_t head, uint16_t track, uint16_t block, uint16_t sector, uint16_t sectorsize);
    uint32_t CalculateCrc();
    stdstr GetRomName() const
    {
        return m_RomName;
    }
    stdstr GetFileName() const
    {
        return m_FileName;
    }
    stdstr GetDiskIdent() const
    {
        return m_DiskIdent;
    }
    Country GetCountry() const
    {
        return m_Country;
    }
    void UnallocateDiskImage();

    LanguageStringID GetError() const
    {
        return m_ErrorMsg;
    }

private:
    bool AllocateDiskImage(uint32_t DiskFileSize);
    bool AllocateDiskHeader();
    bool AllocateAndLoadDiskImage(const char * FileLoc);
    bool LoadDiskRAMImage();
    void ByteSwapDisk();
    void ForceByteSwapDisk();
    void SetError(LanguageStringID ErrorMsg);

    void DetectSystemArea();
    bool IsSysSectorGood(uint32_t block, uint32_t sectorsize);
    Country GetDiskCountryCode();
    void InitSysDataD64();
    void DeinitSysDataD64();
    void GenerateLBAToPhysTable();
    void DetectRamAddress();
    uint32_t LBAToVZone(uint32_t lba);
    uint32_t LBAToByte(uint32_t lba, uint32_t nlbas);
    uint16_t LBAToPhys(uint32_t lba);
    uint16_t PhysToLBA(uint16_t head, uint16_t track, uint16_t block);

    // Constant values
    enum
    {
        ReadFromRomSection = 0x400000,
        MameFormatSize = 0x0435B0C0,
        SDKFormatSize = 0x03DEC800,
        DiskFormatMAME = 0x0,
        DiskFormatSDK = 0x1,
        DiskFormatD64 = 0x2
    };

    // Class variables
    CFile m_DiskFile;
    uint8_t * m_DiskImage;
    uint8_t * m_DiskImageBase;
    uint8_t * m_DiskHeader;
    uint8_t * m_DiskHeaderBase;
    uint32_t m_DiskFileSize;
    uint32_t m_DiskBufAddress;
    uint32_t m_DiskSysAddress;
    uint32_t m_DiskIDAddress;
    uint32_t m_DiskRomAddress;
    uint32_t m_DiskRamAddress;
    LanguageStringID m_ErrorMsg;
    Country m_Country;
    stdstr m_RomName, m_FileName, m_DiskIdent;
    uint8_t m_DiskFormat; // 0 = MAME, 1 = SDK, 2 = D64
    uint8_t m_DiskType;
    bool m_isShadowDisk;

// Disk defines
#define MAX_LBA 0x10DB
#define SIZE_LBA MAX_LBA + 1
#define SYSTEM_LBAS 24
#define DISKID_LBA 14

#define DISK_COUNTRY_JPN 0xE848D316
#define DISK_COUNTRY_USA 0x2263EE56
#define DISK_COUNTRY_DEV 0x00000000

#define SECTORS_PER_BLOCK 85
#define BLOCKS_PER_TRACK 2

    const uint32_t SECTORSIZE[16] = {232, 216, 208, 192, 176, 160, 144, 128,
                                     216, 208, 192, 176, 160, 144, 128, 112};
    const uint32_t SECTORSIZE_P[9] = {232, 216, 208, 192, 176, 160, 144, 128, 112};
    const uint32_t ZoneTracks[16] = {158, 158, 149, 149, 149, 149, 149, 114,
                                     158, 158, 149, 149, 149, 149, 149, 114};

    const uint16_t VZONE_LBA_TBL[7][16] = {
        {0x0124, 0x0248, 0x035A, 0x047E, 0x05A2, 0x06B4, 0x07C6, 0x08D8, 0x09EA, 0x0AB6, 0x0B82, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x06A2, 0x07C6, 0x08D8, 0x09EA, 0x0AFC, 0x0BC8, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08C6, 0x09EA, 0x0AFC, 0x0C0E, 0x0CDA, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AEA, 0x0C0E, 0x0D20, 0x0DEC, 0x0EB8, 0x0FCA, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AD8, 0x0BEA, 0x0D0E, 0x0E32, 0x0EFE, 0x0FCA, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x0980, 0x0A92, 0x0BA4, 0x0CB6, 0x0DC8, 0x0EEC, 0x1010, 0x10DC},
        {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x093A, 0x0A4C, 0x0B5E, 0x0C70, 0x0D82, 0x0E94, 0x0FB8, 0x10DC},
    };

    const uint8_t VZONE_PZONE_TBL[7][16] = {
        {0x0, 0x1, 0x2, 0x9, 0x8, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA},
        {0x0, 0x1, 0x2, 0x3, 0xA, 0x9, 0x8, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB},
        {0x0, 0x1, 0x2, 0x3, 0x4, 0xB, 0xA, 0x9, 0x8, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC},
        {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xC, 0xB, 0xA, 0x9, 0x8, 0x6, 0x7, 0xF, 0xE, 0xD},
        {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0xF, 0xE},
        {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0xF},
        {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8},
    };

    const uint16_t SCYL_ZONE_TBL[2][8] = {
        {0x000, 0x09E, 0x13C, 0x1D1, 0x266, 0x2FB, 0x390, 0x425},
        {0x091, 0x12F, 0x1C4, 0x259, 0x2EE, 0x383, 0x418, 0x48A},
    };

    const uint16_t OUTERCYL_TBL[8] = {0x000, 0x09E, 0x13C, 0x1D1, 0x266, 0x2FB, 0x390, 0x425};

    const uint16_t RAM_START_LBA[7] = {0x5A2, 0x7C6, 0x9EA, 0xC0E, 0xE32, 0x1010, 0x10DC};

    const uint32_t RAM_SIZES[7] = {0x24A9DC0, 0x1C226C0, 0x1450F00, 0xD35680, 0x6CFD40, 0x1DA240, 0x0};

#define BLOCKSIZE(_zone) SECTORSIZE[_zone] * SECTORS_PER_BLOCK
#define TRACKSIZE(_zone) BLOCKSIZE(_zone) * BLOCKS_PER_TRACK
#define ZONESIZE(_zone) TRACKSIZE(_zone) * ZoneTracks[_zone]
#define VZONESIZE(_zone) TRACKSIZE(_zone) * (ZoneTracks[_zone] - 0xC)

#define VZoneToPZone(x, y) VZONE_PZONE_TBL[y][x]

    // Used for MAME format
    const uint32_t MAMEStartOffset[16] =
        {0x0, 0x5F15E0, 0xB79D00, 0x10801A0, 0x1523720, 0x1963D80, 0x1D414C0, 0x20BBCE0,
         0x23196E0, 0x28A1E00, 0x2DF5DC0, 0x3299340, 0x36D99A0, 0x3AB70E0, 0x3E31900, 0x4149200};

    // Used for SDK and D64 format
    uint16_t LBAToPhysTable[SIZE_LBA];
};