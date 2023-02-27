#pragma once

class RomInformation
{
    bool const m_DeleteRomInfo;
    bool const m_DeleteDiskInfo;
    stdstr const m_FileName;
    CN64Rom * m_pRomInfo;
    CN64Disk * m_pDiskInfo;

    friend DWORD CALLBACK RomInfoProc(HWND, DWORD, WPARAM, LPARAM);

public:
    RomInformation(const char * RomFile);
    RomInformation(CN64Rom * RomInfo);
    RomInformation(CN64Disk * DiskInfo);
    ~RomInformation();

    void DisplayInformation(HWND hParent) const;
};
