#pragma once

#define VERSION_COMPANY_NAME		TEXT("CompanyName")
#define VERSION_FILE_DESCRIPTION	TEXT("FileDescription")
#define VERSION_FILE_VERSION		TEXT("FileVersion")
#define VERSION_INTERNAL_NAME		TEXT("InternalName")
#define VERSION_LEGAL_COPYRIGHT		TEXT("LegalCopyright")
#define VERSION_ORIGINAL_FILE_NAME	TEXT("OriginalFileName")
#define VERSION_PRODUCT_NAME		TEXT("ProductName")
#define VERSION_PRODUCT_VERSION		TEXT("ProductVersion")

bool   HasFileVersionInfo ( LPCTSTR Info, LPCTSTR FileName );
stdstr FileVersionInfo    ( LPCTSTR Info, LPCTSTR FileName );
stdstr VersionInfo        ( LPCTSTR Info, HMODULE hModule = NULL);
