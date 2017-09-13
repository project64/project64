#pragma once

#include <stdint.h>

typedef void(*SYNTAX)(uint32_t* opcode);

typedef struct {
    const char* name;
    uint32_t val;
    uint32_t(*base)(uint32_t val); // value shift
    SYNTAX* syntax; // arguments
} INSTRUCTION;

typedef struct {
    const char* name;
    uint32_t val;
} REGISTER;

enum ParseError
{
    ERR_NONE,
    ERR_EXPECTED_REG,
    ERR_INVALID_REG,
    ERR_EXPECTED_VAL
};

class CAssembler {
public:
    static bool AssembleLine(char* line, uint32_t* opcode, uint32_t address = 0x00000000);
};
