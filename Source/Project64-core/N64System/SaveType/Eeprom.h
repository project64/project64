#pragma once
#include <Project64-core/Settings/DebugSettings.h>

class CEeprom :
    protected CDebugSettings
{
public:
    CEeprom(bool ReadOnly);
    ~CEeprom();

    void EepromCommand(uint8_t * Command);

private:
    CEeprom(void);
    CEeprom(const CEeprom&);
    CEeprom& operator=(const CEeprom&);

    void ProcessingError(uint8_t * Command);
    void LoadEeprom();
    void ReadFrom(uint8_t * Buffer, int32_t line);
    void WriteTo(uint8_t * Buffer, int32_t line);

    uint8_t m_EEPROM[0x800];
    bool    m_ReadOnly;
    CFile   m_File;
};
