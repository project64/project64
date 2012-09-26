#pragma once

class CSettingTypeApplicationPath :
	public CSettingTypeApplication
{
private:
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, bool DefaultValue );
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );

public:
	virtual ~CSettingTypeApplicationPath();

	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );

	//return the values
	virtual bool Load   ( int Index, stdstr & Value ) const;
};

