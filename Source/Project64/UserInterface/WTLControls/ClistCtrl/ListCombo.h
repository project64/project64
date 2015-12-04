
#pragma once
#include "ListTypes.h"
#include <wtl/atlcrack.h>

class CListCombo : public CWindowImpl< CListCombo, CComboBox >
{
public:
	CListCombo() : m_wndEditCtrl( this, 1 )
	{
		m_nItem = NULL_ITEM;
		m_nSubItem = NULL_SUBITEM;
		m_nFlags = ITEM_FLAGS_NONE;
		m_nExitChar = 0;
		m_bMouseOver = FALSE;
		m_bActivate = FALSE;
	}
	
	~CListCombo()
	{
	}

protected:
	int m_nItem;
	int m_nSubItem;
	UINT m_nFlags;
	TCHAR m_nExitChar;
	BOOL m_bMouseOver;
	BOOL m_bActivate;
	BOOL m_bSwappedButtons;
	CRect m_rcStatic;
	CRect m_rcButton;
	COLORREF m_rgbStaticBackground;
	COLORREF m_rgbStaticText;
	
	CFont m_fntComboFont;
	CContainedWindowT< CEdit > m_wndEditCtrl;
	
public:
	BOOL Create( HWND hWndParent, int nItem, int nSubItem, CRect& rcRect, UINT nFlags, LPCTSTR lpszItemText, CListArray < stdstr >& aComboList )
	{
		m_nItem = nItem;
		m_nSubItem = nSubItem;
		m_nFlags = nFlags;
		m_nExitChar = 0;
		m_bActivate = FALSE;
		
		m_bSwappedButtons = GetSystemMetrics( SM_SWAPBUTTON );
		m_rgbStaticBackground = GetSysColor( COLOR_HIGHLIGHT );
		m_rgbStaticText = GetSysColor( COLOR_HIGHLIGHTTEXT );
		
		// destroy old combo control...
		if ( IsWindow() )
			DestroyWindow();
			
		DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_SORT;
		
		if ( nFlags & ITEM_FLAGS_COMBO_EDIT )
			dwStyle |= CBS_DROPDOWN;
		else
			dwStyle |= CBS_DROPDOWNLIST;
			
		if ( nFlags & ITEM_FLAGS_EDIT_UPPER )
			dwStyle |= CBS_UPPERCASE;
		
		// create combo control
		CRect Area( ( ( dwStyle & CBS_DROPDOWNLIST ) == CBS_DROPDOWNLIST ) ? rcRect.left + 3 : rcRect.left + 1, rcRect.top, rcRect.right, rcRect.bottom + ( 6 * rcRect.Height() ) );
		if ( CWindowImpl< CListCombo, CComboBox >::Create( hWndParent, Area, NULL, dwStyle ) == NULL )
			return FALSE;
			
		// get system message font
		CLogFont logFont;
		logFont.SetMessageBoxFont();
		if ( !m_fntComboFont.IsNull() )
			m_fntComboFont.DeleteObject();
		if ( m_fntComboFont.CreateFontIndirect( &logFont ) == NULL )
			return FALSE;
		SetFont( m_fntComboFont, FALSE );
		
		// subclass edit control to capture keyboard input
		HWND hEditControl = GetWindow( GW_CHILD );
		if ( hEditControl != NULL )
			m_wndEditCtrl.SubclassWindow( hEditControl );
				
		for ( int nComboItem = 0; nComboItem < aComboList.GetSize(); nComboItem++ )
			AddString( aComboList[ nComboItem ].c_str() );
		
		if ( ( dwStyle & CBS_DROPDOWNLIST ) == CBS_DROPDOWNLIST )
		{
			int nIndex = FindStringExact( -1, lpszItemText );
			if ( nIndex != CB_ERR )
				SetCurSel( nIndex );
		}
		else
		{
			SetWindowText( lpszItemText );
			SetEditSel( 0, -1 );
		}
		
		// set static edit height
		SetItemHeight( -1, rcRect.Height() - 6 );
		
		COMBOBOXINFO infoComboBox = { sizeof( COMBOBOXINFO ) };
		if ( !::GetComboBoxInfo( m_hWnd, &infoComboBox ) )
			return FALSE;
		
		// store combobox details for painting		
		m_rcStatic = infoComboBox.rcItem;
		m_rcButton = infoComboBox.rcButton;
		m_rcButton.DeflateRect( 0, 1 );
		m_rcButton.OffsetRect( -2, 0 );
		
		// show combo control
		ShowWindow( SW_SHOW );

		SetFocus();
		
		// force redraw now
		RedrawWindow();
		
		return TRUE;
	}
	
	BOOL IsValid( TCHAR nChar )
	{
		// validate number and float input
		if ( !( m_nFlags & ( ITEM_FLAGS_EDIT_NUMBER | ITEM_FLAGS_EDIT_FLOAT ) ) || nChar == VK_BACK )
			return TRUE;
		
		stdstr strValue;
		int nValueLength = GetWindowTextLength() + 1;
		strValue.reserve(nValueLength);
		GetWindowText( (char *)strValue.c_str(), nValueLength );
		
		// get selected positions
		DWORD dwSelection = GetEditSel();		
		int nStartChar = LOWORD( dwSelection );
		int nEndChar = HIWORD( dwSelection );
		
		// are we changing the sign?
		if ( ( m_nFlags & ITEM_FLAGS_EDIT_NEGATIVE ) && nChar == _T( '-' ) )
		{
			BOOL bNegative = FALSE;
			if ( m_nFlags & ITEM_FLAGS_EDIT_FLOAT )
			{
				double dblValue = atof( strValue.c_str() );
				bNegative = ( dblValue < 0 );
				strValue.Format( _T( "%lf" ), -dblValue );
			}
			else
			{
				long lValue = _ttol( strValue.c_str() );
				bNegative = ( lValue < 0 );
				strValue.Format( _T( "%ld" ), -lValue );
			}
			
			SetWindowText( strValue.c_str() );
			
			// restore select position
			SetEditSel( bNegative ? nStartChar - 1 : nStartChar + 1, bNegative ? nEndChar - 1 : nEndChar + 1 );
			return FALSE;
		}
		
		// construct new value string using entered character
		stdstr strNewValue = strValue.substr(0, nStartChar ) + nChar + strValue.substr(nEndChar, strValue.length() - nEndChar );
		
		int nGreaterThan = 0;
		int nLessThan = 0;
		int nEquals = 0;
		int nDecimalPoint = 0;
		
		int nNegativeIndex = -1;
		int nGreaterIndex = -1;
		int nLessIndex = -1;
		int nEqualIndex = -1;
		int nDecimalIndex = -1;
		int nDigitIndex = -1;
		
		for ( int nCharIndex = 0; nCharIndex < (int)strNewValue.length(); nCharIndex++ )
		{
			TCHAR nCharValue = (strNewValue.c_str())[ nCharIndex ];
			switch ( nCharValue )
			{
				case _T( '-' ):	nNegativeIndex = nCharIndex;
								break;
				case _T( '>' ):	if ( !( m_nFlags & ITEM_FLAGS_EDIT_OPERATOR ) )
									return FALSE;
								nGreaterIndex = nCharIndex;
								nGreaterThan++;
								break;
				case _T( '<' ):	if ( !( m_nFlags & ITEM_FLAGS_EDIT_OPERATOR ) )
									return FALSE;
								nLessIndex = nCharIndex;
								nLessThan++;
								break;
				case _T( '=' ):	if ( !( m_nFlags & ITEM_FLAGS_EDIT_OPERATOR ) )
									return FALSE;
								nEqualIndex = nCharIndex;
								nEquals++;
								break;
				case _T( '.' ):	if ( !( m_nFlags & ITEM_FLAGS_EDIT_FLOAT ) )
									return FALSE;
								nDecimalIndex = nCharIndex;
								nDecimalPoint++;
								break;
				default:		if ( !_istdigit( nCharValue ) )
									return FALSE;
								if ( nDigitIndex < 0 )
									nDigitIndex = nCharIndex;
								break;
			}

			// invalid if text contains more than one '>', '<', '=' or '.'
			if ( nGreaterThan > 1 || nLessThan > 1 || nEquals > 1 || nDecimalPoint > 1 )
				return FALSE;
		}

		// invalid if text contains '=>' or '=<'
		if ( nGreaterIndex != -1 && nEqualIndex != -1 && nGreaterIndex > nEqualIndex )
			return FALSE;
		if ( nLessIndex != -1 && nEqualIndex != -1 && nLessIndex > nEqualIndex )
			return FALSE;

		// invalid if digits exist before operator
		if ( nDigitIndex != -1 && nGreaterIndex != -1 && nGreaterIndex > nDigitIndex )
			return FALSE;
		if ( nDigitIndex != -1 && nLessIndex != -1 && nLessIndex > nDigitIndex )
			return FALSE;
		if ( nDigitIndex != -1 && nEqualIndex != -1 && nEqualIndex > nDigitIndex )
			return FALSE;
		if ( nDigitIndex != -1 && nNegativeIndex != -1 && nNegativeIndex > nDigitIndex )
			return FALSE;
		
		return TRUE;
	}
	
	BEGIN_MSG_MAP_EX(CListCombo)
		MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST,WM_MOUSELAST,OnMouseRange)
#if(_WIN32_WINNT >= 0x0400)
		MSG_WM_MOUSELEAVE(OnMouseLeave)
#endif
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
		REFLECTED_COMMAND_CODE_HANDLER_EX(CBN_KILLFOCUS, OnKillFocus)
		MSG_WM_GETDLGCODE(OnGetDlgCode)
		MSG_WM_CHAR(OnChar)
		DEFAULT_REFLECTION_HANDLER()
	ALT_MSG_MAP(1)
		MSG_WM_GETDLGCODE(OnGetDlgCode)
		MSG_WM_CHAR(OnChar)
	END_MSG_MAP()
	
	LRESULT OnMouseRange( UINT nMessage, WPARAM /*wParam*/, LPARAM /*lParam*/ )
	{
		SetMsgHandled( FALSE );
		
		if ( nMessage == WM_MOUSEMOVE && !m_bMouseOver )
		{
			m_bMouseOver = TRUE;
			
			TRACKMOUSEEVENT trkMouse;
			trkMouse.cbSize = sizeof( TRACKMOUSEEVENT );
			trkMouse.dwFlags = TME_LEAVE;
			trkMouse.hwndTrack = m_hWnd;
			
			// notify when the mouse leaves button
			_TrackMouseEvent( &trkMouse );
		}
		
		// do not show button as pressed when first created
		m_bActivate = TRUE;
		
		InvalidateRect( m_rcButton );
		
		return 0;
	}
	
	void OnMouseLeave()
	{
		m_bMouseOver = FALSE;
		InvalidateRect( m_rcButton );
	}
	
	BOOL OnEraseBkgnd( CDCHandle dcErase ) 
	{
		return TRUE;
	}
	
	void OnPaint( HDC )
	{
		CPaintDC dcPaint( m_hWnd );
		
		CRect rcClient;
		GetClientRect( rcClient );
		
		CMemoryDC dcMemory( dcPaint.m_hDC, rcClient );
		
		CRect rcClip;
		if ( dcPaint.GetClipBox( rcClip ) == ERROR )
			return;
			
		int nContextState = dcMemory.SaveDC();		
		
		// do not repaint background if drawing button only
		if ( !rcClip.EqualRect( m_rcButton ) )
		{
			CWindow wndParent( GetParent() );
			if ( wndParent.IsWindow() )
			{
				// draw background from parent
				CPoint ptOrigin( 0 );
				MapWindowPoints( wndParent, &ptOrigin, 1 );
				dcMemory.OffsetWindowOrg( ptOrigin.x, ptOrigin.y, &ptOrigin );
				wndParent.SendMessage( WM_PAINT, (WPARAM)dcMemory.m_hDC );
				dcMemory.SetWindowOrg( ptOrigin );
			}
		}
		
		DWORD dwPoint = GetMessagePos();
		CPoint ptMouse( GET_X_LPARAM( dwPoint ), GET_Y_LPARAM( dwPoint ) );
		ScreenToClient( &ptMouse );
		
		DWORD dwStyle = GetStyle();	
		BOOL bHotButton = m_bActivate && ( ( ( dwStyle & CBS_DROPDOWNLIST ) == CBS_DROPDOWNLIST ) ? rcClient.PtInRect( ptMouse ) : m_rcButton.PtInRect( ptMouse ) );
		BOOL bPressed = bHotButton && ( GetAsyncKeyState( m_bSwappedButtons ? VK_RBUTTON : VK_LBUTTON ) < 0 );
		
		if ( ( dwStyle & CBS_DROPDOWNLIST ) == CBS_DROPDOWNLIST && !rcClip.EqualRect( m_rcButton ) )
		{
			dcMemory.SetBkColor( m_rgbStaticBackground );
			dcMemory.ExtTextOut( m_rcStatic.left, m_rcStatic.top, ETO_OPAQUE, m_rcStatic, _T( "" ), 0, NULL );
			
			// draw static text
			int nIndex = GetCurSel();
			if ( nIndex != CB_ERR )
			{
				stdstr strText;
				int cchLen = GetLBTextLen(nIndex);
				if(cchLen != CB_ERR)
				{
					strText.reserve(cchLen + 1);
					GetLBText( nIndex, (LPTSTR)strText.c_str() );
				}
			
				if ( !strText.empty() )
				{
					CRect rcText( m_rcStatic );
					rcText.OffsetRect( 1, 1 );					
					dcMemory.SelectFont( m_fntComboFont );
					dcMemory.SetTextColor( m_rgbStaticText );
					dcMemory.SetBkMode( TRANSPARENT );
					dcMemory.DrawText( strText.c_str(), (int)strText.length(), rcText, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER );
				}
			}
		}
		
		// draw drop down button
		dcMemory.DrawFrameControl( m_rcButton, DFC_SCROLL, DFCS_SCROLLDOWN | ( bPressed ? DFCS_FLAT | DFCS_PUSHED : 0 ) );

		dcMemory.RestoreDC( nContextState );
	}
	
	void OnKillFocus( UINT /*uCode*/, int /*nCtrlID*/, HWND /*hwndCtrl*/ )
	{
		SetMsgHandled( FALSE );
		
		CWindow wndParent( GetParent() );
		if ( wndParent.IsWindow() )
		{
			stdstr strValue;
			
			if ( ( GetStyle() & CBS_DROPDOWNLIST ) == CBS_DROPDOWNLIST )
			{
				int nIndex = GetCurSel();
				if ( nIndex != CB_ERR )
				{
					int cchLen = GetLBTextLen(nIndex);
					if(cchLen != CB_ERR)
					{
						strValue.reserve(cchLen);
						GetLBText( nIndex, (LPTSTR)strValue.c_str() );
					}
					else 
					{
						strValue = "";
					}
				}
			}
			else
			{
				int nValueLength = GetWindowTextLength() + 1;
				strValue.reserve(nValueLength);
				GetWindowText( (LPTSTR)strValue.c_str(), nValueLength );
			}
			
			CListNotify listNotify;
			listNotify.m_hdrNotify.hwndFrom = m_hWnd;
			listNotify.m_hdrNotify.idFrom = GetDlgCtrlID();
			listNotify.m_hdrNotify.code = LCN_ENDEDIT;
			listNotify.m_nItem = m_nItem;
			listNotify.m_nSubItem = m_nSubItem;
			listNotify.m_nExitChar = m_nExitChar;
			listNotify.m_lpszItemText = strValue.c_str();
			listNotify.m_lpItemDate = NULL;

			// forward notification to parent
			FORWARD_WM_NOTIFY( wndParent, listNotify.m_hdrNotify.idFrom, &listNotify.m_hdrNotify, ::SendMessage );
		}
		
		ShowWindow( SW_HIDE );
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
			default:		SetMsgHandled( !IsValid( nChar ) );
							break;
		}
	}
};
