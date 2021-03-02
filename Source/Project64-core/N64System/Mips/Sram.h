#pragma once

class CSram
{
public:
    CSram(bool ReadOnly);
    ~CSram();

    void DmaFromSram(uint8_t * dest, int32_t StartOffset, uint32_t len);
    void DmaToSram(uint8_t * Source, int32_t StartOffset, uint32_t len);

private:
    CSram(void);                        // Disable default constructor
    CSram(const CSram&);              // Disable copy constructor
    CSram& operator=(const CSram&);   // Disable assignment

    bool LoadSram();

    bool m_ReadOnly;
    CFile m_File;
};
