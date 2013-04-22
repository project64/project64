#pragma once

#define VERSION_COMPANY_NAME		"CompanyName"
#define VERSION_FILE_DESCRIPTION	"FileDescription"
#define VERSION_FILE_VERSION		"FileVersion"
#define VERSION_INTERNAL_NAME		"InternalName"
#define VERSION_LEGAL_COPYRIGHT		"LegalCopyright"
#define VERSION_ORIGINAL_FILE_NAME	"OriginalFileName"
#define VERSION_PRODUCT_NAME		"ProductName"
#define VERSION_PRODUCT_VERSION		"ProductVersion"

#include "std string.h"

bool   HasFileVersionInfo ( LPCSTR Info, LPCSTR FileName );
stdstr FileVersionInfo    ( LPCSTR Info, LPCSTR FileName );
stdstr VersionInfo        ( LPCSTR Info, HMODULE hModule = NULL);
