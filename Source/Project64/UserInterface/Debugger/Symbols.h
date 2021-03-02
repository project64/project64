#pragma once
#include "stdafx.h"

class CSymbol;

typedef enum {
    SYM_INVALID = -1,
    SYM_CODE,
    SYM_DATA,
    SYM_U8,
    SYM_U16,
    SYM_U32,
    SYM_U64,
    SYM_S8,
    SYM_S16,
    SYM_S32,
    SYM_S64,
    SYM_FLOAT,
    SYM_DOUBLE,
    SYM_VECTOR2,
    SYM_VECTOR3,
    SYM_VECTOR4,
    NUM_SYM_TYPES
} symbol_type_id_t;

typedef struct
{
    symbol_type_id_t    id;
    const char*       name;
    int               size;
} symbol_type_info_t;

typedef enum {
    ERR_SUCCESS,
    ERR_INVALID_TYPE,
    ERR_INVALID_ADDR,
    ERR_INVALID_NAME,
    ERR_MISSING_FIELDS,
} symbol_parse_error_t;

class CSymbolTable
{
public:
    CSymbolTable(CDebuggerUI* debugger);
    ~CSymbolTable();

private:
    CSymbolTable();
    CDebuggerUI* m_Debugger;
    CriticalSection m_CS;
    std::vector<CSymbol> m_Symbols;
    
    int    m_NextSymbolId;

    CFile  m_SymFileHandle;
    char*  m_SymFileBuffer;
    size_t m_SymFileSize;
    char*  m_ParserToken;
    size_t m_ParserTokenLength;
    char*  m_TokPos;
    char   m_ParserDelimeter;
    char*  m_SymFileParseBuffer;
    bool   m_bHaveFirstToken;

    void ParserFetchToken(const char* delim);

public:
    static symbol_type_info_t m_SymbolTypes[];
    static const char* GetTypeName(int typeId);
    static int GetTypeSize(int typeId);
    static symbol_type_id_t GetTypeId(char* typeName);
    static bool CmpSymbolAddresses(CSymbol& a, CSymbol& b);

    void GetValueString(char* dst, CSymbol* symbol);

    CPath GetSymFilePath();
    void Load();
    void Save();
    void ParseErrorAlert(char* message, int lineNumber);

    void AddSymbol(int type, uint32_t address, const char* name, const char* description = NULL);
    void Reset();
    int  GetCount();
    bool GetSymbolById(int id, CSymbol* symbol);
    bool GetSymbolByIndex(size_t index, CSymbol* symbol);
    bool GetSymbolByAddress(uint32_t address, CSymbol* symbol);
    bool GetSymbolByOverlappedAddress(uint32_t address, CSymbol* symbol);
    bool RemoveSymbolById(int id);
};

class CSymbol {
public:
    int      m_Id;
    int      m_Type;
    uint32_t m_Address;
    char*    m_Name;
    char*    m_Description;

    CSymbol() :
        m_Id(0),
        m_Type(SYM_INVALID),
        m_Address(0),
        m_Name(NULL),
        m_Description(NULL)
    {
    }

    CSymbol(int id, int type, uint32_t address, const char* name, const char* description) :
        m_Id(id),
        m_Type(type),
        m_Address(address),
        m_Name(NULL),
        m_Description(NULL)
    {
        if (name != NULL)
        {
            m_Name = _strdup(name);
        }

        if (description != NULL)
        {
            m_Description = _strdup(description);
        }
    }

    CSymbol(const CSymbol& symbol):
        m_Id(symbol.m_Id),
        m_Type(symbol.m_Type),
        m_Address(symbol.m_Address),
        m_Name(NULL),
        m_Description(NULL)
    {
        m_Name = symbol.m_Name ? _strdup(symbol.m_Name) : NULL;
        m_Description = symbol.m_Description ? _strdup(symbol.m_Description) : NULL;
    }

    CSymbol& operator= (const CSymbol& symbol)
    {
        if (m_Name != NULL)
        {
            free(m_Name);
        }

        if (m_Description != NULL)
        {
            free(m_Description);
        }

        m_Id = symbol.m_Id;
        m_Type = symbol.m_Type;
        m_Address = symbol.m_Address;
        m_Name = symbol.m_Name ? _strdup(symbol.m_Name) : NULL;
        m_Description = symbol.m_Description ? _strdup(symbol.m_Description) : NULL;
        return *this;
    }

    ~CSymbol()
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
        return CSymbolTable::GetTypeName(m_Type);
    }

    int TypeSize()
    {
        return CSymbolTable::GetTypeSize(m_Type);
    }
};