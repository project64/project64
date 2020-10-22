#pragma once
#include <string>
#include <vector>

class CProjectSupport
{
    typedef struct
    {
        char Code[300];
        char Email[300];
        char Name[300];
        char MachineID[300];
        uint32_t RunCount;
        bool Validated;
    } SupportInfo;

public:
    CProjectSupport();

    bool RequestCode(const char * Email);
    bool ValidateCode(const char * Code);
    void IncrementRunCount();

    inline uint32_t RunCount() const { return m_SupportInfo.RunCount; }
    inline const char * MachineID(void) const { return m_SupportInfo.MachineID; }
    inline bool Validated(void) const { return m_SupportInfo.Validated; }

private:
    CProjectSupport(const CProjectSupport&);
    CProjectSupport& operator=(const CProjectSupport&);

    std::string GenerateMachineID();
    bool PerformRequest(const wchar_t * Url, const std::string & PostData, DWORD & StatusCode, std::vector<std::string> & Headers);

    void SaveSupportInfo(void);
    void LoadSupportInfo(void);

    SupportInfo m_SupportInfo;
};