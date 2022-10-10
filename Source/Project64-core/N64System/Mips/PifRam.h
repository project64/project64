#pragma once

#include <Project64-core\Logging.h>
#include <Project64-core\N64System\SaveType\Eeprom.h>

class CPifRam :
    public CLogging,
    private CEeprom
{
public:
    CPifRam(bool SavesReadOnly);
    ~CPifRam();

    void Reset();

    void PifRamWrite();
    void PifRamRead();

    void SI_DMA_READ();
    void SI_DMA_WRITE();

protected:
    uint8_t m_PifRom[0x7C0];
    uint8_t m_PifRam[0x40];

private:
    CPifRam();
    CPifRam(const CPifRam &);
    CPifRam & operator=(const CPifRam &);

    enum
    {
        CHALLENGE_LENGTH = 0x20
    };
    void ProcessControllerCommand(int32_t Control, uint8_t * Command);
    void ReadControllerCommand(int32_t Control, uint8_t * Command);
    void LogControllerPakData(const char * Description);
    void n64_cic_nus_6105(char challenge[], char response[], int32_t length);
};
