#pragma once

class RomInformation
{
    bool const    m_DeleteRomInfo;
    bool const    m_DeleteDiskInfo;
    stdstr  const m_FileName;
    CN64Rom *     m_pRomInfo;
    CN64Disk *    m_pDiskInfo;

    friend INT_PTR CALLBACK RomInfoProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    RomInformation(const char* RomFile);
    RomInformation(CN64Rom* RomInfo);
    RomInformation(CN64Disk* DiskInfo);
    ~RomInformation();

    void DisplayInformation(HWND hParent) const;
};
