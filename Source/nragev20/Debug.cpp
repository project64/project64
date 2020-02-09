/*
    N-Rage`s Dinput8 Plugin
    (C) 2002, 2006  Norbert Wladyka

    Author`s Email: norbert.wladyka@chello.at
    Website: http://go.to/nrage


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>

#include "commonIncludes.h"
#include "FileAccess.h"

bool bDebug = true;


HANDLE hDebug = INVALID_HANDLE_VALUE;


void _DebugAnsiFileWrite( LPCSTR szRemark )
{
    if( !bDebug )
        return;

    if (hDebug == INVALID_HANDLE_VALUE)
    {
        TCHAR szFile[] = _T("NRage-Debug.txt");
        TCHAR szBuffer[MAX_PATH+1];

        GetAbsoluteFileName( szBuffer, szFile, DIRECTORY_LOG );
        hDebug = CreateFile( szBuffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if (hDebug != INVALID_HANDLE_VALUE)
            SetFilePointer(hDebug, 0, 0, FILE_END);
    }

    if( hDebug != INVALID_HANDLE_VALUE )
    {
        DWORD dwWritten;
        LPCSTR szText = szRemark;
        if( szText == NULL )
            szText = "\r\n";

        LPCSTR szCurrPos = szText;

        while( ( szCurrPos = strchr( szCurrPos, '\n' )) != NULL )
        {
            DWORD length = szCurrPos - szText;
            if( length > 0 && szCurrPos[-1] == '\r' )
                --length;

            if( length > 0 )
                WriteFile( hDebug, (LPCVOID)szText, length, &dwWritten, NULL );
            WriteFile( hDebug, "\r\n", 2, &dwWritten, NULL );

            szText = ++szCurrPos;
        }

        DWORD length = lstrlenA( szText );

        if( length > 0 )
            WriteFile( hDebug, (LPCVOID)szText, length, &dwWritten, NULL );
    }
    return;
}

void _cdecl _DebugWriteA( LPCSTR szFormat, ... )
{
    if( szFormat != NULL )
    {
        char szBuffer[4096];

        va_list val;

        va_start( val,szFormat );
        wvsprintfA( szBuffer, szFormat, val );
        va_end( val );
        szBuffer[sizeof(szBuffer)-1] = '\0';

        _DebugAnsiFileWrite( szBuffer );
    }
    else
        _DebugAnsiFileWrite( NULL );
}

void _cdecl _DebugWriteW( const LPCWSTR szFormat, ... )
{
    if( szFormat != NULL )
    {
        WCHAR szBuffer[4096];
        va_list val;

        va_start( val,szFormat );
        wvsprintfW( szBuffer, szFormat, val );
        va_end( val );
        szBuffer[(sizeof(szBuffer) / sizeof(WCHAR))-1] = L'\0';

        char szAnsi[sizeof(szBuffer) / sizeof(WCHAR)];
        WideCharToMultiByte( CP_ACP, 0, szBuffer, -1, szAnsi, sizeof(szAnsi), NULL, NULL );
        _DebugAnsiFileWrite( szAnsi );
    }
    else
        _DebugAnsiFileWrite( NULL );
}

void _WriteDatasA( LPCSTR Header, const int Control, const unsigned char * Data, const HRESULT hr )
{
    if( !bDebug || Data == NULL )
        return;

    _DebugWriteA( "%d:%s\n", Control, Header);

    if( hr )
    {
        _DebugWriteA( "Failed! ErrorCode: %08X\n", hr );
    }


    int iEnd = Data[0] + Data[1] + 2;

    for( int i = -1; i < iEnd; i += 8 )
    {
        _DebugWriteA( "%02X%02X%02X%02X %02X%02X%02X%02X\n", Data[i+0], Data[i+1], Data[i+2], Data[i+3], Data[i+4], Data[i+5], Data[i+6], Data[i+7] );
    }

    return;
}

void _CloseDebugFile()
{
    if( hDebug != INVALID_HANDLE_VALUE )
    {
        _DebugWriteA("---DEBUG FILE CLOSED---\n");
        CloseHandle( hDebug );
    }
}

void _DebugFlush()
{
    if( hDebug != INVALID_HANDLE_VALUE )
    {
        FlushFileBuffers(hDebug);
    }
}
