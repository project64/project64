#pragma once

class Rumblepak
{
public:
    static void ReadFrom(uint32_t address, uint8_t * data);
    static void WriteTo(int32_t Control, uint32_t address, uint8_t * data);
};
