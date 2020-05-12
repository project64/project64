/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "MemoryScanner.h"

CMixed::TypeNameEntry CMixed::TypeNames[] = {
    { "uint8",   ValueType_uint8 },
    { "int8",    ValueType_int8 },
    { "uint16",  ValueType_uint16 },
    { "int16",   ValueType_int16 },
    { "uint32",  ValueType_uint32 },
    { "int32",   ValueType_int32 },
    { "uint64",  ValueType_uint64 },
    { "int64",   ValueType_int64 },
    { "float",   ValueType_float },
    { "double",  ValueType_double },
    { "char",    ValueType_string },
    { "char",    ValueType_unkstring },
    { "char",    ValueType_unkstring },
    { NULL,      ValueType_invalid}
};

const char* CMixed::GetTypeName(void)
{
    switch (m_Type)
    {
    case ValueType_uint8:  return "uint8";
    case ValueType_int8:   return "int8";
    case ValueType_uint16: return "uint16";
    case ValueType_int16:  return "int16";
    case ValueType_uint32: return "uint32";
    case ValueType_int32:  return "int32";
    case ValueType_uint64: return "uint64";
    case ValueType_int64:  return "int64";
    case ValueType_float:  return "float";
    case ValueType_double: return "double";
    case ValueType_string:
    case ValueType_istring:
    case ValueType_unkstring:
        return "char";
    }

    return NULL;
}

ValueType CMixed::GetTypeFromString(const char* name, int* charArrayLength)
{
    for (int i = 0; TypeNames[i].name != NULL; i++)
    {
        if (strcmp(name, TypeNames[i].name) == 0)
        {
            *charArrayLength = 0;
            return TypeNames[i].type;
        }
    }

    if (sscanf(name, "char[%d]", charArrayLength) != 0)
    {
        return ValueType_string;
    }

    return ValueType_invalid;
}

int CMixed::GetTypeSize(void)
{
    switch (m_Type)
    {
    case ValueType_uint8:  return sizeof(uint8_t);
    case ValueType_int8:   return sizeof(int8_t);
    case ValueType_uint16: return sizeof(uint16_t);
    case ValueType_int16:  return sizeof(int16_t);
    case ValueType_uint32: return sizeof(uint32_t);
    case ValueType_int32:  return sizeof(int32_t);
    case ValueType_uint64: return sizeof(uint64_t);
    case ValueType_int64:  return sizeof(int64_t);
    case ValueType_float:  return sizeof(float);
    case ValueType_double: return sizeof(double);
    case ValueType_string:
    case ValueType_istring:
    case ValueType_unkstring:
        return m_StrLength;
    default:
        return 0;
    }
}

bool CMixed::IsStringType(void)
{
    switch (m_Type)
    {
    case ValueType_string:
    case ValueType_istring:
    case ValueType_unkstring:
        return true;
    }

    return false;
}

int CMixed::ToString(char* buffer, bool bHex, size_t size)
{
    if (bHex)
    {
        switch (m_Type)
        {
        case ValueType_uint8:
        case ValueType_int8:
            return snprintf(buffer, size, "0x%02X", m_Value._uint8);
        case ValueType_uint16:
        case ValueType_int16:
            return snprintf(buffer, size, "0x%04X", m_Value._uint16);
        case ValueType_uint32:
        case ValueType_int32:
        case ValueType_float:
            return snprintf(buffer, size, "0x%08X", m_Value._uint32);
        case ValueType_uint64:
        case ValueType_int64:
        case ValueType_double:
            return snprintf(buffer, size, "0x%016llX", m_Value._uint64);
        default:
            return snprintf(buffer, size, "?");
        }
    }

    switch (m_Type)
    {
    case ValueType_uint8:  return snprintf(buffer, size, "%d", m_Value._uint8);
    case ValueType_int8:   return snprintf(buffer, size, "%d", m_Value._sint8);
    case ValueType_uint16: return snprintf(buffer, size, "%d", m_Value._uint16);
    case ValueType_int16:  return snprintf(buffer, size, "%d", m_Value._sint16);
    case ValueType_uint32: return snprintf(buffer, size, "%lu", m_Value._uint32);
    case ValueType_int32:  return snprintf(buffer, size, "%d", m_Value._sint32);
    case ValueType_uint64: return snprintf(buffer, size, "%llu", m_Value._uint64);
    case ValueType_int64:  return snprintf(buffer, size, "%lld", m_Value._sint64);
    case ValueType_float:  return snprintf(buffer, size, "%f", m_Value._float);
    case ValueType_double: return snprintf(buffer, size, "%f", m_Value._double);
    default: return snprintf(buffer, size, "?");
    }
}

CScanResult::CScanResult(AddressType addressType, DisplayFormat displayFormat) :
    m_AddressType(addressType),
    m_Address(0),
    m_DisplayFormat(displayFormat),
    m_bSelected(false),
    m_Description(NULL)
{
}

CScanResult::~CScanResult(void)
{
}

void CScanResult::SetDescription(const char* str)
{
    if (m_Description != NULL)
    {
        free(m_Description);
    }

    size_t len = strlen(str);
    m_Description = (char*)malloc(len + 1);
    strcpy(m_Description, str);
    m_Description[len] = '\0';
}

void CScanResult::DeleteDescription(void)
{
    if (m_Description != NULL)
    {
        free(m_Description);
        m_Description = NULL;
    }
}

const char* CScanResult::GetDescription(void)
{
    if (m_Description == NULL)
    {
        return "";
    }
    return m_Description;
}

int CScanResult::GetValueString(char *buffer, size_t size)
{
    bool bHex = (m_DisplayFormat == DisplayHex);
    return ToString(buffer, bHex, size);
}

bool CScanResult::GetMemoryValue(CMixed* v)
{
    if (g_MMU == NULL)
    {
        return false;
    }

    uint32_t paddr = m_Address & 0x1FFFFFFF;

    if (!CMemoryScanner::PAddrValid(paddr))
    {
        return false;
    }

    uint8_t* mem = CMemoryScanner::GetMemoryPool(paddr);

    uint64_t raw64 = 0;

    if (GetTypeSize() == 8)
    {
        raw64 = ((uint64_t)*(uint32_t*)&mem[paddr] << 32) | *(uint32_t*)&mem[paddr + 4];
    }

    switch (m_Type)
    {
    case ValueType_uint8:  v->Set(*(uint8_t*) &mem[paddr ^ 3]); break;
    case ValueType_int8:   v->Set(*(int8_t*)  &mem[paddr ^ 3]); break;
    case ValueType_uint16: v->Set(*(uint16_t*)&mem[paddr ^ 2]); break;
    case ValueType_int16:  v->Set(*(int16_t*) &mem[paddr ^ 2]); break;
    case ValueType_uint32: v->Set(*(uint32_t*)&mem[paddr]); break;
    case ValueType_int32:  v->Set(*(int32_t*)&mem[paddr]); break;
    case ValueType_uint64: v->Set(*(uint64_t*)&raw64); break;
    case ValueType_int64:  v->Set(*(int64_t*)&raw64); break;
    case ValueType_float:  v->Set(*(float*)&mem[paddr]); break;
    case ValueType_double: v->Set(*(double*)&raw64); break;
    default: return false; // (primitives only)
    }

    return true;
}

int CScanResult::GetMemoryValueString(char* buffer, size_t size, bool bIgnoreHex)
{
    if (g_MMU == NULL)
    {
        sprintf(buffer, "?");
        return 1;
    }
    
    bool bHex = (m_DisplayFormat == DisplayHex) && !bIgnoreHex;

    uint32_t paddr = m_Address & 0x1FFFFFFF;

    if (!CMemoryScanner::PAddrValid(paddr))
    {
        return sprintf(buffer, "?");
    }

    uint8_t* mem = CMemoryScanner::GetMemoryPool(paddr);

    if (m_Type == ValueType_istring ||
        m_Type == ValueType_string  ||
        m_Type == ValueType_unkstring)
    {
        if (bHex)
        {
            char* out = buffer;

            for (int i = 0; i < m_StrLength; i++)
            {
                uint32_t ipaddr = (paddr + i) ^ 3;
                if (i != 0) out += sprintf(out, " ");
                out += sprintf(out, "%02X", mem[ipaddr]);
            }
            *out = '\0';
            return out - buffer;
        }
        else
        {
            for (int i = 0; i < m_StrLength; i++)
            {
                uint32_t ipaddr = (paddr + i) ^ 3;
                buffer[i] = mem[ipaddr];
            }
            buffer[m_StrLength] = '\0';
            return m_StrLength;
        }
    }

    CMixed memVal;
    if (!GetMemoryValue(&memVal))
    {
        return 0;
    }

    return memVal.ToString(buffer, bHex, size);
}

int CScanResult::GetAddressString(char *buffer)
{
    return sprintf(buffer, "0x%08X", m_Address);
}

uint32_t CScanResult::GetVirtualAddress(void)
{
    if (m_AddressType == AddressType_Virtual)
    {
        return m_Address;
    }
    else
    {
        // convert physical to virtual kseg0
        return (m_Address | 0x80000000);
    }
}

bool CScanResult::SetMemoryValueFromString(const char* str)
{
    if (g_MMU == NULL)
    {
        //sprintf(buffer, "?");
        return false;
    }

    bool bHex = (m_DisplayFormat == DisplayHex);

    uint32_t paddr = m_Address & 0x1FFFFFFF;

    if (!CMemoryScanner::PAddrValid(paddr))
    {
        return false;
    }

    uint8_t* mem = CMemoryScanner::GetMemoryPool(m_Address & 0x1FFFFFFF);

    char* endptr;
    uint64_t intVal = strtoull(str, &endptr, 0);
    double doubleVal = strtod(str, &endptr);

    switch (m_Type)
    {
    case ValueType_uint8:
    case ValueType_int8:
        mem[paddr ^ 3] = intVal & 0xFF;
        break;
    case ValueType_uint16:
    case ValueType_int16:
        *(uint16_t*)&mem[paddr ^ 2] = intVal & 0xFFFF;
        break;
    case ValueType_uint32:
    case ValueType_int32:
        *(uint32_t*)&mem[paddr] = intVal & 0xFFFFFFFF;
        break;
    case ValueType_uint64:
    case ValueType_int64:
        *(uint64_t*)&mem[paddr] = (intVal << 32) | (intVal >> 32);
        break;
    case ValueType_float:
        if (bHex)
        {
            *(uint32_t*)&mem[paddr] = intVal & 0xFFFFFFFF;
            break;
        }
        *(float*)&mem[paddr] = (float)doubleVal;
        break;
    case ValueType_double:
        if (bHex)
        {
            *(uint64_t*)&mem[paddr] = (intVal << 32) | (intVal >> 32);
            break;
        }
        intVal = *(uint64_t*)&doubleVal;
        *(uint64_t*)&mem[paddr] = (intVal << 32) | (intVal >> 32);
        break;
    case ValueType_string:
    case ValueType_istring:
    case ValueType_unkstring:
        if (bHex)
        {
            int size = CMemoryScanner::ParseHexString(NULL, str);
            if (size == 0)
            {
                return false;
            }

            char* buff = new char[size];
            CMemoryScanner::ParseHexString(buff, str);

            for (int i = 0; i < m_StrLength; i++)
            {
                uint32_t ipaddr = (paddr + i) ^ 3;
                mem[ipaddr] = buff[i];
            }

            delete[] buff;
        }
        else
        {
            for (int i = 0; i < m_StrLength; i++)
            {
                uint32_t ipaddr = (paddr + i) ^ 3;
                mem[ipaddr] = str[i];
            }
        }
        break;
    }

    return true;
}

bool CScanResult::SetAddressSafe(uint32_t address)
{
    if (!g_MMU || !g_Rom)
    {
        return false;
    }

    uint32_t ramSize = g_MMU->RdramSize();
    uint32_t romSize = g_Rom->GetRomSize();

    uint32_t paddrStart = address & 0x1FFFFFFF;
    uint32_t paddrEnd = (paddrStart + GetTypeSize()) - 1;

    if (m_AddressType == AddressType_Virtual)
    {
        if (!CMemoryScanner::AddrCheck(address, 0x80000000, 0xBFFFFFFF))
        {
            return false;
        }
    }

    if (!CMemoryScanner::PAddrRangeValid(paddrStart, paddrEnd))
    {
        return false;
    }

    if (!CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x00000000, ramSize - 1) &&
        !CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x10000000, 0x10000000 + romSize - 1) &&
        !CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x04000000, 0x04001FFF))
    {
        return false;
    }

    m_Address = address;

    return true;
}

bool CScanResult::SetStrLengthSafe(int length)
{
    if (!g_MMU || !g_Rom)
    {
        return false;
    }

    uint32_t ramSize = g_MMU->RdramSize();
    uint32_t romSize = g_Rom->GetRomSize();

    uint32_t paddrStart = m_Address & 0x1FFFFFFF;
    uint32_t paddrEnd = (paddrStart + length) - 1;

    if (!CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x00000000, ramSize - 1) &&
        !CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x10000000, 0x10000000 + romSize - 1) &&
        !CMemoryScanner::RangeCheck(paddrStart, paddrEnd, 0x04000000, 0x04001FFF))
    {
        return false;
    }

    m_StrLength = length;

    return true;
}

//bool CScanResult::IsSelected(void)
//{
//    return m_bSelected;
//}
//
//void CScanResult::SetSelected(bool bSelected)
//{
//    m_bSelected = bSelected;
//}


/*********************/

CMemoryScanner::CMemoryScanner(void) :
    m_DidFirstScan(false),
    m_ValueType(ValueType_uint8),
    m_StringValueLength(false),
    m_bDataTypePrimitive(true),
    m_SearchType(SearchType_ExactValue),
    m_AddressType(AddressType_Virtual),
    m_VAddrBits(0x80000000),
    m_Memory(NULL)
{
    m_Value._uint64 = 0;
    SetAddressRange(0x80000000, 0x803FFFFF);
}

bool CMemoryScanner::RangeCheck(uint32_t addrStart, uint32_t addrEnd, uint32_t rangeStart, uint32_t rangeEnd)
{
    return (addrStart <= addrEnd) && (addrStart >= rangeStart) && (addrEnd <= rangeEnd);
}

bool CMemoryScanner::AddrCheck(uint32_t addr, uint32_t rangeStart, uint32_t rangeEnd)
{
    return (addr >= rangeStart) && (addr <= rangeEnd);
}

bool CMemoryScanner::PAddrValid(uint32_t physAddr)
{
    if (g_MMU == NULL || g_Rom == NULL)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    uint32_t ramSize = g_MMU->RdramSize();
    uint32_t romSize = g_Rom->GetRomSize();

    return (AddrCheck(physAddr, 0x00000000, 0x00000000 + ramSize - 1) ||
        AddrCheck(physAddr, 0x10000000, 0x10000000 + romSize - 1) ||
        AddrCheck(physAddr, 0x04000000, 0x04001FFF));
}

bool CMemoryScanner::PAddrRangeValid(uint32_t physAddrStart, uint32_t physAddrEnd)
{
    return (RangeCheck(physAddrStart, physAddrEnd, 0x00000000, g_MMU->RdramSize()) ||
        RangeCheck(physAddrStart, physAddrEnd, 0x04000000, 0x04001FFF) ||
        RangeCheck(physAddrStart, physAddrEnd, 0x10000000, 0x15FFFFFF));
}

void CMemoryScanner::SetAddressType(AddressType addressType)
{
    m_AddressType = addressType;
}

void CMemoryScanner::Reset(void)
{
    m_DidFirstScan = false;

    m_ValueType = ValueType_uint8;
    m_SearchType = SearchType_ExactValue;
    
    m_Results.clear();
}

bool CMemoryScanner::SetAddressRange(uint32_t startAddress, uint32_t endAddress)
{
    if (!g_MMU || !g_Rom)
    {
        return false;
    }

    if(m_DidFirstScan)
    {
        return false;
    }

    if (m_AddressType == AddressType_Virtual)
    {
        m_VAddrBits = startAddress & 0xE0000000;

        // don't allow TLB
        if (!RangeCheck(startAddress, endAddress, 0x80000000, 0xBFFFFFFF))
        {
            return false;
        }

        // use physical addresses internally
        startAddress = startAddress & 0x1FFFFFFF;
        endAddress = endAddress & 0x1FFFFFFF;
    }
    else
    {
        m_VAddrBits = 0;
    }

    if (RangeCheck(startAddress, endAddress, 0x00000000, 0x007FFFFF))
    {
        if (endAddress >= g_MMU->RdramSize())
        {
            return false;
        }

        m_Memory = g_MMU->Rdram();
    }
    else if (RangeCheck(startAddress, endAddress, 0x04000000, 0x04001FFF))
    {
        m_Memory = g_MMU->Rdram();
    }
    else if (RangeCheck(startAddress, endAddress, 0x10000000, 0x10FFFFFF))
    {
        if ((endAddress - 0x10000000) >= g_Rom->GetRomSize())
        {
            return false;
        }

        m_Memory = (g_Rom->GetRomAddress() - 0x10000000);
    }
    else
    {
        return false; // invalid range
    }
    
    m_Memory = GetMemoryPool(startAddress);

    m_RangeStartAddress = startAddress;
    m_RangeEndAddress = endAddress;
    return true;
}

uint8_t* CMemoryScanner::GetMemoryPool(uint32_t physAddr)
{
    if (!g_MMU || !g_Rom)
    {
        return NULL;
    }

    if ((physAddr >= 0x00000000 && physAddr < g_MMU->RdramSize()) ||
        (physAddr >= 0x04000000 && physAddr <= 0x04001FFF))
    {
        return g_MMU->Rdram();
    }
    else if (physAddr >= 0x10000000 && physAddr <= 0x18000000)
    {
        return (g_Rom->GetRomAddress() - 0x10000000);
    }
    else
    {
        return NULL;
    }

}

bool CMemoryScanner::SetValueType(ValueType type)
{
    if(m_DidFirstScan)
    {
        return false;
    }
    
    switch(type)
    {
    case ValueType_string:
    case ValueType_istring:
    case ValueType_unkstring:
        m_bDataTypePrimitive = false;
        break;
    default:
        m_bDataTypePrimitive = true;
        break;
    }
    
    m_ValueType = type;
    return true;
}

void CMemoryScanner::SetStringValueLength(int length)
{
    m_StringValueLength = length;
}

bool CMemoryScanner::SetSearchType(SearchType searchType)
{
    if(!m_bDataTypePrimitive)
    {
        return false;
    }
    
    switch(searchType)
    {
    case SearchType_UnknownValue:
    case SearchType_JalTo:
        if (m_DidFirstScan)
        {
            return false;
        }
        break;
    case SearchType_ChangedValue:
    case SearchType_UnchangedValue:
    case SearchType_IncreasedValue:
    case SearchType_DecreasedValue:
        if(!m_DidFirstScan)
        {
            return false;
        }
        break;
    }
    
    m_SearchType = searchType;
    return true;
}

bool CMemoryScanner::DidFirstScan(void)
{
    return m_DidFirstScan;
}

size_t CMemoryScanner::GetNumResults(void)
{
    return m_Results.size();
}

CScanResult* CMemoryScanner::GetResult(size_t index)
{
    if (index >= m_Results.size())
    {
        return NULL;
    }

    return &m_Results[index];
}    

void CMemoryScanner::RemoveResult(size_t index)
{
    if (index >= m_Results.size())
    {
        return;
    }

    m_Results.erase(m_Results.begin() + index);
}

// scan for text or hex array
void CMemoryScanner::FirstScanLoopString(DisplayFormat resultDisplayFormat)
{
    int length = m_StringValueLength;

    uint32_t startAddr = m_RangeStartAddress;
    uint32_t endAddr = (m_RangeEndAddress - length) + 1;

    CScanResult result(m_AddressType, resultDisplayFormat);
    result.SetStrLength(length);

    for (uint32_t addr = startAddr; addr <= endAddr; addr++)
    {
        for (int i = 0; i < length; i++)
        {
            uint32_t leAddr = (addr + i) ^ 3;
            if ((uint8_t)m_Value._string[i] != m_Memory[leAddr])
            {
                goto next_addr;
            }
        }

        result.m_Address = addr | m_VAddrBits;
        result.Set((const char*)NULL);
        m_Results.push_back(result);
    next_addr:;
    }
}

// scan for text (case-insensitive)
void CMemoryScanner::FirstScanLoopIString(DisplayFormat resultDisplayFormat)
{
    int length = m_StringValueLength;

    uint32_t startAddr = m_RangeStartAddress;
    uint32_t endAddr = m_RangeEndAddress - length;

    CScanResult result(m_AddressType, resultDisplayFormat);
    result.SetStrLength(length);

    for (uint32_t addr = startAddr; addr <= endAddr; addr++)
    {
        for (int i = 0; i < length; i++)
        {
            uint32_t leAddr = (addr + i) ^ 3;
            if (toupper((uint8_t)m_Value._string[i]) != toupper(m_Memory[leAddr]))
            {
                goto next_addr;
            }
        }

        result.m_Address = addr | m_VAddrBits;
        result.Set((const char*)NULL);
        m_Results.push_back(result);
    next_addr:;
    }
}

// scan for text of unknown single-byte encoding
void CMemoryScanner::FirstScanLoopUnkString(void)
{
    const uint8_t* str = (const uint8_t*)m_Value._string;
    int length = m_StringValueLength;
    
    uint32_t startAddr = m_RangeStartAddress;
    uint32_t endAddr = m_RangeEndAddress - length;

    CScanResult result(m_AddressType, DisplayHex);
    result.SetStrLength(length);

    for (uint32_t addr = startAddr; addr <= endAddr; addr++)
    {
        uint32_t leAddr = addr ^ 3;

        char numberDiff = 0, lowercaseDiff = 0, uppercaseDiff = 0;
        bool haveNumberDiff = false, haveLowercaseDiff = false, haveUppercaseDiff = false;

        for (int i = 0; i < length; i++)
        {
            leAddr = (addr + i) ^ 3;

            if (!isalnum(str[i]))
            {
                continue;
            }

            if (str[i] >= 'a' && str[i] <= 'z')
            {
                if (!haveLowercaseDiff)
                {
                    lowercaseDiff = str[i] - m_Memory[leAddr];
                    haveLowercaseDiff = true;
                }
                else if (m_Memory[leAddr] + lowercaseDiff != str[i])
                {
                    goto next_addr;
                }
            }
            else if (str[i] >= 'A' && str[i] <= 'Z')
            {
                if (!haveUppercaseDiff)
                {
                    uppercaseDiff = str[i] - m_Memory[leAddr];
                    haveUppercaseDiff = true;
                }
                else if (m_Memory[leAddr] + uppercaseDiff != str[i])
                {
                    goto next_addr;
                }
            }
            else if (str[i] >= '0' && str[i] <= '9')
            {
                if (!haveNumberDiff)
                {
                    numberDiff = str[i] - m_Memory[leAddr];
                    haveNumberDiff = true;
                }
                else if (m_Memory[leAddr] + numberDiff != str[i])
                {
                    goto next_addr;
                }
            }
        }

        result.m_Address = addr | m_VAddrBits;
        result.Set((const char*)NULL);
        m_Results.push_back(result);

    next_addr:;
    }
}

#define _FirstScanLoopPrimitive(T, Compare, resDisplayFormat) FirstScanLoopPrimitive<T>(Compare<T>, resDisplayFormat)
#define _FirstScanLoopPrimitive64(T, Compare, resDisplayFormat) FirstScanLoopPrimitive64<T>(Compare<T>, resDisplayFormat)

#define FIRST_SCAN_PRIMITIVES(CompareFunc) \
    switch(m_ValueType)                    \
    {                                      \
    case ValueType_uint8:  _FirstScanLoopPrimitive(uint8_t,  CompareFunc, resDisplayFormat); break; \
    case ValueType_int8:   _FirstScanLoopPrimitive(int8_t,   CompareFunc, resDisplayFormat); break; \
    case ValueType_uint16: _FirstScanLoopPrimitive(uint16_t, CompareFunc, resDisplayFormat); break; \
    case ValueType_int16:  _FirstScanLoopPrimitive(int16_t,  CompareFunc, resDisplayFormat); break; \
    case ValueType_uint32: _FirstScanLoopPrimitive(uint32_t, CompareFunc, resDisplayFormat); break; \
    case ValueType_int32:  _FirstScanLoopPrimitive(int32_t,  CompareFunc, resDisplayFormat); break; \
    case ValueType_uint64: _FirstScanLoopPrimitive64(uint64_t, CompareFunc, resDisplayFormat); break; \
    case ValueType_int64:  _FirstScanLoopPrimitive64(int64_t,  CompareFunc, resDisplayFormat); break; \
    case ValueType_float:  _FirstScanLoopPrimitive(float,    CompareFunc, resDisplayFormat); break; \
    case ValueType_double: _FirstScanLoopPrimitive64(double,   CompareFunc, resDisplayFormat); break; \
    }

bool CMemoryScanner::FirstScan(DisplayFormat resDisplayFormat)
{
    if (!g_MMU)
    {
        return false;
    }

    if (m_bDataTypePrimitive)
    {
        switch (m_SearchType)
        {
        case SearchType_UnknownValue:
            FIRST_SCAN_PRIMITIVES(NoCompare);
            break;
        case SearchType_ExactValue:
            FIRST_SCAN_PRIMITIVES(CompareEqual);
            break;
        case SearchType_JalTo:
            m_Value._uint32 = 0x0C000000 | ((m_Value._uint32 & 0x3FFFFFF) >> 2);
            FIRST_SCAN_PRIMITIVES(CompareEqual);
            break;
        case SearchType_LessThanValue:
            FIRST_SCAN_PRIMITIVES(CompareLessThan);
            break;
        case SearchType_GreaterThanValue:
            FIRST_SCAN_PRIMITIVES(CompareGreaterThan);
            break;
        case SearchType_LessThanOrEqualToValue:
            FIRST_SCAN_PRIMITIVES(CompareLessThanOrEqual);
            break;
        case SearchType_GreaterThanOrEqualToValue:
            FIRST_SCAN_PRIMITIVES(CompareGreaterThanOrEqual);
            break;
        }
    }
    else
    {
        switch (m_ValueType)
        {
        case ValueType_string:
            FirstScanLoopString(resDisplayFormat);
            break;
        case ValueType_istring:
            FirstScanLoopIString(resDisplayFormat);
            break;
        case ValueType_unkstring:
            FirstScanLoopUnkString();
            break;
        }
    }

    m_DidFirstScan = true;
    return true;
}

#define _NextScanLoopPrimitive(T, Compare) NextScanLoopPrimitive<T>(Compare<T>)
#define _NextScanLoopPrimitiveResults(T, Compare) NextScanLoopPrimitiveResults<T>(Compare<T>)
#define _NextScanLoopPrimitive64(T, Compare) NextScanLoopPrimitive64<T>(Compare<T>)
#define _NextScanLoopPrimitiveResults64(T, Compare) NextScanLoopPrimitiveResults64<T>(Compare<T>)

// compare result's current value in memory against m_Value
#define NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareFunc) \
    switch(m_ValueType)                                 \
    {                                                   \
    case ValueType_uint8:  _NextScanLoopPrimitive(uint8_t,  CompareFunc); break; \
    case ValueType_int8:   _NextScanLoopPrimitive(int8_t,   CompareFunc); break; \
    case ValueType_uint16: _NextScanLoopPrimitive(uint16_t, CompareFunc); break; \
    case ValueType_int16:  _NextScanLoopPrimitive(int16_t,  CompareFunc); break; \
    case ValueType_uint32: _NextScanLoopPrimitive(uint32_t, CompareFunc); break; \
    case ValueType_int32:  _NextScanLoopPrimitive(int32_t,  CompareFunc); break; \
    case ValueType_uint64: _NextScanLoopPrimitive64(uint64_t, CompareFunc); break; \
    case ValueType_int64:  _NextScanLoopPrimitive64(int64_t,  CompareFunc); break; \
    case ValueType_float:  _NextScanLoopPrimitive(float,    CompareFunc); break; \
    case ValueType_double: _NextScanLoopPrimitive64(double,   CompareFunc); break; \
    }

// compare result's current value in memory against result's old value
#define NEXT_SCAN_PRIMITIVES_AGAINST_RESULTS(CompareFunc) \
    switch(m_ValueType)                                   \
    {                                                     \
    case ValueType_uint8:  _NextScanLoopPrimitiveResults(uint8_t,  CompareFunc); break; \
    case ValueType_int8:   _NextScanLoopPrimitiveResults(int8_t,   CompareFunc); break; \
    case ValueType_uint16: _NextScanLoopPrimitiveResults(uint16_t, CompareFunc); break; \
    case ValueType_int16:  _NextScanLoopPrimitiveResults(int16_t,  CompareFunc); break; \
    case ValueType_uint32: _NextScanLoopPrimitiveResults(uint32_t, CompareFunc); break; \
    case ValueType_int32:  _NextScanLoopPrimitiveResults(int32_t,  CompareFunc); break; \
    case ValueType_uint64: _NextScanLoopPrimitiveResults64(uint64_t, CompareFunc); break; \
    case ValueType_int64:  _NextScanLoopPrimitiveResults64(int64_t,  CompareFunc); break; \
    case ValueType_float:  _NextScanLoopPrimitiveResults(float,    CompareFunc); break; \
    case ValueType_double: _NextScanLoopPrimitiveResults64(double,   CompareFunc); break; \
    }

bool CMemoryScanner::NextScan()
{
    if (!g_MMU || !m_DidFirstScan || !m_bDataTypePrimitive)
    {
        // NextScan does not support complex data
        return false;
    }
    
    switch(m_SearchType)
    {
    case SearchType_ExactValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareEqual);
        break;
    case SearchType_LessThanValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareLessThan);
        break;
    case SearchType_GreaterThanValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareGreaterThan);
        break;
    case SearchType_LessThanOrEqualToValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareLessThanOrEqual);
        break;
    case SearchType_GreaterThanOrEqualToValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_VALUE(CompareGreaterThanOrEqual);
        break;

    case SearchType_ChangedValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_RESULTS(CompareNotEqual);
        break;
    case SearchType_UnchangedValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_RESULTS(CompareEqual);
        break;
    case SearchType_IncreasedValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_RESULTS(CompareGreaterThan);
        break;
    case SearchType_DecreasedValue:
        NEXT_SCAN_PRIMITIVES_AGAINST_RESULTS(CompareLessThan);
        break;
    }
    
    return true;
}


int CMemoryScanner::HexDigitVal(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A') + 0x0A;
    if (c >= 'a' && c <= 'f') return (c - 'a') + 0x0A;
    return 0;
}

int CMemoryScanner::ParseHexString(char *dst, const char* src)
{
    bool bHiNibble = true;
    uint8_t curByte = 0;
    int size = 0;

    for (int i = 0; src[i] != '\0'; i++)
    {
        if (!isxdigit(src[i]))
        {
            if (!bHiNibble)
            {
                return 0;
            }

            if (isspace(src[i]))
            {
                continue;
            }

            return 0;
        }

        if (bHiNibble)
        {
            curByte = (HexDigitVal(src[i]) << 4) & 0xF0;
            bHiNibble = false;
        }
        else
        {
            curByte |= HexDigitVal(src[i]);
            if (dst != NULL)
            {
                dst[size] = curByte;
            }
            size++;
            bHiNibble = true;
        }
    }

    if (!bHiNibble)
    {
        return 0;
    }

    return size;
}
