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
#pragma once

#include <stdafx.h>

enum ValueType
{
    ValueType_invalid = -1,
    ValueType_uint8,
    ValueType_int8,
    ValueType_uint16,
    ValueType_int16,
    ValueType_uint32,
    ValueType_int32,
    ValueType_uint64,
    ValueType_int64,
    ValueType_float,
    ValueType_double,
    // non-primitives:
    ValueType_string,
    ValueType_istring,
    ValueType_unkstring
};

enum AddressType
{
    AddressType_Physical,
    AddressType_Virtual
};

enum SearchType
{
    SearchType_ExactValue,
    SearchType_LessThanValue,
    SearchType_GreaterThanValue,
    SearchType_LessThanOrEqualToValue,
    SearchType_GreaterThanOrEqualToValue,
    // first scan only:
    SearchType_UnknownValue,
    SearchType_JalTo,
    // next scan only:
    SearchType_ChangedValue,
    SearchType_UnchangedValue,
    SearchType_IncreasedValue,
    SearchType_DecreasedValue
};

enum DisplayFormat
{
    DisplayDefault,
    DisplayHex
};

typedef union {
    uint8_t   _uint8;
    int8_t    _sint8;
    uint16_t  _uint16;
    int16_t   _sint16;
    uint32_t  _uint32;
    int32_t   _sint32;
    uint64_t  _uint64;
    int64_t   _sint64;
    float     _float;
    double    _double;
    const wchar_t* _string;
} MixedValue;

class CMixed
{
protected:
    ValueType m_Type;
    int m_StrLength;

public:
    MixedValue m_Value;

    CMixed()
    {
        m_StrLength = 0;
        m_Value._uint64 = 0;
        m_Type = ValueType_uint64;
    }

    inline void SetType(ValueType t) { m_Type = t; }
    inline ValueType GetType(void) { return m_Type; }

    inline void SetStrLength(int length) { m_StrLength = length; }
    inline int GetStrLength(void) { return m_StrLength; }

    inline void Set(uint8_t v)     { SetType(ValueType_uint8);  m_Value._uint8 = v; }
    inline void Set(int8_t v)      { SetType(ValueType_int8);   m_Value._sint8 = v; }
    inline void Set(uint16_t v)    { SetType(ValueType_uint16); m_Value._uint16 = v; }
    inline void Set(int16_t v)     { SetType(ValueType_int16);  m_Value._sint16 = v; }
    inline void Set(uint32_t v)    { SetType(ValueType_uint32); m_Value._uint32 = v; }
    inline void Set(int32_t v)     { SetType(ValueType_int32);  m_Value._sint32 = v; }
    inline void Set(uint64_t v)    { SetType(ValueType_uint64); m_Value._uint64 = v; }
    inline void Set(int64_t v)     { SetType(ValueType_int64);  m_Value._sint64 = v; }
    inline void Set(float v)       { SetType(ValueType_float);  m_Value._float = v; }
    inline void Set(double v)      { SetType(ValueType_double); m_Value._double = v; }
    inline void Set(const wchar_t* v) { SetType(ValueType_string); m_Value._string = v; }

    inline void Get(uint8_t* v)     { *v = m_Value._uint8; }
    inline void Get(int8_t* v)      { *v = m_Value._sint8; }
    inline void Get(uint16_t* v)    { *v = m_Value._uint16; }
    inline void Get(int16_t* v)     { *v = m_Value._sint16; }
    inline void Get(uint32_t* v)    { *v = m_Value._uint32; }
    inline void Get(int32_t* v)     { *v = m_Value._sint32; }
    inline void Get(uint64_t* v)    { *v = m_Value._uint64; }
    inline void Get(int64_t* v)     { *v = m_Value._sint64; }
    inline void Get(float* v)       { *v = m_Value._float; }
    inline void Get(double* v)      { *v = m_Value._double; }
    inline void Get(const wchar_t** v) { *v = m_Value._string; }

    const char* GetTypeName(void);
    int GetTypeSize(void);
    bool IsStringType(void);
    int ToString(char* buffer, bool bHex, size_t size);

    static ValueType GetTypeFromString(const char* name, int* typeArraySize);

private:
    typedef struct
    {
        const char* name;
        ValueType type;
    } TypeNameEntry;

    static TypeNameEntry TypeNames[];
};

class CScanResult : public CMixed
{
public:
    CScanResult(AddressType addressType, DisplayFormat displayFormat);
    ~CScanResult(void);

    uint32_t m_Address;
    AddressType m_AddressType;
    DisplayFormat m_DisplayFormat;
    bool m_bSelected;
    char* m_Description;

public:
    int GetValueString(char* buffer, size_t size);
    int GetMemoryValueString(char* buffer, size_t size, bool bIgnoreHex = false);
    int GetAddressString(char *buffer);
    uint32_t GetVirtualAddress(void);
    bool SetMemoryValueFromString(const char* str);
    //bool IsSelected(void);
    //void SetSelected(bool bSelected);
    void SetDescription(const char* str);
    const char* GetDescription(void);
    void DeleteDescription(void);
    bool GetMemoryValue(CMixed* v);

    bool SetAddressSafe(uint32_t address);
    bool SetStrLengthSafe(int length);
};

class CMemoryScanner
{
public:
    CMemoryScanner(void);

    bool SetAddressRange(uint32_t startAddress, uint32_t endAddress);
    bool SetValueType(ValueType type);
    bool SetSearchType(SearchType searchType);
    void SetAddressType(AddressType addressType);

    static int ParseHexString(char* dst, const char* src);

    static bool AddrCheck(uint32_t addr, uint32_t rangeStart, uint32_t rangeEnd);
    static bool RangeCheck(uint32_t addrStart, uint32_t addrEnd, uint32_t rangeStart, uint32_t rangeEnd);
    static bool PAddrValid(uint32_t physAddr);
    static bool PAddrRangeValid(uint32_t physAddrStart, uint32_t physAddrEnd);

    template <class T>
    void SetValue(T value)
    {
        *(T*)&m_Value = value;
    }

    void SetStringValueLength(int length);

    void Reset(void);
    bool FirstScan(DisplayFormat resDisplayFormat = DisplayDefault);
    bool NextScan(void);

    bool DidFirstScan(void);

    size_t GetNumResults(void);
    CScanResult* GetResult(size_t index);
    void RemoveResult(size_t index);

private:
    static int HexDigitVal(char c);

    uint8_t* m_Memory;
    bool m_DidFirstScan;
    
    uint32_t m_RangeStartAddress;
    uint32_t m_RangeEndAddress;

    ValueType m_ValueType;
    bool m_bDataTypePrimitive;
    SearchType m_SearchType;
    AddressType m_AddressType;
    uint32_t m_VAddrBits;
    
    std::vector<CScanResult> m_Results;
    std::vector<CScanResult> m_NewResults;
    
    MixedValue m_Value;
    int m_StringValueLength;

    friend class CScanResult;
    static uint8_t* GetMemoryPool(uint32_t physAddr);

    template <class T> static bool CompareLessThan(T a, T b) { return a < b; }
    template <class T> static bool CompareLessThanOrEqual(T a, T b) { return a <= b; }
    template <class T> static bool CompareGreaterThan(T a, T b) { return a > b; }
    template <class T> static bool CompareGreaterThanOrEqual(T a, T b) { return a >= b; }
    template <class T> static bool CompareEqual(T a, T b) { return a == b; }
    template <class T> static bool CompareNotEqual(T a, T b) { return a != b; }
    template <class T> static bool NoCompare(T /*a*/, T /*b*/) { return true; }

    void FirstScanLoopString(DisplayFormat resultDisplayFormat);
    void FirstScanLoopIString(DisplayFormat resultDisplayFormat);
    void FirstScanLoopUnkString(void);
    
    template <class T>
    void FirstScanLoopPrimitive(bool(*CompareFunc)(T, T), DisplayFormat resultDisplayFormat)
    {
        T searchValue = *(T*)&m_Value;
        uint32_t startAddr = ((m_RangeStartAddress - 1) | (sizeof(T) - 1)) + 1;
        uint32_t endAddr = m_RangeEndAddress;

        CScanResult result(m_AddressType, resultDisplayFormat);

        for (uint32_t addr = startAddr; addr <= endAddr; addr += sizeof(T))
        {
            uint32_t leAddr = (addr ^ (4 - sizeof(T)));
            T memValue = *(T*)&m_Memory[leAddr];

            if (CompareFunc(memValue, searchValue))
            {
                result.m_Address = addr | m_VAddrBits;
                result.Set(memValue);
                m_Results.push_back(result);
            }
        }
    }
    
    // for int64 and double
    template <class T>
    void FirstScanLoopPrimitive64(bool(*CompareFunc)(T, T), DisplayFormat resultDisplayFormat)
    {
        T searchValue = *(T*)&m_Value;
        uint32_t startAddr = ((m_RangeStartAddress - 1) | (sizeof(T) - 1)) + 1;
        uint32_t endAddr = m_RangeEndAddress;

        CScanResult result(m_AddressType, resultDisplayFormat);

        for (uint32_t addr = startAddr; addr <= endAddr; addr += sizeof(T))
        {
            T memValue;

            *((uint32_t*)(&memValue) + 1) = *(uint32_t*) &m_Memory[addr];
            *((uint32_t*)(&memValue) + 0) = *(uint32_t*) &m_Memory[addr + 4];

            if (CompareFunc(memValue, searchValue))
            {
                result.m_Address = addr | m_VAddrBits;
                result.Set(memValue);
                m_Results.push_back(result);
            }
        }
    }

    // compare result's current value in memory against m_Value
    template <class T>
    void NextScanLoopPrimitive(bool(*CompareFunc)(T, T))
    {
        T searchValue = *(T*)&m_Value;

        for (size_t index = 0; index < m_Results.size(); index++)
        {
            CScanResult* presult = &m_Results[index];

            uint32_t addr = presult->m_Address & 0x1FFFFFFF;
            uint32_t leAddr = (addr ^ (4 - sizeof(T)));
            T memValue = *(T*) &m_Memory[leAddr];

            if (CompareFunc(memValue, searchValue))
            {
                presult->Set(memValue);
                m_NewResults.push_back(*presult);
            }
        }

        m_Results.clear();
        m_Results.swap(m_NewResults);
    }

    // compare result's current value in memory against m_Value (for 64 bit types)
    template <class T>
    void NextScanLoopPrimitive64(bool(*CompareFunc)(T, T))
    {
        T searchValue = *(T*)&m_Value;

        for (size_t index = 0; index < m_Results.size(); index++)
        {
            CScanResult* presult = &m_Results[index];

            uint32_t addr = presult->m_Address & 0x1FFFFFFF;
            T memValue;

            *((uint32_t*)(&memValue) + 1) = *(uint32_t*) &m_Memory[addr];
            *((uint32_t*)(&memValue) + 0) = *(uint32_t*) &m_Memory[addr + 4];

            if (CompareFunc(memValue, searchValue))
            {
                presult->Set(memValue);
                m_NewResults.push_back(*presult);
            }
        }

        m_Results.clear();
        m_Results.swap(m_NewResults);
    }

    // compare result's current value in memory against result's old value
    template <class T>
    void NextScanLoopPrimitiveResults(bool(*CompareFunc)(T, T))
    {
        for (size_t index = 0; index < m_Results.size(); index++)
        {
            CScanResult* presult = &m_Results[index];

            uint32_t addr = presult->m_Address & 0x1FFFFFFF;
            uint32_t leAddr = (addr ^ (4 - sizeof(T)));

            T memValue, oldValue;
            memValue = *(T*)&m_Memory[leAddr];
            presult->Get(&oldValue);

            if (CompareFunc(memValue, oldValue))
            {
                presult->Set(memValue);
                m_NewResults.push_back(*presult);
            }
        }

        m_Results.clear();
        m_Results.swap(m_NewResults);
    }

    // compare result's current value in memory against result's old value (for 64 bit types)
    template <class T>
    void NextScanLoopPrimitiveResults64(bool(*CompareFunc)(T, T))
    {
        for (size_t index = 0; index < m_Results.size(); index++)
        {
            CScanResult* presult = &m_Results[index];

            uint32_t addr = presult->m_Address & 0x1FFFFFFF;

            T memValue, oldValue;

            *((uint32_t*)(&memValue) + 1) = *(uint32_t*)&m_Memory[addr];
            *((uint32_t*)(&memValue) + 0) = *(uint32_t*)&m_Memory[addr + 4];

            presult->Get(&oldValue);

            if (CompareFunc(memValue, oldValue))
            {
                presult->Set(memValue);
                m_NewResults.push_back(*presult);
            }
        }

        m_Results.clear();
        m_Results.swap(m_NewResults);
    }
};
