#include "stdafx.h"
#include "SettingsType-Application.h"
#include "SettingsType-ApplicationPath.h"
#include <Common/path.h>

CSettingTypeApplicationPath::CSettingTypeApplicationPath(const char * Section, const char * Name, SettingID DefaultSetting) :
CSettingTypeApplication(Section, Name, DefaultSetting)
{
}

CSettingTypeApplicationPath::~CSettingTypeApplicationPath()
{
}

bool CSettingTypeApplicationPath::IsSettingSet(void) const
{
    return CSettingTypeApplication::IsSettingSet();
}

bool CSettingTypeApplicationPath::Load(int Index, stdstr & Value) const
{
    bool bRes = CSettingTypeApplication::Load(Index, Value);
    if (bRes)
    {
        if (Value.substr(0, 2) == ".\\" || Value.substr(0, 2) == "./" ||
            Value.substr(0, 3) == "..\\" || Value.substr(0, 3) == "../")
        {
            CPath FullFilePath(g_Settings->LoadStringVal(Cmd_BaseDirectory).c_str(), ""), RelativePath(Value);
            FullFilePath.SetNameExtension(RelativePath.GetNameExtension().c_str());
            FullFilePath.AppendDirectory(RelativePath.GetDirectory().c_str());

            Value = (const std::string &)FullFilePath;
        }
    }
    return bRes;
}