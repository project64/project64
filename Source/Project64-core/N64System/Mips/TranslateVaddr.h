#pragma once

__interface CTransVaddr
{
    virtual bool TranslateVaddr  ( uint32_t VAddr, uint32_t &PAddr) const  = 0;
    virtual bool ValidVaddr      ( uint32_t VAddr ) const = 0;
    virtual bool VAddrToRealAddr ( uint32_t VAddr, void * &RealAddress ) const = 0;
};
