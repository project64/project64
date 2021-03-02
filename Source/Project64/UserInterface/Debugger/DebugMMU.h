#pragma once
#include <stdafx.h>

class CDebugMMU
{
public:
    size_t ReadPhysical(uint32_t paddr, size_t length, uint8_t* buffer);
    size_t ReadVirtual(uint32_t vaddr, size_t length, uint8_t* buffer);
    size_t WritePhysical(uint32_t paddr, size_t length, uint8_t* buffer);
    size_t WriteVirtual(uint32_t vaddr, size_t length, uint8_t* buffer);

    template<typename T>
    bool DebugLoad_PAddr(uint32_t paddr, T& value)
    {
        union {
            T       word;
            uint8_t bytes[sizeof(T)];
        } buffer;

        if (ReadPhysical(paddr, sizeof(buffer), buffer.bytes) == sizeof(buffer))
        {
            value = ByteSwap<T>(buffer.word);
            return true;
        }

        return false;
    }

    template<typename T>
    bool DebugLoad_VAddr(uint32_t vaddr, T& value)
    {
        union {
            T       word;
            uint8_t bytes[sizeof(T)];
        } buffer;

        if (ReadVirtual(vaddr, sizeof(buffer), buffer.bytes) == sizeof(buffer))
        {
            value = ByteSwap<T>(buffer.word);
            return true;
        }

        return false;
    }

    template<typename T>
    bool DebugStore_PAddr(uint32_t paddr, T value)
    {
        union {
            T       word;
            uint8_t bytes[sizeof(T)];
        } buffer = { ByteSwap<T>(value) };

        return (WritePhysical(paddr, sizeof(buffer), buffer.bytes) == sizeof(buffer));
    }

    template<typename T>
    bool DebugStore_VAddr(uint32_t vaddr, T value)
    {
        union {
            T       word;
            uint8_t bytes[sizeof(T)];
        } buffer = { ByteSwap<T>(value) };

        return (WriteVirtual(vaddr, sizeof(buffer), buffer.bytes) == sizeof(buffer));
    }

private:
    uint8_t*  GetPhysicalPtr(uint32_t paddr, WORD* flags = NULL);
    bool      GetPhysicalByte(uint32_t paddr, uint8_t* value);
    bool      SetPhysicalByte(uint32_t paddr, uint8_t value);

    template<typename T>
    T ByteSwap(T value)
    {
        union
        {
            T        value;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
        } bytes;

        bytes.value = value;

        switch (sizeof(T))
        {
        case sizeof(uint8_t) :
            break;
        case sizeof(uint16_t) :
            bytes.u16 = _byteswap_ushort(bytes.u16);
            break;
        case sizeof(uint32_t) :
            bytes.u32 = _byteswap_ulong(bytes.u32);
            break;
        case sizeof(uint64_t) :
            bytes.u64 = _byteswap_uint64(bytes.u64);
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        return bytes.value;
    }
};
