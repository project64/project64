// GameShark Pro / Pro Action Replay (N64) pass-through cartridge emulation
//
// Author: Nekokabu <nekokabu@gmail.com>
//
// Everything below is derived from static analysis of the v3.30 firmware
// ("(C) MUSHROOM &NU", "09:54 Mar 27") and from measurements on a real
// cartridge. No third party documentation of the hardware was used.
//
// Address map, as used by the firmware (X = value of the address latch,
// 0x10 after power on, 0x1E once the firmware menu is running):
//
//   0xX0000000 - 0xX000003F  firmware header (key code: CRC1/CRC2/entry point)
//   0xX0000040 - 0xX0000FFF  not answered, the IPL3 of the game cartridge shows
//   0xX0001000 - 0xX001FFFF  firmware
//   0xX0020000 - 0xX03FFFFF  reads as zero (measured on a real cartridge, the
//                            key code CRCs in the firmware only add up this way)
//   0xX0400000 + 0x000 (r)   status: bit 10 = GS button (0 = pressed),
//                            bit 4 / bits 3-0 = PC link strobe / data nibble
//   0xX0400000 + 0x400 (w)   address latch (0x10 | value & 0xF)
//   0xX0400000 + 0x600 (w)   PC link control
//   0xX0400000 + 0x800 (w)   7 segment LED shift register (bit 8 data, bit 9 clock)
//   0xX0500000 - 0xX05FFFFF  PC link (parallel port) output latches
//   0xX0600000 - 0xX07FFFFF  register writes here do not move the cartridge
//   0xX0C00000 - 0xX0DFFFFF  firmware flash, 256KB, read
//   0xX0E00000 - 0xX0FFFFFF  firmware flash, 16-bit word access with JEDEC
//                            software data protect commands (page write)
//
// Anything else is answered by the game cartridge.

#include "stdafx.h"

#include "GameSharkCart.h"
#include <chrono>
#include <stdio.h>
#include <string.h>

#ifndef GAMESHARK_STANDALONE
#include <Common/File.h>
#include <Common/path.h>
#endif

namespace
{
const char * const FirmwareName = "(C) MUSHROOM &NU";

const uint32_t Order_Z64 = 0x80371240; // Big endian (native)
const uint32_t Order_V64 = 0x37804012; // Byte swapped
const uint32_t Order_N64 = 0x40123780; // Little endian

enum
{
    KeyTableStart = 0x2FC00,
    KeyTableEnd = 0x30000,
    KeyEntrySize = 0x2C,
    KeyNameOffset = 13,
    KeyNameLen = 31,
};

enum
{
    // The firmware shows up to here in the boot window, zero after it
    BootWindowFirmwareEnd = 0x20000,
    BootChecksumStart = 0x1000,
    BootChecksumLen = 0x100000,
};

uint32_t FirstWord(const uint8_t * Data)
{
    return ((uint32_t)Data[0] << 24) | ((uint32_t)Data[1] << 16) | ((uint32_t)Data[2] << 8) | Data[3];
}

uint32_t OrderXor(uint32_t Order)
{
    switch (Order)
    {
    case Order_V64: return 1;
    case Order_N64: return 3;
    }
    return 0;
}

int64_t TimeMs(void)
{
    return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool ReadWholeFile(const char * FileName, std::vector<uint8_t> & Data, uint32_t MaxSize)
{
    Data.clear();
#ifdef GAMESHARK_STANDALONE
    FILE * fp = fopen(FileName, "rb");
    if (fp == nullptr)
    {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long Len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (Len <= 0 || (uint32_t)Len > MaxSize)
    {
        fclose(fp);
        return false;
    }
    Data.resize((size_t)Len);
    bool res = fread(Data.data(), 1, (size_t)Len, fp) == (size_t)Len;
    fclose(fp);
    return res;
#else
    CFile File;
    if (!File.Open(FileName, CFileBase::modeRead))
    {
        return false;
    }
    uint32_t Len = File.GetLength();
    if (Len == 0 || Len > MaxSize)
    {
        return false;
    }
    Data.resize(Len);
    File.Seek(0, CFileBase::begin);
    return File.Read(Data.data(), Len) == Len;
#endif
}

bool WriteWholeFile(const char * FileName, const std::vector<uint8_t> & Data)
{
#ifdef GAMESHARK_STANDALONE
    FILE * fp = fopen(FileName, "wb");
    if (fp == nullptr)
    {
        return false;
    }
    bool res = fwrite(Data.data(), 1, Data.size(), fp) == Data.size();
    fclose(fp);
    return res;
#else
    CFile File;
    if (!File.Open(FileName, CFileBase::modeWrite | CFileBase::modeCreate))
    {
        return false;
    }
    File.SetLength(0);
    return File.Write(Data.data(), (uint32_t)Data.size());
#endif
}

void RemoveFile(const char * FileName)
{
#ifdef GAMESHARK_STANDALONE
    remove(FileName);
#else
    CPath Path(FileName);
    if (Path.Exists())
    {
        Path.Delete();
    }
#endif
}

std::string IpsFileName(const char * FirmwareFile)
{
#ifdef GAMESHARK_STANDALONE
    std::string Name(FirmwareFile);
    size_t Dot = Name.find_last_of('.');
    size_t Slash = Name.find_last_of("/\\");
    if (Dot != std::string::npos && (Slash == std::string::npos || Dot > Slash))
    {
        Name.erase(Dot);
    }
    return Name + ".ips";
#else
    CPath Path(FirmwareFile);
    Path.SetExtension("ips");
    return (const char *)Path;
#endif
}
} // namespace

CGameSharkCart::CGameSharkCart() :
    m_FileOrder(Order_Z64),
    m_Loaded(false),
    m_Dirty(false),
    m_Base(0x10),
    m_RegPcControl(0),
    m_RegLed(0),
    m_FlashState(Flash_Idle),
    m_PageBase(0),
    m_PageCount(0),
    m_ButtonReads(0),
    m_ButtonExpire(0)
{
    memset(m_Flash, 0xFF, sizeof(m_Flash));
    memset(m_FlashHost, 0xFF, sizeof(m_FlashHost));
}

CGameSharkCart::~CGameSharkCart()
{
    SaveChanges();
}

bool CGameSharkCart::IsFirmwareImage(const uint8_t * Data, uint32_t Size)
{
    if (Data == nullptr || Size < 0x1000 || Size > FlashSize)
    {
        return false;
    }
    uint32_t Order = FirstWord(Data);
    if (Order != Order_Z64 && Order != Order_V64 && Order != Order_N64)
    {
        return false;
    }
    uint32_t Xor = OrderXor(Order);

    // Internal name
    for (uint32_t i = 0, n = (uint32_t)strlen(FirmwareName); i < n; i++)
    {
        if (Data[(0x20 + i) ^ Xor] != (uint8_t)FirmwareName[i])
        {
            return false;
        }
    }
    // The cartridge has no IPL3 of its own, it borrows the one of the game
    for (uint32_t i = 0x40; i < 0x1000; i++)
    {
        if (Data[i] != 0)
        {
            return false;
        }
    }
    return true;
}

bool CGameSharkCart::IsFirmwareFile(const char * FileName)
{
    std::vector<uint8_t> Data;
    if (FileName == nullptr || !ReadWholeFile(FileName, Data, FlashSize))
    {
        return false;
    }
    return IsFirmwareImage(Data.data(), (uint32_t)Data.size());
}

bool CGameSharkCart::LoadFirmwareData(const uint8_t * Data, uint32_t Size)
{
    if (!IsFirmwareImage(Data, Size))
    {
        return false;
    }
    m_OrigFile.assign(Data, Data + Size);
    m_FileOrder = FirstWord(Data);

    memset(m_Flash, 0xFF, sizeof(m_Flash));
    memset(m_FlashHost, 0xFF, sizeof(m_FlashHost));
    for (uint32_t i = 0; i < Size; i++)
    {
        SetFlashByte(FileIndex(i), Data[i]);
    }
    m_Loaded = true;
    m_Dirty = false;
    PowerOn();
    return true;
}

bool CGameSharkCart::LoadFirmware(const char * FileName)
{
    std::vector<uint8_t> Data;
    if (FileName == nullptr || !ReadWholeFile(FileName, Data, FlashSize))
    {
        return false;
    }
    if (!LoadFirmwareData(Data.data(), (uint32_t)Data.size()))
    {
        return false;
    }
    m_FirmwareFile = FileName;
    m_IpsFile = IpsFileName(FileName);

    // Changes made to the flash by earlier sessions
    std::vector<uint8_t> Ips;
    if (ReadWholeFile(m_IpsFile.c_str(), Ips, 0x1000000))
    {
        ApplyIps(Ips.data(), (uint32_t)Ips.size());
    }
    m_Dirty = false;
    return true;
}

uint32_t CGameSharkCart::FileIndex(uint32_t Offset) const
{
    return Offset ^ OrderXor(m_FileOrder);
}

void CGameSharkCart::FlashToFileOrder(std::vector<uint8_t> & Data) const
{
    Data.resize(FlashSize);
    for (uint32_t i = 0; i < FlashSize; i++)
    {
        Data[i] = m_Flash[FileIndex(i)];
    }
}

bool CGameSharkCart::ApplyIps(const uint8_t * Ips, uint32_t Size)
{
    if (!m_Loaded || Ips == nullptr || Size < 8 || memcmp(Ips, "PATCH", 5) != 0)
    {
        return false;
    }
    std::vector<uint8_t> File;
    FlashToFileOrder(File);

    uint32_t Pos = 5;
    bool FoundEnd = false;
    while (Pos + 3 <= Size)
    {
        if (memcmp(&Ips[Pos], "EOF", 3) == 0)
        {
            FoundEnd = true;
            break;
        }
        if (Pos + 5 > Size)
        {
            break;
        }
        uint32_t Offset = ((uint32_t)Ips[Pos] << 16) | ((uint32_t)Ips[Pos + 1] << 8) | Ips[Pos + 2];
        uint32_t Len = ((uint32_t)Ips[Pos + 3] << 8) | Ips[Pos + 4];
        Pos += 5;
        if (Len != 0)
        {
            if (Pos + Len > Size)
            {
                break;
            }
            for (uint32_t i = 0; i < Len; i++)
            {
                if (Offset + i < FlashSize)
                {
                    File[Offset + i] = Ips[Pos + i];
                }
            }
            Pos += Len;
        }
        else
        {
            // Run length encoded record
            if (Pos + 3 > Size)
            {
                break;
            }
            uint32_t RleLen = ((uint32_t)Ips[Pos] << 8) | Ips[Pos + 1];
            uint8_t Value = Ips[Pos + 2];
            Pos += 3;
            for (uint32_t i = 0; i < RleLen; i++)
            {
                if (Offset + i < FlashSize)
                {
                    File[Offset + i] = Value;
                }
            }
        }
    }
    if (!FoundEnd)
    {
        return false;
    }
    for (uint32_t i = 0; i < FlashSize; i++)
    {
        SetFlashByte(FileIndex(i), File[i]);
    }
    return true;
}

bool CGameSharkCart::BuildIps(std::vector<uint8_t> & Ips) const
{
    Ips.clear();
    if (!m_Loaded)
    {
        return false;
    }
    std::vector<uint8_t> File;
    FlashToFileOrder(File);

    // Bytes past the end of the original file count as different unless they are
    // still erased, the patch then grows the file to the full size of the flash
    uint32_t OrigSize = (uint32_t)m_OrigFile.size();
    std::vector<bool> Diff(FlashSize);
    bool AnyDiff = false;
    for (uint32_t i = 0; i < FlashSize; i++)
    {
        Diff[i] = i < OrigSize ? File[i] != m_OrigFile[i] : File[i] != 0xFF;
        AnyDiff = AnyDiff || Diff[i];
    }
    if (!AnyDiff)
    {
        return false;
    }

    Ips.insert(Ips.end(), {'P', 'A', 'T', 'C', 'H'});
    uint32_t Pos = 0;
    while (Pos < FlashSize)
    {
        if (!Diff[Pos])
        {
            Pos++;
            continue;
        }
        // Extend the record over short runs of unchanged bytes, a new record
        // costs 5 bytes of header
        uint32_t End = Pos + 1, Gap = 0;
        for (uint32_t i = Pos + 1; i < FlashSize && (i - Pos) < 0xFFFF; i++)
        {
            if (Diff[i])
            {
                End = i + 1;
                Gap = 0;
            }
            else if (++Gap > 5)
            {
                break;
            }
        }
        uint32_t Len = End - Pos;
        Ips.push_back((uint8_t)(Pos >> 16));
        Ips.push_back((uint8_t)(Pos >> 8));
        Ips.push_back((uint8_t)Pos);
        Ips.push_back((uint8_t)(Len >> 8));
        Ips.push_back((uint8_t)Len);
        Ips.insert(Ips.end(), File.begin() + Pos, File.begin() + End);
        Pos = End;
    }
    Ips.insert(Ips.end(), {'E', 'O', 'F'});
    return true;
}

bool CGameSharkCart::SaveChanges(void)
{
    if (!m_Loaded || !m_Dirty || m_IpsFile.empty())
    {
        return true;
    }
    m_Dirty = false;

    std::vector<uint8_t> Ips;
    if (!BuildIps(Ips))
    {
        // Flash is back to the content of the firmware file
        RemoveFile(m_IpsFile.c_str());
        return true;
    }
    return WriteWholeFile(m_IpsFile.c_str(), Ips);
}

void CGameSharkCart::PowerOn(void)
{
    SaveChanges();
    m_Base = 0x10;
    m_RegPcControl = 0;
    m_RegLed = 0;
    m_FlashState = Flash_Idle;
    m_PageBase = 0;
    m_PageCount = 0;
    m_ButtonReads = 0;
    m_ButtonExpire = 0;
}

void CGameSharkCart::EnterGameMode(void)
{
    SaveChanges();
    m_Base = 0x1E;
    m_FlashState = Flash_Idle;
    m_ButtonReads = 0;
}

void CGameSharkCart::PressButton(void)
{
    // The trainer samples the button once a frame and then waits for it to be
    // released, hold it for a couple of samples and never longer than half a second
    m_ButtonReads = 2;
    m_ButtonExpire = TimeMs() + 500;
}

CGameSharkCart::Region CGameSharkCart::Decode(uint32_t PAddr, uint32_t & Offset) const
{
    Offset = 0;
    if (!m_Loaded || (uint8_t)(PAddr >> 24) != m_Base)
    {
        return Region_None;
    }
    uint32_t Addr = PAddr & 0x00FFFFFF;
    if (Addr < 0x00400000)
    {
        if (Addr < 0x40)
        {
            Offset = Addr;
            return Region_Flash;
        }
        if (Addr < 0x1000)
        {
            return Region_None;
        }
        if (Addr < BootWindowFirmwareEnd)
        {
            Offset = Addr;
            return Region_Flash;
        }
        return Region_Zero;
    }
    if (Addr < 0x00800000)
    {
        Offset = Addr - 0x00400000;
        return Region_Reg;
    }
    if (Addr < 0x00C00000)
    {
        return Region_None;
    }
    if (Addr < 0x00E00000)
    {
        Offset = Addr & (FlashSize - 1);
        return Region_Flash;
    }
    Offset = Addr - 0x00E00000;
    return Region_FlashIO;
}

uint32_t CGameSharkCart::FlashWord32(uint32_t Offset) const
{
    Offset &= (FlashSize - 4);
    return ((uint32_t)m_Flash[Offset] << 24) | ((uint32_t)m_Flash[Offset + 1] << 16) | ((uint32_t)m_Flash[Offset + 2] << 8) | m_Flash[Offset + 3];
}

void CGameSharkCart::SetFlashByte(uint32_t Offset, uint8_t Value)
{
    if (Offset >= FlashSize)
    {
        return;
    }
    m_Flash[Offset] = Value;
    uint32_t Shift = (3 - (Offset & 3)) * 8;
    uint32_t & Word = m_FlashHost[Offset >> 2];
    Word = (Word & ~((uint32_t)0xFF << Shift)) | ((uint32_t)Value << Shift);
}

void CGameSharkCart::FlashReadAccess(void)
{
    // A page load ends when the host stops sending data, the firmware then polls
    // the toggle bit. Programming is instant here, the data reads back straight away
    if (m_FlashState == Flash_PageLoad)
    {
        m_FlashState = Flash_Idle;
    }
    if (m_Dirty)
    {
        SaveChanges();
    }
}

bool CGameSharkCart::Read32(uint32_t PAddr, uint32_t & Value)
{
    uint32_t Offset;
    switch (Decode(PAddr & ~3, Offset))
    {
    case Region_Flash:
        FlashReadAccess();
        Value = FlashWord32(Offset);
        return true;
    case Region_FlashIO:
        // Address bit 20 only selects the word on writes, both words are read at once
        FlashReadAccess();
        Value = FlashWord32(Offset & (FlashSize - 1));
        return true;
    case Region_Zero:
        Value = 0;
        return true;
    case Region_Reg:
        Value = RegRead(Offset);
        return true;
    default:
        break;
    }
    return false;
}

bool CGameSharkCart::ReadByte(uint32_t PAddr, uint8_t & Value)
{
    uint32_t Offset;
    switch (Decode(PAddr, Offset))
    {
    case Region_Flash:
        FlashReadAccess();
        Value = m_Flash[Offset];
        return true;
    case Region_FlashIO:
        FlashReadAccess();
        Value = m_Flash[Offset & (FlashSize - 1)];
        return true;
    case Region_Zero:
        Value = 0;
        return true;
    case Region_Reg:
        Value = (uint8_t)(RegRead(Offset & ~3) >> ((3 - (PAddr & 3)) * 8));
        return true;
    default:
        break;
    }
    return false;
}

uint8_t * CGameSharkCart::MemoryPtr(uint32_t PAddr, uint32_t Size)
{
    uint32_t Offset, OffsetEnd;
    if (Size == 0 || Decode(PAddr, Offset) != Region_Flash || Decode(PAddr + Size - 1, OffsetEnd) != Region_Flash)
    {
        return nullptr;
    }
    if (OffsetEnd != Offset + Size - 1)
    {
        return nullptr;
    }
    return ((uint8_t *)m_FlashHost) + Offset;
}

bool CGameSharkCart::Write32(uint32_t PAddr, uint32_t Value)
{
    uint32_t Offset;
    switch (Decode(PAddr & ~3, Offset))
    {
    case Region_Reg:
        // Only the first 16-bit data phase of a PI bus write is valid, the
        // firmware puts the same value in both halves
        RegWrite(Offset, (uint16_t)(Value >> 16));
        return true;
    case Region_FlashIO:
        // 16-bit flash word, address bit 20 stands in for the lowest address
        // line as the second half of the PI bus write carries no data
        FlashWrite(((Offset & 0x3FFFC) >> 1) | ((Offset >> 20) & 1), (uint16_t)(Value >> 16));
        return true;
    case Region_Flash:
    case Region_Zero:
        return true;
    default:
        break;
    }
    return false;
}

uint32_t CGameSharkCart::RegRead(uint32_t /*Offset*/)
{
    // Status: GS button released, no PC connected to the parallel port
    uint16_t Status = 0x0400;
    if (m_ButtonReads > 0)
    {
        if (TimeMs() < m_ButtonExpire)
        {
            m_ButtonReads -= 1;
            Status &= ~0x0400;
        }
        else
        {
            m_ButtonReads = 0;
        }
    }
    return ((uint32_t)Status << 16) | Status;
}

void CGameSharkCart::RegWrite(uint32_t Offset, uint16_t Value)
{
    if (Offset >= 0x00200000)
    {
        // 0x600000 block. The firmware writes the latch through here before it
        // looks for a second firmware at 0x12C00000 and keeps using the flash at
        // 0x1EE00000 afterwards, so these writes can not move the cartridge
        return;
    }
    if (Offset >= 0x00100000)
    {
        // PC link output latches, nothing is connected
        return;
    }
    switch (Offset & 0xE00)
    {
    case 0x400:
        if (m_Dirty)
        {
            SaveChanges();
        }
        m_Base = (uint8_t)(0x10 | (Value & 0x0F));
        break;
    case 0x600:
        m_RegPcControl = Value;
        break;
    case 0x800:
        m_RegLed = Value;
        break;
    }
}

void CGameSharkCart::FlashWrite(uint32_t WordIndex, uint16_t Value)
{
    WordIndex &= (FlashSize / 2) - 1;

    switch (m_FlashState)
    {
    case Flash_PageLoad:
        if (m_PageCount == 0)
        {
            // A page write erases the whole page before it is programmed
            m_PageBase = WordIndex & ~0x7F;
            for (uint32_t i = 0; i < 0x100; i++)
            {
                SetFlashByte((m_PageBase << 1) + i, 0xFF);
            }
        }
        if ((WordIndex & ~0x7F) == m_PageBase)
        {
            SetFlashByte(WordIndex << 1, (uint8_t)(Value >> 8));
            SetFlashByte((WordIndex << 1) + 1, (uint8_t)Value);
            m_Dirty = true;
        }
        m_PageCount += 1;
        if (m_PageCount >= 0x80)
        {
            m_FlashState = Flash_Idle;
        }
        return;
    case Flash_Idle:
        m_FlashState = (WordIndex == 0x5555 && Value == 0xAAAA) ? Flash_Unlock1 : Flash_Idle;
        return;
    case Flash_Unlock1:
        m_FlashState = (WordIndex == 0x2AAA && Value == 0x5555) ? Flash_Unlock2 : Flash_Idle;
        return;
    case Flash_Unlock2:
        m_FlashState = Flash_Idle;
        if (WordIndex == 0x5555)
        {
            switch (Value)
            {
            case 0xA0A0: // Page write
                m_FlashState = Flash_PageLoad;
                m_PageCount = 0;
                break;
            case 0x8080: // Erase prefix
                m_FlashState = Flash_Erase;
                break;
            }
        }
        return;
    case Flash_Erase:
        m_FlashState = (WordIndex == 0x5555 && Value == 0xAAAA) ? Flash_EraseUnlock1 : Flash_Idle;
        return;
    case Flash_EraseUnlock1:
        m_FlashState = (WordIndex == 0x2AAA && Value == 0x5555) ? Flash_EraseUnlock2 : Flash_Idle;
        return;
    case Flash_EraseUnlock2:
        if (WordIndex == 0x5555 && Value == 0x1010)
        {
            // Chip erase
            for (uint32_t i = 0; i < FlashSize; i++)
            {
                SetFlashByte(i, 0xFF);
            }
            m_Dirty = true;
        }
        m_FlashState = Flash_Idle;
        return;
    }
    m_FlashState = Flash_Idle;
}

uint32_t CGameSharkCart::HeaderWord(uint32_t Offset) const
{
    return FlashWord32(Offset & 0x3C);
}

std::string CGameSharkCart::ActiveKeyName(void) const
{
    if (!m_Loaded)
    {
        return "";
    }
    uint32_t Crc1 = HeaderWord(0x10), Crc2 = HeaderWord(0x14);
    for (uint32_t Entry = KeyTableStart; Entry + KeyEntrySize <= KeyTableEnd; Entry += KeyEntrySize)
    {
        if (FlashWord32(Entry) != Crc1 || FlashWord32(Entry + 4) != Crc2)
        {
            continue;
        }
        std::string Name;
        for (uint32_t i = 0; i < KeyNameLen; i++)
        {
            uint8_t ch = m_Flash[Entry + KeyNameOffset + i];
            if (ch == 0)
            {
                break;
            }
            if (ch < 0x20 || ch > 0x7E)
            {
                return "";
            }
            Name += (char)ch;
        }
        return Name;
    }
    return "";
}

uint32_t CGameSharkCart::ActiveKeyEntry(void) const
{
    // Entry point stored with the active key in the key table, 0 when the key is not in it.
    // It is not the entry point in the header, that one stays the same for every key
    uint32_t Crc1 = HeaderWord(0x10), Crc2 = HeaderWord(0x14);
    for (uint32_t Entry = KeyTableStart; Entry + KeyEntrySize <= KeyTableEnd; Entry += KeyEntrySize)
    {
        if (FlashWord32(Entry) == Crc1 && FlashWord32(Entry + 4) == Crc2)
        {
            return FlashWord32(Entry + 8);
        }
    }
    return 0;
}

int CGameSharkCart::ActiveKeyCic(void) const
{
    if (!m_Loaded)
    {
        return 0;
    }
    // CICs whose checksum does not depend on the game can be checked directly
    static const int CheckCic[] = {6102, 6103, 6106};
    for (size_t i = 0; i < sizeof(CheckCic) / sizeof(CheckCic[0]); i++)
    {
        uint32_t Crc1, Crc2;
        if (BootChecksum(CheckCic[i], nullptr, 0, Crc1, Crc2) && Crc1 == HeaderWord(0x10) && Crc2 == HeaderWord(0x14))
        {
            return CheckCic[i];
        }
    }
    // Otherwise go by the entry point of the key table entry
    switch (ActiveKeyEntry())
    {
    case 0x80190000: return 6105;
    case 0x80201000: return 5101; // Same entry as 6103, which was ruled out above
    }
    return 0;
}

std::string CGameSharkCart::ActiveKeyDescription(void) const
{
    if (!m_Loaded)
    {
        return "";
    }
    uint32_t Crc1 = HeaderWord(0x10), Crc2 = HeaderWord(0x14);
    int Cic = ActiveKeyCic();

    // Check digit: low byte of CRC1 + CRC2 + entry point + header words 0x30-0x3C
    // (matches every entry of the key table in the v3.30 firmware)
    uint32_t Entry = ActiveKeyEntry();
    if (Entry == 0)
    {
        switch (Cic)
        {
        case 6102: Entry = 0x80180000; break;
        case 6103: Entry = 0x80201000; break;
        case 6105: Entry = 0x80190000; break;
        case 6106: Entry = 0x80200400; break;
        }
    }

    char Text[64];
    if (Entry != 0)
    {
        uint32_t Sum = Crc1 + Crc2 + Entry + HeaderWord(0x30) + HeaderWord(0x34) + HeaderWord(0x38) + HeaderWord(0x3C);
        snprintf(Text, sizeof(Text), "%08X %08X %02X", Crc1, Crc2, Sum & 0xFF);
    }
    else
    {
        snprintf(Text, sizeof(Text), "%08X %08X", Crc1, Crc2);
    }

    std::string Description = ActiveKeyName();
    if (!Description.empty())
    {
        Description += " / ";
    }
    if (Cic != 0)
    {
        char CicText[16];
        snprintf(CicText, sizeof(CicText), "CIC-%d", Cic);
        Description += CicText;
    }
    else
    {
        Description += "CIC-?";
    }
    Description += " (";
    Description += Text;
    Description += ")";
    return Description;
}

uint32_t CGameSharkCart::BootWord(uint32_t Offset) const
{
    // What the IPL3 of the game reads from the cartridge while the latch is 0x10
    return Offset < BootWindowFirmwareEnd ? FlashWord32(Offset) : 0;
}

bool CGameSharkCart::BootChecksum(int Cic, const uint8_t * GameRom, uint32_t GameRomSize, uint32_t & Crc1, uint32_t & Crc2) const
{
    // The checksum the IPL3 of the game calculates over the first megabyte it
    // loads from 0x10001000. The results for 6102, 6103 and 6106 match the key
    // codes stored in the firmware, the 6105 variant needs the IPL3 itself
    uint32_t Seed;
    switch (Cic)
    {
    case 6101:
    case 6102: Seed = 0xF8CA4DDC; break;
    case 6103: Seed = 0xA3886759; break;
    case 6105: Seed = 0xDF26F436; break;
    case 6106: Seed = 0x1FEA617A; break;
    default: return false;
    }
    if (!m_Loaded || (Cic == 6105 && (GameRom == nullptr || GameRomSize < 0x1000)))
    {
        return false;
    }

    uint32_t t1 = Seed, t2 = Seed, t3 = Seed, t4 = Seed, t5 = Seed, t6 = Seed;
    for (uint32_t i = BootChecksumStart; i < BootChecksumStart + BootChecksumLen; i += 4)
    {
        uint32_t d = BootWord(i);
        if ((uint32_t)(t6 + d) < t6)
        {
            t4 += 1;
        }
        t6 += d;
        t3 ^= d;
        uint32_t Rot = d & 0x1F;
        uint32_t r = Rot != 0 ? ((d << Rot) | (d >> (32 - Rot))) : d;
        t5 += r;
        if (t2 > d)
        {
            t2 ^= r;
        }
        else
        {
            t2 ^= t6 ^ d;
        }
        if (Cic == 6105)
        {
            // Game ROM is stored as host order words
            uint32_t Ipl3Word;
            memcpy(&Ipl3Word, &GameRom[0x40 + 0x710 + (i & 0xFF)], sizeof(Ipl3Word));
            t1 += Ipl3Word ^ d;
        }
        else
        {
            t1 += t5 ^ d;
        }
    }
    switch (Cic)
    {
    case 6103:
        Crc1 = (t6 ^ t4) + t3;
        Crc2 = (t5 ^ t2) + t1;
        break;
    case 6106:
        Crc1 = (t6 * t4) + t3;
        Crc2 = (t5 * t2) + t1;
        break;
    default:
        Crc1 = t6 ^ t4 ^ t3;
        Crc2 = t5 ^ t2 ^ t1;
        break;
    }
    return true;
}

CGameSharkCart::KeyCheck CGameSharkCart::CheckKey(int Cic, const uint8_t * GameRom, uint32_t GameRomSize) const
{
    uint32_t Crc1, Crc2;
    if (!BootChecksum(Cic, GameRom, GameRomSize, Crc1, Crc2))
    {
        return KeyCheck_Unknown;
    }
    return (Crc1 == HeaderWord(0x10) && Crc2 == HeaderWord(0x14)) ? KeyCheck_Match : KeyCheck_Mismatch;
}
