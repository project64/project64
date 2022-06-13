#pragma once
#include <Project64-core\Settings\DebugSettings.h>

class CartridgeDomain2Address2Handler;

class CDMA :
    private CDebugSettings
{
    CDMA();

public:
    void PI_DMA_READ();
    void PI_DMA_WRITE();

protected:
    CDMA(CartridgeDomain2Address2Handler & Domain2Address2Handler);

private:
    CDMA(const CDMA&);
    CDMA& operator=(const CDMA&);

    void OnFirstDMA();

    CartridgeDomain2Address2Handler & m_Domain2Address2Handler;
};
