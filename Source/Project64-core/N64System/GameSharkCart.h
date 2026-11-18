#pragma once
// GameShark Pro / Pro Action Replay (N64) pass-through cartridge emulation
//
// Author: Nekokabu <nekokabu@gmail.com>
//
// The hardware model in this file is derived only from static analysis of the
// v3.30 firmware ("09:54 Mar 27", internal name "(C) MUSHROOM &NU") and from
// measurements taken on a real cartridge. See Docs/GameShark.md for details.

#include <stdint.h>
#include <string>
#include <vector>

class CGameSharkCart
{
public:
    enum
    {
        FlashSize = 0x40000, // 2 x 128KB page-write flash, seen as 16-bit wide
    };

    enum KeyCheck
    {
        KeyCheck_Match,    // The active key code boots with this game
        KeyCheck_Mismatch, // The game's IPL3 will reject the active key code
        KeyCheck_Unknown,  // No checksum model for this CIC, can not tell
    };

    CGameSharkCart();
    ~CGameSharkCart();

    // Firmware detection (any of the three ROM byte orders)
    static bool IsFirmwareImage(const uint8_t * Data, uint32_t Size);
    static bool IsFirmwareFile(const char * FileName);

    // Firmware image, the file on disk is never modified
    bool LoadFirmware(const char * FileName);
    bool LoadFirmwareData(const uint8_t * Data, uint32_t Size);
    bool ApplyIps(const uint8_t * Ips, uint32_t Size);
    bool BuildIps(std::vector<uint8_t> & Ips) const;
    bool SaveChanges(void);
    const std::string & FirmwareFile(void) const
    {
        return m_FirmwareFile;
    }
    const std::string & IpsFile(void) const
    {
        return m_IpsFile;
    }

    // Cartridge state
    void PowerOn(void);       // Power on / reset, cartridge answers at 0x10000000
    void EnterGameMode(void); // State as left by the firmware once it is running
    void PressButton(void);   // GS button
    uint8_t Base(void) const
    {
        return m_Base;
    }

    // PI bus access, PAddr in 0x10000000-0x1FFFFFFF. The functions return false
    // when the access is not answered by the GameShark, in which case it belongs
    // to the game cartridge plugged in to the top of it
    bool Read32(uint32_t PAddr, uint32_t & Value);
    bool Write32(uint32_t PAddr, uint32_t Value);
    bool ReadByte(uint32_t PAddr, uint8_t & Value);
    uint8_t * MemoryPtr(uint32_t PAddr, uint32_t Size);

    // Key code information
    uint32_t HeaderWord(uint32_t Offset) const;
    std::string ActiveKeyName(void) const;
    // Active key code for display, e.g. "Zelda / CIC-6105 (A9245352 5F737737 7D)"
    std::string ActiveKeyDescription(void) const;
    int ActiveKeyCic(void) const; // 6102, 6103, 6105, 6106, 5101 or 0 when unknown
    KeyCheck CheckKey(int Cic, const uint8_t * GameRom, uint32_t GameRomSize) const;
    bool BootChecksum(int Cic, const uint8_t * GameRom, uint32_t GameRomSize, uint32_t & Crc1, uint32_t & Crc2) const;

private:
    CGameSharkCart(const CGameSharkCart &);
    CGameSharkCart & operator=(const CGameSharkCart &);

    enum Region
    {
        Region_None,    // Not answered by the GameShark
        Region_Flash,   // Firmware (read)
        Region_Zero,    // Answered, reads as zero
        Region_Reg,     // Registers
        Region_FlashIO, // Flash command / word access
    };

    enum FlashState
    {
        Flash_Idle,
        Flash_Unlock1,
        Flash_Unlock2,
        Flash_PageLoad,
        Flash_Erase,
        Flash_EraseUnlock1,
        Flash_EraseUnlock2,
    };

    Region Decode(uint32_t PAddr, uint32_t & Offset) const;
    uint32_t FlashWord32(uint32_t Offset) const;
    void SetFlashByte(uint32_t Offset, uint8_t Value);
    void FlashWrite(uint32_t WordIndex, uint16_t Value);
    void FlashReadAccess(void);
    void RegWrite(uint32_t Offset, uint16_t Value);
    uint32_t RegRead(uint32_t Offset);
    uint32_t BootWord(uint32_t Offset) const;
    uint32_t ActiveKeyEntry(void) const;
    uint32_t FileIndex(uint32_t Offset) const;
    void FlashToFileOrder(std::vector<uint8_t> & Data) const;

    std::string m_FirmwareFile;
    std::string m_IpsFile;
    std::vector<uint8_t> m_OrigFile;     // Firmware file exactly as stored on disk
    uint32_t m_FileOrder;                // First word of the file, tells the byte order
    uint8_t m_Flash[FlashSize];          // Big endian byte order
    uint32_t m_FlashHost[FlashSize / 4]; // Same data as host order words (opcode fetch)
    bool m_Loaded;
    bool m_Dirty;

    uint8_t m_Base;          // Address latch, bits 31-24 the cartridge answers at
    uint16_t m_RegPcControl; // Register 0x600
    uint16_t m_RegLed;       // Register 0x800
    FlashState m_FlashState;
    uint32_t m_PageBase;
    uint32_t m_PageCount;
    uint32_t m_ButtonReads;
    int64_t m_ButtonExpire;
};
