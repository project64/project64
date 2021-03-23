#pragma once

#include "ListTypes.h"

#define DATE_STRING					32

class CListDate : public CWindowImpl< CListDate, CDateTimePickerCtrl >
{
public:
	CListDate()
	{
		m_nItem = NULL_ITEM;
		m_nSubItem = NULL_SUBITEM;
		m_nFlags = ITEM_FLAGS_NONE;
		m_nExitChar = 0;
	}
	
	~CListDate()
	{
	}

protected:
	int m_nItem;
	int m_nSubItem;
	UINT m_nFlags;
	TCHAR m_nExitChar;
	CFont m_fntDateFont;
	
public:
	BOOL Create( HWND hWndParent, int nItem, int nSubItem, CRect& rcRect, UINT nFlags, SYSTEMTIME& stItemDate )
	{
		m_nItem = nItem;
		m_nSubItem = nSubItem;
		m_nFlags = nFlags;
		m_nExitChar = 0;
		
		// Destroy old date control
		if ( IsWindow() )
			DestroyWindow();
			
		DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN;
		
		if ( nFlags & ITEM_FLAGS_DATETIME_NONE )
			dwStyle |= DTS_SHOWNONE;
		
		if ( nFlags & ITEM_FLAGS_TIME_ONLY )
			dwStyle |= DTS_UPDOWN;
		
		// Create date-time control
		CRect Area( rcRect.left + 3, rcRect.top + 2, rcRect.right - 3, rcRect.bottom - 2 );
		if ( CWindowImpl< CListDate, CDateTimePickerCtrl >::Create( hWndParent, Area, NULL, dwStyle ) == NULL )
			return FALSE;
		
		// Remove border
		ModifyStyleEx( WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED );
		
		// Get system message font
		CLogFont logFont;
		logFont.SetMessageBoxFont();
		if ( !m_fntDateFont.IsNull() )
			m_fntDateFont.DeleteObject();
		if ( m_fntDateFont.CreateFontIndirect( &logFont ) == NULL )
			return FALSE;
		SetMonthCalFont( m_fntDateFont );
		SetFont( m_fntDateFont );
		
		TCHAR szDateFormat[ DATE_STRING ];
		GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szDateFormat, DATE_STRING );
		
		TCHAR szTimeFormat[ DATE_STRING ];
		GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, szTimeFormat, DATE_STRING );
		
		if ( nFlags & ITEM_FLAGS_DATE_ONLY )
			SetFormat( szDateFormat );
		else if ( nFlags & ITEM_FLAGS_TIME_ONLY )
			SetFormat( szTimeFormat );
		else
			SetFormat(stdstr_f("%s %s",szDateFormat,szTimeFormat ).ToUTF16().c_str());
		
		// Get current date if setting time-only
		if ( nFlags & ITEM_FLAGS_TIME_ONLY )
		{
			SYSTEMTIME stCurrentDate;
			if ( GetSystemTime( &stCurrentDate ) == GDT_VALID )
			{
				stItemDate.wYear = stCurrentDate.wYear;
				stItemDate.wMonth = stCurrentDate.wMonth;
				stItemDate.wDay = stCurrentDate.wDay;
			}
		}
		
		SetSystemTime( ( !( nFlags & ITEM_FLAGS_TIME_ONLY ) && stItemDate.wYear == 0 ) ? GDT_NONE : GDT_VALID, &stItemDate );
		
		// Show date-time control
		ShowWindow( SW_SHOW );
		
		SetFocus();
		
		return TRUE;
	}
		
	BEGIN_MSG_MAP_EX(CListDate)
		MSG_WM_KILLFOCUS(OnKillFocus)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(DTN_CLOSEUP, OnCloseUp)
		MSG_WM_GETDLGCODE(OnGetDlgCode)
		MSG_WM_CHAR(OnChar)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()
	
	void OnKillFocus( HWND hNewWnd )
	{
		// Have we dropped down the calendar control?
		if ( hNewWnd != NULL && GetMonthCal() == hNewWnd )
			return;
		
		// Have we selected a new date from the calendar control?
		if ( GetFocus() == m_hWnd )
			return;
		
		// Hide calendar control in case it's not closed by losing focus
		if ( GetMonthCal().IsWindow() )
			GetMonthCal().ShowWindow( SW_HIDE );
		
		CWindow wndParent( GetParent() );
		if ( wndParent.IsWindow() )
		{
			SYSTEMTIME stItemDate;
			BOOL bValidDate = ( GetSystemTime( &stItemDate ) == GDT_VALID );
			if ( !bValidDate )
				ZeroMemory( &stItemDate, sizeof( SYSTEMTIME ) );
			
			if ( m_nFlags & ITEM_FLAGS_DATE_ONLY )
			{
				stItemDate.wHour = 0;
				stItemDate.wMinute = 0;
				stItemDate.wSecond = 0;
				stItemDate.wMilliseconds = 0;
			}
			
			if ( m_nFlags & ITEM_FLAGS_TIME_ONLY )
			{
				stItemDate.wYear = 0;
				stItemDate.wMonth = 0;
				stItemDate.wDay = 0;
				stItemDate.wDayOfWeek = 0;				
			}
				
			CListNotify listNotify;
			listNotify.m_hdrNotify.hwndFrom = m_hWnd;
			listNotify.m_hdrNotify.idFrom = GetDlgCtrlID();
			listNotify.m_hdrNotify.code = LCN_ENDEDIT;
			listNotify.m_nItem = m_nItem;
			listNotify.m_nSubItem = m_nSubItem;
			listNotify.m_nExitChar = m_nExitChar;
			listNotify.m_lpszItemText = bValidDate ? _T( "1" ) : _T( "0" );
			listNotify.m_lpItemDate = &stItemDate;

			// Forward notification to parent
			FORWARD_WM_NOTIFY( wndParent, listNotify.m_hdrNotify.idFrom, &listNotify.m_hdrNotify, ::SendMessage );
		}
		
		ShowWindow( SW_HIDE );
	}
	
	LRESULT OnCloseUp( LPNMHDR /*lpNMHDR*/ )
	{
		SetMsgHandled( FALSE );
		SetFocus();
		return TRUE;
	}
	
	UINT OnGetDlgCode( LPMSG /*lpMessage*/ )
	{
		return DLGC_WANTALLKEYS;
	}
	
	void OnChar( TCHAR nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ )
	{
		switch ( nChar )
		{
			case VK_TAB:
			case VK_RETURN:
			case VK_ESCAPE:	{
								m_nExitChar = nChar;
								CWindow wndParent( GetParent() );
								if ( wndParent.IsWindow() )
									wndParent.SetFocus();
							}
							break;
			default:		SetMsgHandled( FALSE );
							break;
		}
	}
};
