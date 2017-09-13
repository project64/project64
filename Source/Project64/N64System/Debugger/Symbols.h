#pragma once
#include "stdafx.h"

class CSymbolEntry;

class CSymbols
{
public:
    typedef enum {
        ERR_SUCCESS,
        ERR_INVALID_TYPE,
        ERR_INVALID_ADDR,
        ERR_INVALID_NAME,
        ERR_MISSING_FIELDS,
    } ParseError;

    typedef enum {
        TYPE_CODE,
        TYPE_DATA,
        TYPE_U8,
        TYPE_U16,
        TYPE_U32,
        TYPE_U64,
        TYPE_S8,
        TYPE_S16,
        TYPE_S32,
        TYPE_S64,
        TYPE_FLOAT,
        TYPE_DOUBLE
    } DataType;

    static constexpr char* SymbolTypes[] = {
        "code", // 0
        "data", // 1
        "u8",
        "u16",
        "u32",
        "u64",
        "s8",
        "s16",
        "s32",
        "s64",
        "float",
        "double",
        NULL
    };

    static constexpr int TypeSizes[] = {
        1, 1, // code data
        1, 2, 4, 8, // u8 u16 u32 u64
        1, 2, 4, 8, // s8 s16 s32 s64
        4, 8 // float double
    };

private:
    static bool m_bInitialized;
    static vector<CSymbolEntry*> m_Symbols;
    static int m_NextSymbolId;

    static CFile m_SymFileHandle;
    static char* m_SymFileBuffer;
    static size_t m_SymFileSize;

    static CRITICAL_SECTION m_CriticalSection;

    static char* m_ParserToken;
    static size_t m_ParserTokenLength;
    static char m_ParserDelimeter;
    static char* m_SymFileParseBuffer;
    static bool m_bHaveFirstToken;

    static void ParserInit();
    static void ParserDone();
    static void ParserFetchToken(const char* delim);

    static bool SortFunction(CSymbolEntry* a, CSymbolEntry* b);

public:
    static CPath GetSymFilePath();
    static void Load();
    static void Save();
    static void ParseErrorAlert(char* message, int lineNumber);

    static void Add(int type, uint32_t address, char* name, char* description = NULL);
    static void RemoveEntryById(int id);

    static void Reset();

    static const char* GetTypeName(int typeNumber);
    static int GetTypeNumber(char* typeName);
    static void GetValueString(char* str, CSymbolEntry* lpSymbol);

    static int GetCount();

    static CSymbolEntry* GetEntryById(int id);
    static CSymbolEntry* GetEntryByIndex(int id);
    static CSymbolEntry* GetEntryByAddress(uint32_t address);

    static const char* GetNameByAddress(uint32_t address);

    static void InitializeCriticalSection();
    static void DeleteCriticalSection();
    static void EnterCriticalSection();
    static void LeaveCriticalSection();
};

class CSymbolEntry {
public:
    int m_Id;
    int m_Type;
    uint32_t m_Address;
    char* m_Name;
    char* m_Description;

    CSymbolEntry(int id, int type, uint32_t address, char* name, char* description) :
        m_Name(NULL),
        m_Description(NULL),
        m_Id(id),
        m_Type(type),
        m_Address(address)
    {
        if (name != NULL)
        {
            size_t nameLen = strlen(name);
            m_Name = (char*)malloc(nameLen + 1);
            strcpy(m_Name, name);
        }

        if (description != NULL)
        {
            size_t descLen = strlen(description);
            m_Description = (char*)malloc(descLen + 1);
            strcpy(m_Description, description);
        }
    }

    ~CSymbolEntry()
    {
        if (m_Name != NULL)
        {
            free(m_Name);
        }
        if (m_Description != NULL)
        {
            free(m_Description);
        }
    }

    const char* TypeName()
    {
        return CSymbols::SymbolTypes[m_Type];
    }

    int TypeSize()
    {
        return CSymbols::TypeSizes[m_Type];
    }
};