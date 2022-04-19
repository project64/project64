#pragma once
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\N64System\SaveType\FlashRam.h>
#include <Project64-core\N64System\SaveType\Sram.h>

class CDMA :
    private CDebugSettings
{
    CDMA();

public:
    void SP_DMA_WRITE();
    void PI_DMA_READ();
    void PI_DMA_WRITE();

protected:
    CDMA(CFlashram & FlashRam, CSram & Sram);

private:
    CDMA(const CDMA&);
    CDMA& operator=(const CDMA&);

    CFlashram & m_FlashRam;
    CSram     & m_Sram;

    void OnFirstDMA();
};
