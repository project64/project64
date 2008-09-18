#include "..\..\Settings.h"
#include "SettingsType-RelativePath.h"

CSettingTypeRelativePath::CSettingTypeRelativePath(LPCSTR Path, LPCSTR FileName)
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,FileName);
	m_FileName.AppendDirectory(Path);
}

bool CSettingTypeRelativePath::Load ( int Index, stdstr & value ) const
{
	value = (LPCSTR)m_FileName;
	return true;
}

void CSettingTypeRelativePath::Save ( int Index, const stdstr & Value )
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,Value);
}

void CSettingTypeRelativePath::Save ( int Index, const char * Value )
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,Value);
}


