/////////////////////////////////////////////////////////////////////////////
// 
// CListCtrl - A WTL list control with Windows Vista style item selection.
//
// Revision:      1.5
// Last modified: 2nd November 2016
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <set>
#include <algorithm>
#pragma warning(push)
#pragma warning(disable : 4838) // warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#include <wtl/atlctrlx.h>
#include <wtl/atlframe.h>
#include <wtl/atlmisc.h>
#include <wtl/atlgdi.h>
#pragma warning(pop)

#include "DragDrop.h"
#include "DropArrows.h"
#include "TitleTip.h"
#include "ListEdit.h"
#include "ListCombo.h"
#include "ListDate.h"

struct CListColumn
{
	stdstr m_strText;
	int m_nWidth;
	BOOL m_bFixed;
	UINT m_nFormat;
	UINT m_nFlags;
	int m_nImage;
	int m_nIndex;	
	CListArray < stdstr > m_aComboList;
};

template < class T >
class CListImpl : public CWindowImpl< CListImpl< T > >,
				  public CDoubleBufferImpl< CListImpl< T > >
{
public:
	CListImpl()
	{
		m_bSortEnabled = TRUE; // Added by Rowan 05/12/2006
		m_bRightClickSelect = FALSE; // shygoo 2016 Nov 2
		m_bShowHeader = TRUE;
		m_bSortAscending = TRUE;
		m_bButtonDown = FALSE;
		m_bMouseOver = FALSE;
		m_bColumnSizing = FALSE;
		m_bBeginSelect = FALSE;
		m_bSingleSelect = FALSE;
		m_bFocusSubItem = FALSE;
		m_bGroupSelect = FALSE;
		m_bEnableHorizScroll = FALSE;
		m_bEnableVertScroll = FALSE;
		m_bShowHorizScroll = TRUE;
		m_bShowVertScroll = TRUE;
		m_bShowSort = TRUE;
		m_bResizeTimer = FALSE;
		m_bDragDrop = FALSE;
		m_bSmoothScroll = TRUE;
		m_bEditItem = FALSE;
		m_bScrolling = FALSE;
		m_bScrollDown = FALSE;
		m_bTileBackground = FALSE;
		m_nMouseWheelScroll = 3;
		m_nTotalWidth = 0;
		m_nHeaderHeight = 0;
		m_nItemHeight = 0;
		m_nFirstSelected = NULL_ITEM;
		m_nFocusItem = NULL_ITEM;
		m_nFocusSubItem = NULL_SUBITEM;
		m_nHotItem = NULL_ITEM;
		m_nHotSubItem = NULL_SUBITEM;
		m_nTitleTipItem = NULL_ITEM;
		m_nTitleTipSubItem = NULL_SUBITEM;
		m_nSortColumn = NULL_COLUMN;
		m_nHighlightColumn = NULL_COLUMN;
		m_nDragColumn = NULL_COLUMN;
		m_nHotColumn = NULL_COLUMN;
		m_nHotDivider = NULL_COLUMN;
		m_nColumnSizing = NULL_COLUMN;
		m_nScrollOffset = 0;
		m_nScrollDelta = 0;
		m_nScrollUnit = 0;
		m_nStartScrollPos = 0;
		m_nStartSize = 0;
		m_nStartPos = 0;
		m_ptDownPoint = 0;
		m_ptSelectPoint = 0;
		m_rcGroupSelect = 0;
		m_dwSearchTick = 0;
		m_dwScrollTick = 0;
		m_strSearchString = _T( "" );
	}
	
	~CListImpl()
	{
		if (m_wndItemEdit.IsWindow())
		{
			// patch memory window crash
			m_wndItemEdit.UnsubclassWindow();
		}
	}

protected:
	BOOL m_bSortEnabled; // Added by Rowan 05/12/2006 to disable sorting
	BOOL m_bRightClickSelect; // shygoo 2016 Nov 2
	BOOL m_bShowHeader;
	BOOL m_bShowSort;
	BOOL m_bSortAscending;
	BOOL m_bButtonDown;
	BOOL m_bMouseOver;
	BOOL m_bColumnSizing;
	BOOL m_bBeginSelect;
	BOOL m_bSingleSelect;
	BOOL m_bFocusSubItem;
	BOOL m_bGroupSelect;
	BOOL m_bShowHorizScroll;
	BOOL m_bShowVertScroll;
	BOOL m_bEnableHorizScroll;
	BOOL m_bEnableVertScroll;
	BOOL m_bResizeTimer;
	BOOL m_bDragDrop;
	BOOL m_bSmoothScroll;
	BOOL m_bEditItem;
	BOOL m_bScrolling;
	BOOL m_bScrollDown;
	BOOL m_bTileBackground;
	CPoint m_ptDownPoint;
	CPoint m_ptSelectPoint;
	CRect m_rcGroupSelect;
	int m_nItemHeight;
	int m_nHeaderHeight;
	int m_nFirstSelected;
	int m_nFocusItem;
	int m_nFocusSubItem;
	int m_nHotItem;
	int m_nHotSubItem;
	int m_nTitleTipItem;
	int m_nTitleTipSubItem;
	int m_nMouseWheelScroll;
	int m_nTotalWidth;
	int m_nSortColumn;
	int m_nDragColumn;
	int m_nHighlightColumn;
	int m_nHotColumn;
	int m_nHotDivider;
	int m_nColumnSizing;
	int m_nScrollOffset;
	int m_nScrollDelta;
	int m_nScrollUnit;
	int m_nStartScrollPos;
	int m_nStartSize;
	int m_nStartPos;
	DWORD m_dwSearchTick;
	DWORD m_dwScrollTick;
	stdstr m_strSearchString;
	CBitmap m_bmpScrollList;
	CBitmap m_bmpBackground;
	
	CLIPFORMAT m_nHeaderClipboardFormat;
	
	COLORREF m_rgbBackground;
	COLORREF m_rgbHeaderBackground;
	COLORREF m_rgbHeaderBorder;
	COLORREF m_rgbHeaderShadow;
	COLORREF m_rgbHeaderText;
	COLORREF m_rgbHeaderHighlight;
	COLORREF m_rgbSelectedItem;
	COLORREF m_rgbSelectedText;
	COLORREF m_rgbItemText;
	COLORREF m_rgbSelectOuter;
	COLORREF m_rgbSelectInner;
	COLORREF m_rgbSelectTop;
	COLORREF m_rgbSelectBottom;
	COLORREF m_rgbNoFocusTop;
	COLORREF m_rgbNoFocusBottom;
	COLORREF m_rgbNoFocusOuter;
	COLORREF m_rgbNoFocusInner;
	COLORREF m_rgbFocusTop;
	COLORREF m_rgbFocusBottom;
	COLORREF m_rgbProgressTop;
	COLORREF m_rgbProgressBottom;
	COLORREF m_rgbItemFocus;
	COLORREF m_rgbHyperLink;
	
	CCursor m_curDivider;
	CCursor m_curHyperLink;
	CFont m_fntListFont;
	CFont m_fntUnderlineFont;
	CImageList m_ilListItems;
	CImageList m_ilItemImages;
	CDragDrop < CListImpl > m_oleDragDrop;
	CToolTipCtrl m_ttToolTip;
	CDropArrows m_wndDropArrows;
	CTitleTip m_wndTitleTip;
	CListEdit m_wndItemEdit;
	CListCombo m_wndItemCombo;
	CListDate m_wndItemDate;
	
	CListArray < CListColumn > m_aColumns;
	set < int > m_setSelectedItems;
public:
	BOOL SubclassWindow( HWND hWnd )
	{
		T* pT;
		pT = static_cast<T*>(this);
		return CWindowImpl< CListImpl >::SubclassWindow( hWnd ) ? pT->Initialise() : FALSE;
	}
	
	void RegisterClass()
	{
		T* pT = static_cast<T*>(this);
		pT = pT;
		pT->GetWndClassInfo().m_wc.lpfnWndProc = m_pfnSuperWindowProc;
		pT->GetWndClassInfo().Register( &m_pfnSuperWindowProc );
	}
	
	BOOL Initialise()
	{
		// load list images
		if ( !m_ilListItems.CreateFromImage( IDB_LISTITEMS, 16, 0, RGB( 255, 0, 255 ), IMAGE_BITMAP, LR_CREATEDIBSECTION ) )
			return FALSE;
		
		if ( m_curDivider.LoadCursor( IDC_DIVIDER ) == NULL )
			return FALSE;
		if ( m_curHyperLink.LoadCursor( IDC_HYPERLINK ) == NULL )
			return FALSE;
		
		// load interface settings
		if ( !LoadSettings() )
			return FALSE;
			
		// give control a static border
		ModifyStyle( WS_BORDER, WS_CLIPCHILDREN );
		ModifyStyleEx( WS_EX_CLIENTEDGE, WS_EX_STATICEDGE, SWP_FRAMECHANGED );		
		
		// register drag drop
		m_oleDragDrop.Register( this );
		m_oleDragDrop.AddTargetFormat( m_nHeaderClipboardFormat );
		m_oleDragDrop.AddSourceFormat( m_nHeaderClipboardFormat );
		
		// create the tooltip
		if ( !m_ttToolTip.Create( m_hWnd ) )
			return FALSE;
		m_ttToolTip.SetMaxTipWidth( SHRT_MAX );
		
		return TRUE;
	}
	
	BOOL LoadSettings()
	{
		m_rgbBackground = GetSysColor( COLOR_WINDOW );
		m_rgbHeaderBackground = GetSysColor( COLOR_BTNFACE );
		m_rgbHeaderBorder = GetSysColor( COLOR_3DHIGHLIGHT );
		m_rgbHeaderShadow = GetSysColor( COLOR_3DSHADOW );
		m_rgbHeaderText = GetSysColor( COLOR_WINDOWTEXT );
		m_rgbHeaderHighlight = RGB( 130, 140, 180 );
		m_rgbSelectedItem = GetSysColor( COLOR_HIGHLIGHT );
		m_rgbSelectedText = GetSysColor( COLOR_HIGHLIGHTTEXT );
		m_rgbItemText = GetSysColor( COLOR_WINDOWTEXT );
		m_rgbSelectOuter = RGB( 170, 200, 245 );
		m_rgbSelectInner = RGB( 230, 250, 250 );
		m_rgbSelectTop = RGB( 210, 240, 250 );
		m_rgbSelectBottom = RGB( 185, 215, 250 );
		m_rgbNoFocusTop = RGB( 250, 250, 250 );
		m_rgbNoFocusBottom = RGB( 235, 235, 235 );
		m_rgbNoFocusOuter = RGB( 220, 220, 220 );
		m_rgbNoFocusInner = RGB( 245, 245, 245 );
		m_rgbFocusTop = RGB( 235, 245, 245 );
		m_rgbFocusBottom = RGB( 225, 235, 245 );
		m_rgbProgressTop = RGB( 170, 240, 170 );
		m_rgbProgressBottom = RGB( 45, 210, 50 );
		m_rgbItemFocus = RGB( 180, 190, 210 );
		m_rgbHyperLink = RGB( 0, 0, 200 );
		
		m_nHeaderClipboardFormat = (CLIPFORMAT)RegisterClipboardFormat( _T( "HEADERCLIPBOARDFORMAT" ) );
		
		// get number of lines to scroll
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &m_nMouseWheelScroll, 0 );
#endif
		
		// get system message font
		CLogFont logFont;
		logFont.SetMessageBoxFont();
		if ( !m_fntListFont.IsNull() )
			m_fntListFont.DeleteObject();
		if ( m_fntListFont.CreateFontIndirect( &logFont ) == NULL )
			return FALSE;
		
		// get system underline font
		logFont.lfUnderline = BYTE(TRUE);
		if ( !m_fntUnderlineFont.IsNull() )
			m_fntUnderlineFont.DeleteObject();
		if ( m_fntUnderlineFont.CreateFontIndirect( &logFont ) == NULL )
			return FALSE;
		
		CClientDC dcClient( m_hWnd );
		
		HFONT hOldFont = dcClient.SelectFont( m_fntListFont );
		
		CSize sizeExtent;
		if ( !dcClient.GetTextExtent( _T( "Height" ), -1, &sizeExtent ) )
			return FALSE;
		
		dcClient.SelectFont( hOldFont );
		
		// has system font changed
		if ( m_nItemHeight != sizeExtent.cy + ITEM_HEIGHT_MARGIN )
		{
			m_nItemHeight = sizeExtent.cy + ITEM_HEIGHT_MARGIN;
			m_nHeaderHeight = m_nItemHeight;
			
			// create drop arrows window
			if ( m_wndDropArrows.IsWindow() )
				m_wndDropArrows.DestroyWindow();			
			if ( !m_wndDropArrows.Create( m_hWnd, m_nHeaderHeight, TRUE ) )
				return FALSE;
		}
		
		// create titletip window
		if ( m_wndTitleTip.IsWindow() )
			m_wndTitleTip.DestroyWindow();			
		if ( !m_wndTitleTip.Create( m_hWnd ) )
			return FALSE;
		
		return TRUE;
	}
		
	// Added by Rowan 05/12/2006
	void SetSortEnabled(BOOL bSortEnabled)
	{
		m_bSortEnabled = bSortEnabled;
	}
	
	// shygoo 2016 Nov 2
	void SetRightClickSelect( BOOL bRightClickSelect = TRUE)
	{
		m_bRightClickSelect = bRightClickSelect;
	}

	void ShowHeader( BOOL bShowHeader = TRUE )
	{
		m_bShowHeader = bShowHeader;		
		ResetScrollBars();
		Invalidate();
	}

	void ShowHeaderSort( BOOL bShowSort = TRUE )
	{
		m_bShowSort = bShowSort;
		Invalidate();
	}

	void SetSingleSelect( BOOL bSingleSelect = TRUE )
	{
		m_bSingleSelect = bSingleSelect;
		Invalidate();
	}
	
	void SetFocusSubItem( BOOL bFocusSubItem = TRUE )
	{
		m_bFocusSubItem = bFocusSubItem;
		Invalidate();
	}
	
	void SetDragDrop( BOOL bDragDrop = TRUE )
	{
		m_bDragDrop = bDragDrop;
	}
	
	void SetSmoothScroll( BOOL bSmoothScroll = TRUE )
	{
		m_bSmoothScroll = bSmoothScroll;
	}
	
	void SetBackgroundImage( HBITMAP hBackgroundImage, BOOL bTileImage = FALSE )
	{
		m_bmpBackground = hBackgroundImage;
		m_bTileBackground = bTileImage;
	}
	
	void SetImageList( CImageList& ilItemImages )
	{
		m_ilItemImages = ilItemImages;
	}
	
	UINT ValidateFlags( UINT nFlags )
	{
		if ( nFlags & ITEM_FLAGS_CENTRE )
			nFlags &= ~( ITEM_FLAGS_LEFT | ITEM_FLAGS_RIGHT );
		if ( nFlags & ITEM_FLAGS_RIGHT )
			nFlags &= ~ITEM_FLAGS_LEFT;
		if ( nFlags & ITEM_FLAGS_DATE_ONLY )
			nFlags &= ~ITEM_FLAGS_TIME_ONLY;
		if ( nFlags & ( ITEM_FLAGS_EDIT_NUMBER | ITEM_FLAGS_EDIT_FLOAT ) )
			nFlags &= ~ITEM_FLAGS_EDIT_UPPER;
		if ( !( nFlags & ( ITEM_FLAGS_EDIT_NUMBER | ITEM_FLAGS_EDIT_FLOAT ) ) )
			nFlags &= ~( ITEM_FLAGS_EDIT_NEGATIVE | ITEM_FLAGS_EDIT_OPERATOR );
		if ( nFlags & ITEM_FLAGS_COMBO_EDIT )
			nFlags &= ~( ITEM_FLAGS_DATE_ONLY | ITEM_FLAGS_TIME_ONLY | ITEM_FLAGS_DATETIME_NONE );
		return nFlags;
	}
	
	void AddColumn( CListColumn& listColumn )
	{
		// minimum column width
		if ( listColumn.m_strText.empty() && listColumn.m_nImage != ITEM_IMAGE_NONE )
		{
			CSize sizeIcon;
			m_ilListItems.GetIconSize( sizeIcon );
			listColumn.m_nWidth = sizeIcon.cx + 5;
			listColumn.m_nFlags |= ITEM_FLAGS_CENTRE;
		}
		
		// correct incompatible flag mask values
		listColumn.m_nFlags = ValidateFlags( listColumn.m_nFlags );
		
		// initial data index
		listColumn.m_nIndex = GetColumnCount();
		
		m_aColumns.Add( listColumn );
		
		ResetScrollBars();
		Invalidate();
	}
	
	void AddColumn( LPCTSTR lpszText, int nWidth = 0, int nImage = ITEM_IMAGE_NONE, BOOL bFixed = FALSE, UINT nFormat = ITEM_FORMAT_NONE, UINT nFlags = ITEM_FLAGS_NONE )
	{
		CListColumn listColumn;
		listColumn.m_strText = lpszText;
		listColumn.m_nWidth = nWidth;
		listColumn.m_bFixed = bFixed;
		listColumn.m_nFormat = nFormat;
		listColumn.m_nFlags = nFlags;
		listColumn.m_nImage = nImage;
		AddColumn( listColumn );
	}

	void RemoveAllCoumns ( void )
	{
		m_aColumns.RemoveAll();
		ResetScrollBars();
		Invalidate();
	}
	
	BOOL GetHasEditItem ()
	{
		return m_bEditItem;
	}

	int GetColumnCount()
	{
		return m_aColumns.GetSize();
	}
	
	BOOL GetColumn( int nColumn, CListColumn& listColumn )
	{
		if ( nColumn < 0 || nColumn >= GetColumnCount() ) 
			return FALSE;
		listColumn = m_aColumns[ nColumn ];
		return TRUE;
	}
		
	int GetTotalWidth( BOOL bRecalc = FALSE )
	{
		if ( bRecalc )
		{
			m_nTotalWidth = 0;
			for ( int nColumn = 0; nColumn < GetColumnCount(); nColumn++ )
				m_nTotalWidth += GetColumnWidth( nColumn );
		}		
		return m_nTotalWidth - 1;
	}
	
	int GetTotalHeight()
	{
		T* pT = static_cast<T*>(this);
		return max( ( pT->GetItemCount() * m_nItemHeight ) + ( m_bShowHeader ? m_nHeaderHeight : 0 ), 1 );
	}
	
	BOOL SetColumnWidth( int nColumn, int nWidth )
	{
		if ( nColumn < 0 || nColumn >= GetColumnCount() ) 
			return FALSE;
		
		// set new column size if not fixed
		if ( !m_aColumns[ nColumn ].m_bFixed )
		{
			m_aColumns[ nColumn ].m_nWidth = nWidth;
			
			ResetScrollBars();
			Invalidate();
		}
		
		return TRUE;
	}

	int GetColumnWidth( int nColumn )
	{
		CListColumn listColumn;
		return GetColumn( nColumn, listColumn ) ? listColumn.m_nWidth : 0;
	}
	
	int GetColumnIndex( int nColumn )
	{
		CListColumn listColumn;
		return GetColumn( nColumn, listColumn ) ? listColumn.m_nIndex : 0;
	}
	
	int IndexToOrder( int nIndex )
	{
		for ( int nColumn = 0; nColumn < GetColumnCount(); nColumn++ )
		{
			if ( GetColumnIndex( nColumn ) == nIndex )
				return nColumn;
		}
		return -1;
	}
	
	BOOL SetColumnFormat( int nColumn, UINT nFormat, UINT nFlags = ITEM_FLAGS_NONE )
	{
		if ( nColumn < 0 || nColumn >= GetColumnCount() ) 
			return FALSE;
		m_aColumns[ nColumn ].m_nFormat = nFormat;
		m_aColumns[ nColumn ].m_nFlags = ValidateFlags( nFlags );
		return TRUE;
	}
	
	BOOL SetColumnFormat( int nColumn, UINT nFormat, UINT nFlags, CListArray < stdstr >& aComboList )
	{
		if ( nColumn < 0 || nColumn >= GetColumnCount() ) 
			return FALSE;
		m_aColumns[ nColumn ].m_nFormat = nFormat;
		m_aColumns[ nColumn ].m_nFlags = ValidateFlags( nFlags );
		m_aColumns[ nColumn ].m_aComboList = aComboList;
		return TRUE;
	}
	
	UINT GetColumnFormat( int nColumn )
	{
		CListColumn listColumn;
		return GetColumn( nColumn, listColumn ) ? listColumn.m_nFormat : ITEM_FORMAT_NONE;
	}
	
	UINT GetColumnFlags( int nColumn )
	{
		CListColumn listColumn;
		return GetColumn( nColumn, listColumn ) ? listColumn.m_nFlags : ITEM_FLAGS_NONE;
	}
	
	BOOL GetColumnComboList( int nColumn, CListArray < stdstr >& aComboList )
	{
		CListColumn listColumn;
		if ( !GetColumn( nColumn, listColumn ) )
			return FALSE;
		aComboList = listColumn.m_aComboList;
		return !aComboList.IsEmpty();		
	}
	
	BOOL GetColumnRect( int nColumn, CRect& rcColumn )
	{
		if ( nColumn < 0 || nColumn >= GetColumnCount() ) 
			return FALSE;
			
		GetClientRect( rcColumn );
		rcColumn.bottom = m_nHeaderHeight;
		
		for ( int nColumnOrder = 0; nColumnOrder < GetColumnCount(); nColumnOrder++ )
		{
			int nWidth = GetColumnWidth( nColumnOrder );
			
			if ( nColumn == nColumnOrder )
			{
				rcColumn.right = rcColumn.left + nWidth;
				break;
			}
			
			rcColumn.left += nWidth;
		}
		
		// offset column by scroll position
		rcColumn.OffsetRect( -GetScrollPos( SB_HORZ ), 0 );
		
		return TRUE;
	}
	
	BOOL AddItem()
	{
		ResetScrollBars();
		return Invalidate();
	}
	
	BOOL DeleteItem( int nItem )
	{
		m_setSelectedItems.erase( nItem );		
		ResetScrollBars();
		return Invalidate();
	}
	
	BOOL DeleteAllItems()
	{
		m_setSelectedItems.clear();		
		ResetScrollBars();
		return Invalidate();
	}
	
	int GetItemCount()
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return 0;
	}
	
	stdstr GetItemText( int nItem, int nSubItem )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return _T( "" );
	}
	
	BOOL GetItemDate( int nItem, int nSubItem, SYSTEMTIME& stItemDate )
	{
		T* pT = static_cast<T*>(this);
		
		ZeroMemory( &stItemDate, sizeof( SYSTEMTIME ) );
		
		stdstr strItemText = pT->GetItemText( nItem, nSubItem );
		if ( strItemText.empty() )
			return FALSE;
		
		// get date-time from item text: yyyymmddhhmmss
		stItemDate.wYear = (WORD)_ttoi( strItemText.substr(0, 4 ).c_str() );
		stItemDate.wMonth = (WORD)_ttoi( strItemText.substr( 4, 2 ).c_str() );
		stItemDate.wDay = (WORD)_ttoi( strItemText.substr( 6, 2 ).c_str() );
		stItemDate.wHour = (WORD)_ttoi( strItemText.substr( 8, 2 ).c_str() );
		stItemDate.wMinute = (WORD)_ttoi( strItemText.substr( 10, 2 ).c_str() );
		stItemDate.wSecond = (WORD)_ttoi( strItemText.substr( 12, 2 ).c_str() );
		stItemDate.wMilliseconds = 0;
		
		return TRUE;
	}
	
	int GetItemImage( int nItem, int nSubItem )
	{
		return ITEM_IMAGE_NONE; // may be implemented in a derived class
	}
	
	UINT GetItemFormat( int nItem, int nSubItem )
	{
		return GetColumnFormat( IndexToOrder( nSubItem ) ); // may be implemented in a derived class
	}
	
	UINT GetItemFlags( int nItem, int nSubItem )
	{
		return GetColumnFlags( IndexToOrder( nSubItem ) ); // may be implemented in a derived class
	}
	
	BOOL GetItemComboList( int nItem, int nSubItem, CListArray < stdstr >& aComboList )
	{
		return GetColumnComboList( IndexToOrder( nSubItem ), aComboList ); // may be implemented in a derived class
	}
	
	HFONT GetItemFont( int /*nItem*/, int /*nSubItem*/ )
	{
		return m_fntListFont; // may be implemented in a derived class
	}
	
	BOOL GetItemColours( int nItem, int nSubItem, COLORREF& rgbBackground, COLORREF& rgbText )
	{
		rgbBackground = m_rgbBackground;
		rgbText = m_rgbItemText;
		return TRUE;
	}
	
	stdstr virtual GetItemToolTip( int /*nItem*/, int /*nSubItem*/ )
	{
		return _T( "" ); // may be implemented in a derived class
	}
	stdstr virtual GetHeaderToolTip(int /*column*/)
	{
		return _T("");	//implemented by child class
	}


	BOOL SetItemText( int nItem, int nSubItem, LPCTSTR lpszText )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemComboIndex( int nItem, int nSubItem, int nIndex )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemDate( int nItem, int nSubItem, SYSTEMTIME& stItemDate )
	{
		T* pT = static_cast<T*>(this);
		
		// set date-time in format (yyyymmddhhmmss)
		stdstr strFormatDate;
		strFormatDate.Format( _T( "%04d%02d%02d%02d%02d%02d" ), stItemDate.wYear, stItemDate.wMonth, stItemDate.wDay, stItemDate.wHour, stItemDate.wMinute, stItemDate.wSecond );
		
		return pT->SetItemText( nItem, nSubItem, strFormatDate.c_str() );
	}
	
	BOOL SetItemCheck( int nItem, int nSubItem, int nCheckValue )
	{
		T* pT = static_cast<T*>(this);
		
		switch ( pT->GetItemFormat( nItem, nSubItem ) )
		{
			case ITEM_FORMAT_CHECKBOX:			return pT->SetItemText( nItem, nSubItem, nCheckValue > 0 ? _T( "1" ) : _T( "0" ) );
			case ITEM_FORMAT_CHECKBOX_3STATE:	if ( nCheckValue < 0 )
													return pT->SetItemText( nItem, nSubItem, _T( "-1" ) );
												if ( nCheckValue > 0 )
													return pT->SetItemText( nItem, nSubItem, _T( "1" ) );
												return pT->SetItemText( nItem, nSubItem, _T( "0" ) );
		}
		
		return FALSE;
	}
		
	BOOL SetItemImage( int nItem, int nSubItem, int nImage )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemFormat( int nItem, int nSubItem, UINT nFormat, UINT nFlags = ITEM_FLAGS_NONE )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemFormat( int nItem, int nSubItem, UINT nFormat, UINT nFlags, CListArray < stdstr >& aComboList )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemFont( int nItem, int nSubItem, HFONT hFont )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL SetItemColours( int nItem, int nSubItem, COLORREF rgbBackground, COLORREF rgbText )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
		
	void ReverseItems()
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
	}
	
	void SortItems( int nColumn, BOOL bAscending )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
	}
	
	BOOL GetItemRect( int nItem, int nSubItem, CRect& rcItem )
	{
		T* pT = static_cast<T*>(this);
		
		int nTopItem = GetTopItem();		
		if ( nItem < nTopItem || nItem >= pT->GetItemCount() || nItem >= nTopItem + GetCountPerPage() )
			return FALSE;
		
		CRect rcClient;
		GetClientRect( rcClient );
		
		// calculate item rect based on scroll position
		rcItem = rcClient;
		rcItem.top = ( m_bShowHeader ? m_nHeaderHeight : 0 ) + ( ( nItem - nTopItem ) * m_nItemHeight );
		rcItem.bottom = rcItem.top + m_nItemHeight;
		rcItem.right = min( rcClient.right, GetTotalWidth() );
		
		if ( nSubItem != NULL_SUBITEM )
		{
			CRect rcColumn;
			if ( !GetColumnRect( nSubItem, rcColumn ) )
				return FALSE;

			rcItem.left = rcColumn.left;
			rcItem.right = rcColumn.right;
		}
		
		return TRUE;
	}
	
	BOOL GetItemRect( int nItem, CRect& rcItem )
	{
		return GetItemRect( nItem, NULL_SUBITEM, rcItem );
	}
	
	BOOL InvalidateItem( int nItem, int nSubItem = NULL_SUBITEM )
	{
		CRect rcItem;
		return GetItemRect( nItem, nSubItem, rcItem ) ? InvalidateRect( rcItem ) : FALSE;
	}

	BOOL InvalidateHeader()
	{
		if ( !m_bShowHeader )
			return TRUE;
		CRect rcHeader;
		if ( !GetClientRect( rcHeader ) )
			return FALSE;
		rcHeader.bottom = m_nHeaderHeight;
		return InvalidateRect( rcHeader );	
	}
	
	int GetTopItem()
	{
		return (int)( GetScrollPos( SB_VERT ) / m_nItemHeight );
	}
	
	int GetCountPerPage( BOOL bPartial = TRUE )
	{
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		// calculate number of items per control height (include partial item)
		div_t divHeight = div( rcClient.Height(), m_nItemHeight );
			
		// round up to nearest item count
		return max( bPartial && divHeight.rem > 0 ? divHeight.quot + 1 : divHeight.quot, 1 );
	}
	
	BOOL IsItemVisible( int nItem, int nSubItem = NULL_SUBITEM, BOOL bPartial = TRUE )
	{
		T* pT = static_cast<T*>(this);
		
		int nTopItem = GetTopItem();
		if ( nItem < nTopItem || nItem >= pT->GetItemCount() )
			return FALSE;
		
		// check whether item is visible
		if ( nItem < nTopItem || nItem >= nTopItem + GetCountPerPage( bPartial ) )
			return FALSE;
		
		// check whether subitem is visible
		if ( m_bFocusSubItem && nSubItem != NULL_SUBITEM )
		{
			CRect rcColumn;
			if ( !GetColumnRect( nSubItem, rcColumn ) )
				return FALSE;
			
			CRect rcClient;
			GetClientRect( rcClient );
			
			if ( rcColumn.left < rcClient.left || rcColumn.right > rcClient.right )
				return FALSE;
		}
		
		return TRUE;
	}
	
	BOOL EnsureItemVisible( int nItem, int nSubItem = NULL_SUBITEM )
	{
		if ( IsItemVisible( nItem, nSubItem, FALSE ) )
			return TRUE;
		
		HideTitleTip();
		
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		CRect rcItem;
		rcItem.top = ( m_bShowHeader ? m_nHeaderHeight : 0 ) + ( ( nItem - GetTopItem() ) * m_nItemHeight );
		rcItem.bottom = rcItem.top + m_nItemHeight;
		
		if ( rcItem.top < rcClient.top || rcItem.bottom > rcClient.bottom )
		{
			int nScrollItem = NULL_ITEM;
			
			// scroll list up/down to include item
			if ( rcItem.top < rcClient.top || rcItem.Height() > rcClient.Height() )
				nScrollItem = nItem;
			else if ( rcItem.bottom > rcClient.bottom )
				nScrollItem = nItem - ( GetCountPerPage( FALSE ) - 1 );
			
			if ( nScrollItem != NULL_ITEM )
				SetScrollPos( SB_VERT, nScrollItem * m_nItemHeight );
		}
		
		if ( m_bFocusSubItem && nSubItem != NULL_SUBITEM )
		{
			CRect rcColumn;
			if ( !GetColumnRect( nSubItem, rcColumn ) )
				return FALSE;
			
			GetClientRect( rcClient );

			int nScrollPos = 0;

			// scroll list left/right to include subitem
			if ( rcColumn.Width() > rcClient.Width() || rcColumn.left < 0 )
				nScrollPos = rcColumn.left;
			else if ( rcColumn.right > rcClient.right )
				nScrollPos = rcColumn.right - rcClient.right;

			if ( nScrollPos != 0 )
				SetScrollPos( SB_HORZ, GetScrollPos( SB_HORZ ) + nScrollPos );
		}
		
		return Invalidate();
	}
	
	void ShowScrollBar( int nScrollBar, BOOL bShow = TRUE )
	{
		switch ( nScrollBar )
		{
			case SB_HORZ:	m_bShowHorizScroll = bShow;
							break;
			case SB_VERT:	m_bShowVertScroll = bShow;
							break;
			case SB_BOTH:	m_bShowHorizScroll = bShow;
							m_bShowVertScroll = bShow;
							break;
		}
		
		ResetScrollBars();
		Invalidate();
	}
	
	void ResetScrollBars( int nScrollBar = SB_BOTH, int nScrollPos = -1, BOOL bRecalc = TRUE )
	{
		T* pT = static_cast<T*>(this);
		
		CRect rcClient;
		GetClientRect( rcClient );
		
		SCROLLINFO infoScroll;
		infoScroll.cbSize = sizeof( SCROLLINFO );
		infoScroll.fMask = nScrollPos < 0 ? SIF_PAGE | SIF_RANGE : SIF_PAGE | SIF_RANGE | SIF_POS;
		infoScroll.nPos = nScrollPos;
		infoScroll.nMin = 0;
		
		if ( ( nScrollBar == SB_BOTH || nScrollBar == SB_VERT ) && m_bShowVertScroll )
		{
			infoScroll.nMax = ( pT->GetItemCount() * m_nItemHeight ) + ( m_bShowHeader ? m_nHeaderHeight : 0 );
			infoScroll.nPage = rcClient.Height() - ( m_bShowHeader ? m_nHeaderHeight : 0 );
			
			// are we within client range?
			if ( (UINT)infoScroll.nMax <= infoScroll.nPage + ( m_bShowHeader ? m_nHeaderHeight : 0 ) )
				infoScroll.nMax = 0;
				    
			// set vertical scroll bar
			m_bEnableVertScroll = SetScrollInfo( SB_VERT, &infoScroll, TRUE ) ? ( infoScroll.nMax > 0 ) : FALSE;
		}
		
		if ( ( nScrollBar == SB_BOTH || nScrollBar == SB_HORZ ) && m_bShowHorizScroll )
		{
			infoScroll.nMax = GetTotalWidth( bRecalc );
			infoScroll.nPage = rcClient.Width();
			
			// are we within client range?
			if ( infoScroll.nPage >= (UINT)infoScroll.nMax )
				infoScroll.nMax = 0;
				
			// set horizontal scroll bar
			m_bEnableHorizScroll = SetScrollInfo( SB_HORZ, &infoScroll, TRUE ) ? ( infoScroll.nMax > (int)infoScroll.nPage ) : FALSE;
		}
	}

	BOOL IsScrollBarVisible( int nScrollBar )
	{
		switch ( nScrollBar )
		{
			case SB_HORZ:	return m_bEnableHorizScroll;
			case SB_VERT:	return m_bEnableVertScroll;
			case SB_BOTH:	return ( m_bEnableHorizScroll && m_bEnableVertScroll );
			default:		return FALSE;
		}
	}
	
	BOOL ResetSelected()
	{
		m_setSelectedItems.clear();
		m_nFocusItem = NULL_ITEM;
		m_nFocusSubItem = NULL_SUBITEM;
		m_nFirstSelected = NULL_ITEM;
		return Invalidate();
	}
	
	BOOL SelectItem( int nItem, int nSubItem = NULL_SUBITEM, UINT nFlags = 0 )
	{
		T* pT = static_cast<T*>(this);
		
		if ( nItem < 0 || nItem >= pT->GetItemCount() )
			return FALSE;
		
		BOOL bSelectItem = TRUE;
		BOOL bSelectRange = !m_bSingleSelect && ( nFlags & MK_SHIFT );
		BOOL bNewSelect = !( bSelectRange || ( nFlags & MK_CONTROL ) );
		BOOL bEnsureVisible = FALSE;

		// are we starting a new select sequence?
		if ( bNewSelect || bSelectRange )
		{
			// are we simply reselecting the same item?
			if ( m_setSelectedItems.size() == 1 && *m_setSelectedItems.begin() == nItem )
			{
				bSelectItem = FALSE;
				m_nFirstSelected = nItem;
				m_nFocusItem = nItem;
				m_nFocusSubItem = nSubItem;
			}
			else
				m_setSelectedItems.clear();
		}
		else // we adding to or removing from select sequence
		{
			if ( m_bSingleSelect )
				m_setSelectedItems.clear();
			
			set < int >::iterator posSelectedItem = m_setSelectedItems.find( nItem );
			
			// is this item already selected?
			if ( posSelectedItem != m_setSelectedItems.end() )
			{
				bSelectItem = FALSE;
				m_setSelectedItems.erase( posSelectedItem );				
				m_nFirstSelected = nItem;
				m_nFocusItem = nItem;
				m_nFocusSubItem = m_setSelectedItems.size() > 1 ? NULL_SUBITEM : nSubItem;
			}
		}
		
		// are we adding this item to the select sequence?
		if ( bSelectItem )
		{
			bEnsureVisible = TRUE;
			
			if ( bSelectRange )
			{
				if ( m_nFirstSelected == NULL_ITEM )
					m_nFirstSelected = nItem;
					
				for ( int nSelectedItem = min( m_nFirstSelected, nItem ); nSelectedItem <= max( m_nFirstSelected, nItem ); nSelectedItem++ )
					m_setSelectedItems.insert( nSelectedItem );
			}
			else
			{
				m_nFirstSelected = nItem;
				m_setSelectedItems.insert( nItem );
			}
			
			m_nFocusItem = nItem;
			m_nFocusSubItem = m_setSelectedItems.size() > 1 ? NULL_SUBITEM : nSubItem;
			
			// notify parent of selected item
			NotifyParent( m_nFocusItem, m_nFocusSubItem, LCN_SELECTED );
		}
		
		// start visible timer (scrolls list to partially hidden item)
		if ( !IsItemVisible( nItem, m_setSelectedItems.size() > 1 ? NULL_SUBITEM : nSubItem, FALSE ) )
			SetTimer( ITEM_VISIBLE_TIMER, ITEM_VISIBLE_PERIOD );
		else if ( m_nFocusItem != NULL_ITEM && m_nFocusSubItem != NULL_SUBITEM )
			EditItem( m_nFocusItem, m_nFocusSubItem );

		return Invalidate();
	}
	
	BOOL IsSelected( int nItem )
	{
		set < int >::iterator posSelectedItem = m_setSelectedItems.find( nItem );
		return ( posSelectedItem != m_setSelectedItems.end() );
	}
	
	BOOL GetSelectedItems( CListArray < int >& aSelectedItems )
	{
		aSelectedItems.RemoveAll();
		for ( set < int >::iterator posSelectedItem = m_setSelectedItems.begin(); posSelectedItem != m_setSelectedItems.end(); ++posSelectedItem )
			aSelectedItems.Add( *posSelectedItem );
		return !aSelectedItems.IsEmpty();
	}
	
	BOOL SetFocusItem( int nItem, int nSubItem = NULL_SUBITEM )
	{
		m_nFocusItem = nItem;
		m_nFocusSubItem = nSubItem;		
		return EnsureItemVisible( m_nFocusItem, m_nFocusSubItem );
	}

	BOOL GetFocusItem( int& nItem, int& nSubItem )
	{
		nItem = IsSelected( m_nFocusItem ) ? m_nFocusItem : ( m_setSelectedItems.empty() ? NULL_ITEM : *m_setSelectedItems.begin() );
		nSubItem = !m_bFocusSubItem || nItem == NULL_ITEM ? NULL_SUBITEM : m_nFocusSubItem;
		return ( nItem != NULL_ITEM );
	}
	
	int GetFocusItem()
	{
		return IsSelected( m_nFocusItem ) ? m_nFocusItem : ( m_setSelectedItems.empty() ? NULL_ITEM : *m_setSelectedItems.begin() );
	}
	
	BOOL HitTestHeader( CPoint point, int& nColumn, UINT& nFlags )
	{
		// reset hittest flags
		nFlags = HITTEST_FLAG_NONE;
		
		if ( !m_bShowHeader )
			return FALSE;
		
		CRect rcClient;
		if ( !GetClientRect( rcClient ) )
			return FALSE;
		
		// are we over the header?
		if ( point.y < rcClient.top || point.y > m_nHeaderHeight )
			return FALSE;
		
		int nDividerPos = 0;
		int nColumnCount = GetColumnCount();
	
		// get hit-test subitem
		for ( nColumn = 0; nColumn < nColumnCount; nColumn++ )
		{
			int nColumnWidth = GetColumnWidth( nColumn );
			nDividerPos += nColumnWidth;

			// offset divider position with current scroll position
			int nRelativePos = nDividerPos - GetScrollPos( SB_HORZ );

			// are we over the divider zone?
			if ( point.x >= nRelativePos - DRAG_HEADER_OFFSET - 1 && point.x <= nRelativePos + DRAG_HEADER_OFFSET )
			{
				nFlags |= HITTEST_FLAG_HEADER_DIVIDER;
				
				// are we to the left of the divider (or over last column divider)?
				if ( ( point.x >= nRelativePos - DRAG_HEADER_OFFSET - 1 && point.x < nRelativePos ) || nColumn + 1 >= nColumnCount - 1 )
				{
					nFlags |= HITTEST_FLAG_HEADER_LEFT;
					return TRUE;
				}

				// find last zero-length column after this column
				for ( int nNextColumn = nColumn + 1; nNextColumn < nColumnCount; nNextColumn++ )
				{
					if ( GetColumnWidth( nNextColumn ) > 0 )
						break;
					nColumn = nNextColumn;
				}
				
				nFlags |= HITTEST_FLAG_HEADER_RIGHT;

				return TRUE;
			}

			// are we over a column?
			if ( point.x > nRelativePos - nColumnWidth && point.x < nRelativePos )
				return TRUE;
		}	
		
		return FALSE;
	}
	
	BOOL HitTest( CPoint point, int& nItem, int& nSubItem )
	{
		T* pT = static_cast<T*>(this);
		
		// are we over the header?
		if ( point.y < ( m_bShowHeader ? m_nHeaderHeight : 0 ) )
			return FALSE;
		
		// calculate hit test item
		nItem = GetTopItem() + (int)( ( point.y - ( m_bShowHeader ? m_nHeaderHeight : 0 ) ) / m_nItemHeight );
		
		if ( nItem < 0 || nItem >= pT->GetItemCount() )
			return FALSE;
		
		int nTotalWidth = 0;
		int nColumnCount = GetColumnCount();
	
		// get hit-test subitem
		for ( nSubItem = 0; nSubItem < nColumnCount; nSubItem++ )
		{
			int nColumnWidth = GetColumnWidth( nSubItem );
			nTotalWidth += nColumnWidth;

			// offset position with current scroll position
			int nRelativePos = nTotalWidth - GetScrollPos( SB_HORZ );

			// are we over a subitem?
			if ( point.x > nRelativePos - nColumnWidth && point.x < nRelativePos )
				return TRUE;
		}
		
		return FALSE;
	}
	
	BOOL AutoSizeColumn( int nColumn )
	{
		T* pT = static_cast<T*>(this);
		
		CListColumn listColumn;
		if ( !GetColumn( nColumn, listColumn ) || listColumn.m_bFixed )
			return FALSE;
			
		CClientDC dcClient( m_hWnd );
		HFONT hOldFont = dcClient.SelectFont( m_fntListFont );
		
		// set to column text width if zero-length
		CSize sizeExtent;
		if ( !dcClient.GetTextExtent( listColumn.m_strText.c_str(), -1, &sizeExtent ) )
			return FALSE;
		
		int nMaxWidth = sizeExtent.cx + ITEM_WIDTH_MARGIN;
		
		CSize sizeIcon = 0;
		if ( !m_ilItemImages.IsNull() )
			m_ilItemImages.GetIconSize( sizeIcon );
		
		// calculate maximum column width required
		for ( int nItem = 0; nItem < pT->GetItemCount(); nItem++ )
		{
			if ( !dcClient.GetTextExtent( pT->GetItemText( nItem, listColumn.m_nIndex ), -1, &sizeExtent ) )
				return FALSE;
			
			if ( !m_ilItemImages.IsNull() && pT->GetItemImage( nItem, listColumn.m_nIndex ) != ITEM_IMAGE_NONE )
				sizeExtent.cx += sizeIcon.cx;
			
			nMaxWidth = max( nMaxWidth, (int)sizeExtent.cx + ITEM_WIDTH_MARGIN );
		}
		
		dcClient.SelectFont( hOldFont );
		
		return SetColumnWidth( nColumn, nMaxWidth );	
	}
	
	void ResizeColumn( BOOL bColumnScroll = FALSE )
	{
		HideTitleTip();
		
		int nCurrentPos = GET_X_LPARAM( GetMessagePos() );
		
		CRect rcClient;
		GetClientRect( rcClient );
		int nScrollLimit = GetTotalWidth() - rcClient.Width();
		
		if ( bColumnScroll )
		{
			// have we finished scrolling list to accommodate new column size?
			if ( !m_bColumnSizing || !m_bEnableHorizScroll || nCurrentPos - m_nStartScrollPos > 0 )
			{
				KillTimer( RESIZE_COLUMN_TIMER );
				
				// reset resize start point
				m_nStartPos = nCurrentPos;
				m_bResizeTimer = FALSE;
			}
			else if ( nCurrentPos < m_nStartPos && GetScrollPos( SB_HORZ ) >= nScrollLimit )
			{
				// reset start column size
				m_nStartSize = max( GetColumnWidth( m_nColumnSizing ) + ( nCurrentPos - m_nStartScrollPos ), 0 );
				
				// resize column
				SetColumnWidth( m_nColumnSizing, m_nStartSize );
			}
		}
		else
		{
			int nColumnSize = max( m_nStartSize + ( nCurrentPos - m_nStartPos ), 0 );
			
			// are we scrolled fully to the right and wanting to reduce the size of a column?
			if ( m_bEnableHorizScroll && GetScrollPos( SB_HORZ ) >= nScrollLimit && nColumnSize < GetColumnWidth( m_nColumnSizing ) )
			{
				if ( !m_bResizeTimer )
				{
					// only start the scroll timer once
					m_bResizeTimer = TRUE;

					// set new start scroll position
					m_nStartScrollPos = nCurrentPos;

					// start column resize / scroll timer
					SetTimer( RESIZE_COLUMN_TIMER, RESIZE_COLUMN_PERIOD );
				}
			}
			else
			{
				// resizing is done in scroll timer (if started)
				if ( !m_bResizeTimer )
					SetColumnWidth( m_nColumnSizing, nColumnSize );
			}
		}
	}
	
	void DragColumn()
	{
		HideTitleTip();
		
		CRect rcColumn;
		if ( !GetColumnRect( m_nHighlightColumn, rcColumn ) )
			return;
		
		CRect rcHeaderItem( rcColumn );
		rcHeaderItem.MoveToXY( 0, 0 );
		
		CListColumn listColumn;
		if ( !GetColumn( m_nHighlightColumn, listColumn ) )
			return;
		
		// store drag column
		m_nDragColumn = m_nHighlightColumn;
		
		CClientDC dcClient( m_hWnd );
		
		CDC dcHeader;
		dcHeader.CreateCompatibleDC( dcClient );
		
		int nContextState = dcHeader.SaveDC();
		
		// create drag header bitmap
		CBitmapHandle bmpHeader;
		bmpHeader.CreateCompatibleBitmap( dcClient, rcHeaderItem.Width(), rcHeaderItem.Height() );
		dcHeader.SelectBitmap( bmpHeader );
		
		dcHeader.SetBkColor( m_rgbHeaderBackground );
		dcHeader.ExtTextOut( rcHeaderItem.left, rcHeaderItem.top, ETO_OPAQUE, rcHeaderItem, _T( "" ), 0, NULL );
		dcHeader.Draw3dRect( rcHeaderItem, m_rgbHeaderBorder, m_rgbHeaderShadow );
		
		CRect rcHeaderText( rcHeaderItem );
		rcHeaderText.left += m_nHighlightColumn == 0 ? 4 : 3;
		rcHeaderText.OffsetRect( 0, 1 );
		
		// margin header text
		rcHeaderText.DeflateRect( 4, 0, 5, 0 );
		
		// has this header item an associated image?
		if ( listColumn.m_nImage != ITEM_IMAGE_NONE )
		{
			CSize sizeIcon;
			m_ilListItems.GetIconSize( sizeIcon );
			
			CRect rcHeaderImage;
			rcHeaderImage.left = listColumn.m_strText.empty() ? ( ( rcHeaderText.left + rcHeaderText.right ) / 2 ) - ( sizeIcon.cx / 2 ) - ( 0 ) : rcHeaderText.left;
			rcHeaderImage.right = min( rcHeaderImage.left + sizeIcon.cx, rcHeaderItem.right );
			rcHeaderImage.top = ( ( rcHeaderItem.top + rcHeaderItem.bottom ) / 2 ) - ( sizeIcon.cy / 2 );
			rcHeaderImage.bottom = min( rcHeaderImage.top + sizeIcon.cy, rcHeaderItem.bottom );
				
			m_ilListItems.DrawEx( listColumn.m_nImage, dcHeader, rcHeaderImage, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );

			// offset header text (for image)
			rcHeaderText.left += sizeIcon.cx + 4;
		}
		
		dcHeader.SelectFont( m_fntListFont );
		dcHeader.SetTextColor( m_rgbHeaderText );
		dcHeader.SetBkMode( TRANSPARENT );

		UINT nFormat = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;

		if ( listColumn.m_nFlags & ITEM_FLAGS_CENTRE )
			nFormat |= DT_CENTER;
		else if ( listColumn.m_nFlags & ITEM_FLAGS_RIGHT )
			nFormat |= DT_RIGHT;
		else
			nFormat |= DT_LEFT;
			
		// draw header text
		if ( !listColumn.m_strText.empty() )
			dcHeader.DrawText( listColumn.m_strText.c_str(), (int)listColumn.m_strText.length(), rcHeaderText, nFormat );

		dcHeader.RestoreDC( nContextState );
		
		SHDRAGIMAGE shDragImage;
		ZeroMemory( &shDragImage, sizeof( SHDRAGIMAGE ) );
		
		shDragImage.sizeDragImage.cx = rcHeaderItem.Width();
		shDragImage.sizeDragImage.cy = rcHeaderItem.Height();
		shDragImage.ptOffset.x = rcColumn.Width() / 2;
		shDragImage.ptOffset.y = rcColumn.Height() / 2;
		shDragImage.hbmpDragImage = bmpHeader;
		shDragImage.crColorKey = m_rgbBackground;
		
		// start header drag operation
		m_oleDragDrop.DoDragDrop( &shDragImage, DROPEFFECT_MOVE );
		
		// hide drop arrows after moving column
		m_wndDropArrows.Hide();
		
		if ( m_bButtonDown )
		{
			ReleaseCapture();
			m_bButtonDown = FALSE;
			m_bBeginSelect = FALSE;
			m_ptDownPoint = 0;
			m_ptSelectPoint = 0;
		}
		
		// finish moving a column
		if ( m_nHighlightColumn != NULL_COLUMN )
		{
			m_nHighlightColumn = NULL_COLUMN;
			InvalidateHeader();
		}
		
		m_nDragColumn = NULL_COLUMN;
	}
	
	BOOL DropColumn( CPoint point )
	{
		if ( !m_bShowHeader )
			return FALSE;
			
		m_nHotDivider = NULL_COLUMN;
		m_nHotColumn = NULL_COLUMN;
		UINT nHeaderFlags = HITTEST_FLAG_NONE;
		
		// are we over the header?
		if ( HitTestHeader( point, m_nHotColumn, nHeaderFlags ) )
		{
			CRect rcColumn;
			if ( !GetColumnRect( m_nHotColumn, rcColumn ) )
				return FALSE;
			m_nHotDivider = point.x < ( ( rcColumn.left + rcColumn.right ) / 2 ) ? m_nHotColumn : m_nHotColumn + 1;
			
			if ( m_nHotDivider == m_nDragColumn || m_nHotDivider == m_nDragColumn + 1 )
				m_nHotDivider = NULL_COLUMN;
		}
		
		if ( m_nHotDivider != NULL_COLUMN )
		{
			CRect rcHeader;
			GetClientRect( rcHeader );
			rcHeader.bottom = m_nHeaderHeight;
		
			CPoint ptDivider( 0, rcHeader.Height() / 2 );
			
			CRect rcColumn;
			int nColumnCount = GetColumnCount();
			
			// set closest divider position
			if ( GetColumnRect( m_nHotDivider < nColumnCount ? m_nHotDivider : nColumnCount - 1, rcColumn ) )
				ptDivider.x = m_nHotDivider < nColumnCount ? rcColumn.left : rcColumn.right;
			
			ClientToScreen( &ptDivider );
			
			// track drop window
			m_wndDropArrows.Show( ptDivider );
			return TRUE;
		}
		
		m_wndDropArrows.Hide();
		
		return FALSE;
	}
	
	BOOL SortColumn( int nColumn )
	{
		T* pT = static_cast<T*>(this);
		
		if ( !m_bShowHeader || !m_bShowSort )
			return FALSE;
			
		int nSortIndex = GetColumnIndex( nColumn );
		
		CWaitCursor curWait;
		
		if ( nSortIndex != m_nSortColumn )
		{
			// sort by new column
			m_bSortAscending = TRUE;
			m_nSortColumn = nSortIndex;
			pT->SortItems( m_nSortColumn, m_bSortAscending );
		}
		else
		{
			// toggle sort order if sorting same column
			m_bSortAscending = !m_bSortAscending;
			pT->ReverseItems();
		}
			
		return ResetSelected();
	}
	
	BOOL GetSortColumn( int& nColumn, BOOL& bAscending )
	{
		if ( !m_bShowHeader || !m_bShowSort || m_nSortColumn == NULL_COLUMN )
			return FALSE;
		nColumn = m_nSortColumn;
		bAscending = m_bSortAscending;
		return TRUE;
	}
	
	BOOL DragItem()
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
		return FALSE;
	}
	
	BOOL GroupSelect( CPoint point )
	{
		HideTitleTip();
		
		int nHorzScroll = GetScrollPos( SB_HORZ );
		int nVertScroll = GetScrollPos( SB_VERT );
		
		m_rcGroupSelect.left = min( m_ptSelectPoint.x, point.x + nHorzScroll );
		m_rcGroupSelect.right = max( m_ptSelectPoint.x, point.x + nHorzScroll );
		m_rcGroupSelect.top = min( m_ptSelectPoint.y, point.y + nVertScroll );
		m_rcGroupSelect.bottom = max( m_ptSelectPoint.y, point.y + nVertScroll );
		
		if ( m_rcGroupSelect.IsRectEmpty() )
			return FALSE;
		
		// select items in group
		AutoSelect( point );
		
		// start auto scroll timer
		SetTimer( ITEM_AUTOSCROLL_TIMER, ITEM_SCROLL_PERIOD );
		
		DWORD dwCurrentTick = GetTickCount();
		
		// timer messages are a low priority, therefore we need to simulate the timer when moving the mouse
		if ( ( dwCurrentTick - m_dwScrollTick ) > ITEM_SCROLL_PERIOD - 10 )
		{
			if ( AutoScroll( point ) )
				m_dwScrollTick = dwCurrentTick;
		}
		
		// redraw list immediately
		return RedrawWindow();
	}
	
	void AutoSelect( CPoint /*point*/ )
	{
		m_setSelectedItems.clear();
			
		if ( m_rcGroupSelect.left < GetTotalWidth() )
		{
			int nHorzScroll = GetScrollPos( SB_HORZ );
			int nVertScroll = GetScrollPos( SB_VERT );
			
			CRect rcGroupSelect( m_rcGroupSelect );
			rcGroupSelect.OffsetRect( -nHorzScroll, -nVertScroll );
		
			int nTopItem = GetTopItem();
			int nLastItem = nTopItem + ( ( rcGroupSelect.bottom - ( m_bShowHeader ? m_nHeaderHeight : 0 ) ) / m_nItemHeight );
			nTopItem += ( ( rcGroupSelect.top - ( m_bShowHeader ? m_nHeaderHeight : 0 ) ) / m_nItemHeight ) - ( ( rcGroupSelect.top < 0 ) ? 1 : 0 );
			
			for ( int nItem = nTopItem; nItem <= nLastItem; nItem++ )
			{
				if ( m_setSelectedItems.empty() )
					m_nFirstSelected = nItem;
				m_setSelectedItems.insert( nItem );
				
				m_nFocusItem = nItem;
				m_nFocusSubItem = NULL_SUBITEM;
			}
		}

		if ( m_setSelectedItems.empty() )
			m_nFirstSelected = NULL_ITEM;
	}
	
	BOOL AutoScroll( CPoint point )
	{
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		int nHorzScroll = GetScrollPos( SB_HORZ );
		int nVertScroll = GetScrollPos( SB_VERT );

		BOOL bAutoScroll = FALSE;
		
		if ( point.y < rcClient.top )
		{
			SendMessage( WM_VSCROLL, MAKEWPARAM( SB_LINEUP, 0 ) );
			int nAutoScroll = GetScrollPos( SB_VERT );
			if ( nVertScroll != nAutoScroll )
			{
				m_rcGroupSelect.top = rcClient.top + nAutoScroll - 1;
				m_rcGroupSelect.bottom = max( m_ptSelectPoint.y, point.y + nAutoScroll - 1 );
				bAutoScroll = TRUE;
			}
		}
		if ( point.y > rcClient.bottom )
		{
			SendMessage( WM_VSCROLL, MAKEWPARAM( SB_LINEDOWN, 0 ) );
			int nAutoScroll = GetScrollPos( SB_VERT );
			if ( nVertScroll != nAutoScroll )
			{
				m_rcGroupSelect.top = min( m_ptSelectPoint.y, point.y + nAutoScroll + 1 );
				m_rcGroupSelect.bottom = rcClient.bottom + nAutoScroll + 1;
				bAutoScroll = TRUE;
			}
		}
		if ( point.x < rcClient.left )
		{
			SendMessage( WM_HSCROLL, MAKEWPARAM( SB_LINELEFT, 0 ) );
			int nAutoScroll = GetScrollPos( SB_HORZ );
			if ( nHorzScroll != nAutoScroll )
			{
				m_rcGroupSelect.left = rcClient.left + nAutoScroll - 1;
				m_rcGroupSelect.right = max( m_ptSelectPoint.x, point.x + nAutoScroll - 1 );
				bAutoScroll = TRUE;
			}
		}
		if ( point.x > rcClient.right )
		{
			SendMessage( WM_HSCROLL, MAKEWPARAM( SB_LINERIGHT, 0 ) );
			int nAutoScroll = GetScrollPos( SB_HORZ );
			if ( nHorzScroll != nAutoScroll )
			{
				m_rcGroupSelect.left = min( m_ptSelectPoint.x, point.x + nAutoScroll + 1 );
				m_rcGroupSelect.right = rcClient.right + nAutoScroll + 1;
				bAutoScroll = TRUE;
			}
		}
		
		// was scrolling performed?
		return bAutoScroll;
	}
	
	BOOL BeginScroll( int nBeginItem, int nEndItem )
	{
		T* pT = static_cast<T*>(this);
		
		// any scroll required?
		if ( nBeginItem == nEndItem )
			return FALSE;
		
		// calculate scroll offset
		m_nScrollOffset = abs( nEndItem - nBeginItem ) * m_nItemHeight;
		m_nScrollUnit = min( max( m_nScrollOffset / m_nItemHeight, ITEM_SCROLL_UNIT_MIN ), ITEM_SCROLL_UNIT_MAX );
		m_nScrollDelta = ( m_nScrollOffset - m_nScrollUnit ) / m_nScrollUnit;
		m_bScrollDown = ( nBeginItem < nEndItem );
		
		CClientDC dcClient( m_hWnd );
		
		CDC dcScrollList;
		dcScrollList.CreateCompatibleDC( dcClient );
		
		int nContextState = dcScrollList.SaveDC();
		
		CRect rcScrollList;
		GetClientRect( rcScrollList );
		rcScrollList.bottom = ( GetCountPerPage() + abs( nEndItem - nBeginItem ) ) * m_nItemHeight;
		
		if ( !m_bmpScrollList.IsNull() )
			m_bmpScrollList.DeleteObject();
		m_bmpScrollList.CreateCompatibleBitmap( dcClient, rcScrollList.Width(), rcScrollList.Height() ); 
		dcScrollList.SelectBitmap( m_bmpScrollList );
		
		pT->DrawBkgnd( dcScrollList.m_hDC );
		
		CRect rcItem;
		rcItem.left = -GetScrollPos( SB_HORZ );
		rcItem.right = GetTotalWidth();
		rcItem.top = 0;
		rcItem.bottom = rcItem.top;
		
		// draw all visible items into bitmap
		for ( int nItem = min( nBeginItem, nEndItem ); nItem < pT->GetItemCount(); rcItem.top = rcItem.bottom, nItem++ )
		{
			rcItem.bottom = rcItem.top + m_nItemHeight;
			
			if ( rcItem.top > rcScrollList.bottom )
				break;
			
			// may be implemented in a derived class
			pT->DrawItem( dcScrollList.m_hDC, nItem, rcItem );
		}
		
		dcScrollList.RestoreDC( nContextState );
		
		ScrollList();
		
		// start scrolling timer
		SetTimer( ITEM_SCROLL_TIMER, ITEM_SCROLL_PERIOD );
		
		return TRUE;
	}
	
	BOOL EndScroll()
	{
		KillTimer( ITEM_SCROLL_TIMER );
		if ( !m_bmpScrollList.IsNull() )
			m_bmpScrollList.DeleteObject();
		return Invalidate();
	}
	
	BOOL ScrollList()
	{
		if ( m_nScrollOffset <= m_nScrollUnit )
			m_nScrollOffset--;
		else
		{
			m_nScrollOffset -= m_nScrollDelta;
			if ( m_nScrollOffset < m_nScrollDelta )
				m_nScrollOffset = m_nScrollUnit;
		}
		
		if ( m_bmpScrollList.IsNull() || m_nScrollOffset < 0 )
			return FALSE;
		
		CClientDC dcClient( m_hWnd );
	
		CDC dcScrollList;
		dcScrollList.CreateCompatibleDC( dcClient );
		
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		HBITMAP hOldBitmap = dcScrollList.SelectBitmap( m_bmpScrollList );
		
		CSize sizScrollBitmap;
		m_bmpScrollList.GetSize( sizScrollBitmap );
		
		// draw scrolled list
		dcClient.BitBlt( 0, rcClient.top, rcClient.Width(), rcClient.Height(), dcScrollList, 0, m_bScrollDown ? ( sizScrollBitmap.cy - ( GetCountPerPage() * m_nItemHeight ) - m_nScrollOffset ) : m_nScrollOffset, SRCCOPY );

		dcScrollList.SelectBitmap( hOldBitmap );
		
		return TRUE;
	}
	
	BOOL EditItem( int nItem, int nSubItem = NULL_SUBITEM )
	{
		T* pT = static_cast<T*>(this);
		
		if ( !EnsureItemVisible( nItem, nSubItem ) )
			return FALSE;
		
		if ( GetFocus() != m_hWnd )
			return FALSE;
		
		CRect rcSubItem;
		if ( !GetItemRect( nItem, nSubItem, rcSubItem ) )
			return FALSE;
		
		int nIndex = GetColumnIndex( nSubItem );
		if ( pT->GetItemFlags( nItem, nIndex ) & ITEM_FLAGS_READ_ONLY )
			return TRUE;
		
		switch ( pT->GetItemFormat( nItem, nIndex ) )
		{
			case ITEM_FORMAT_EDIT:				m_bEditItem = TRUE;
												if ( !RedrawWindow() )
													return FALSE;
												if ( !m_wndItemEdit.Create( m_hWnd, nItem, nSubItem, rcSubItem, pT->GetItemFlags( nItem, nIndex ), pT->GetItemText( nItem, nIndex ), pT->GetItemMaxEditLen(nItem, nIndex ) ) )
													return FALSE;
												m_wndItemEdit.SetFont(GetItemFont(nItem, nIndex));
												break;
			case ITEM_FORMAT_DATETIME:			{
													m_bEditItem = TRUE;
													if ( !RedrawWindow() )
														return FALSE;
													SYSTEMTIME stItemDate;
													GetItemDate( nItem, nIndex, stItemDate );
													if ( !m_wndItemDate.Create( m_hWnd, nItem, nSubItem, rcSubItem, pT->GetItemFlags( nItem, nIndex ), stItemDate ) )
														return FALSE;
												}
												break;
			case ITEM_FORMAT_COMBO:				{
													m_bEditItem = TRUE;
													if ( !RedrawWindow() )
														return FALSE;
													CListArray < stdstr > aComboList;
													if ( !pT->GetItemComboList( nItem, nIndex, aComboList ) )
														return FALSE;
													if ( !m_wndItemCombo.Create( m_hWnd, nItem, nSubItem, rcSubItem, pT->GetItemFlags( nItem, nIndex ), pT->GetItemText( nItem, nIndex ), aComboList ) )
														return FALSE;
												}
												break;
		}
		return TRUE;
	}
	
	stdstr FormatDate( SYSTEMTIME& stFormatDate )
	{
		if ( stFormatDate.wYear == 0 )
			return _T( "" );
		
		// format date to local format
		TCHAR szDateFormat[ DATE_STRING ];
		return GetDateFormat( LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stFormatDate, NULL, szDateFormat, DATE_STRING ) == 0 ? _T( "" ) : szDateFormat;
	}
	
	stdstr FormatTime( SYSTEMTIME& stFormatDate )
	{
		SYSTEMTIME stFormatTime = stFormatDate;
		stFormatTime.wYear = 0;
		stFormatTime.wMonth = 0;
		stFormatTime.wDay = 0;
		
		// format time to local format
		TCHAR szTimeFormat[ DATE_STRING ];
		return GetTimeFormat( LOCALE_USER_DEFAULT, 0, &stFormatTime, NULL, szTimeFormat, DATE_STRING ) == 0 ? _T( "" ) : szTimeFormat;
	}
	
	void NotifyParent( int nItem, int nSubItem, int nMessage )
	{
		T* pT = static_cast<T*>(this);
		
		CListNotify listNotify;
		listNotify.m_hdrNotify.hwndFrom = pT->m_hWnd;
		listNotify.m_hdrNotify.idFrom = pT->GetDlgCtrlID();
		listNotify.m_hdrNotify.code = nMessage;
		listNotify.m_nItem = nItem;
		listNotify.m_nSubItem = GetColumnIndex( nSubItem );
		listNotify.m_nExitChar = 0;
		listNotify.m_lpszItemText = NULL;
		listNotify.m_lpItemDate = NULL;

		// forward notification to parent
		FORWARD_WM_NOTIFY( pT->GetParent(), listNotify.m_hdrNotify.idFrom, &listNotify.m_hdrNotify, ::SendMessage );
	}
	
	BOOL ShowTitleTip( CPoint point, int nItem, int nSubItem )
	{
		T* pT = static_cast<T*>(this);
		
		// do not show titletip if editing
		if ( m_bEditItem )
			return FALSE;
		
		// is titletip already shown for this item?
		if ( nItem == m_nTitleTipItem && nSubItem == m_nTitleTipSubItem )
			return FALSE;
		
		CRect rcSubItem;
		if ( !GetItemRect( nItem, nSubItem, rcSubItem ) )
		{
			HideTitleTip();
			return FALSE;
		}
		
		int nIndex = GetColumnIndex( nSubItem );
		CRect rcItemText( rcSubItem );
				
		// margin item text
//		rcItemText.left += nSubItem == 0 ? 4 : 3;
//		rcItemText.DeflateRect( 4, 0 );
		
		// offset item text (for image)
		if ( !m_ilItemImages.IsNull() && pT->GetItemImage( nItem, nIndex ) != ITEM_IMAGE_NONE )
		{
			CSize sizeIcon;
			m_ilItemImages.GetIconSize( sizeIcon );
			rcItemText.left += sizeIcon.cx + 4;
		}
				
		// is current cursor position over item text (not over an icon)?
		if ( !rcItemText.PtInRect( point ) )
			return FALSE;
		
		stdstr strItemText;
		
		switch ( pT->GetItemFormat( nItem, nIndex ) )
		{
			case ITEM_FORMAT_CHECKBOX:
			case ITEM_FORMAT_CHECKBOX_3STATE:	
			case ITEM_FORMAT_PROGRESS:			break; // no titletip for checkboxes or progress
			case ITEM_FORMAT_DATETIME:			{
													SYSTEMTIME stItemDate;
													if ( !GetItemDate( nItem, nIndex, stItemDate ) )
														break;
													
													UINT nItemFlags = pT->GetItemFlags( nItem, nIndex );
													if ( nItemFlags & ITEM_FLAGS_DATE_ONLY )
														strItemText = FormatDate( stItemDate );
													else if ( nItemFlags & ITEM_FLAGS_TIME_ONLY )
														strItemText = FormatTime( stItemDate );
													else
														strItemText = FormatDate( stItemDate ) + _T( " " ) + FormatTime( stItemDate );
												}
												break;
			default:							strItemText = pT->GetItemText( nItem, nIndex );
												break;
		}

		if ( strItemText.empty() )
		{
			HideTitleTip();
			return FALSE;
		}
		
		ClientToScreen( rcItemText );
		if ( !m_wndTitleTip.Show( rcItemText, strItemText.c_str(), pT->GetItemToolTip( nItem, nIndex ).c_str() ) )
		{
			HideTitleTip();
			return FALSE;
		}
		
		m_nTitleTipItem = nItem;
		m_nTitleTipSubItem = nSubItem;
						
		return TRUE;
	}
	
	BOOL HideTitleTip( BOOL bResetItem = TRUE )
	{
		if ( bResetItem )
		{
			m_nTitleTipItem = NULL_ITEM;
			m_nTitleTipSubItem = NULL_SUBITEM;
		}
		return m_wndTitleTip.Hide();
	}
	
	BEGIN_MSG_MAP_EX(CListImpl)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_SETFOCUS(OnSetFocus)
		MSG_WM_KILLFOCUS(OnKillFocus)
		MSG_WM_GETDLGCODE(OnGetDlgCode)
		MSG_WM_SIZE(OnSize)
		MSG_WM_HSCROLL(OnHScroll)
		MSG_WM_VSCROLL(OnVScroll)
		MSG_WM_CANCELMODE(OnCancelMode)
		MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST,WM_MOUSELAST,OnMouseRange)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_LBUTTONUP(OnLButtonUp)
		MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
		MSG_WM_RBUTTONDOWN(OnRButtonDown)
		MSG_WM_RBUTTONUP(OnRButtonUp)
		MSG_WM_MOUSEMOVE(OnMouseMove)
#if(_WIN32_WINNT >= 0x0400)
		MSG_WM_MOUSELEAVE(OnMouseLeave)
		MSG_WM_MOUSEWHEEL(OnMouseWheel)
#endif
		MSG_WM_TIMER(OnTimer)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_SYSKEYDOWN(OnSysKeyDown)
		MSG_WM_SETTINGCHANGE(OnSettingsChange)
		MSG_WM_SYSCOLORCHANGE(OnSettingsChange)
		MSG_WM_FONTCHANGE(OnSettingsChange)
		//MSG_WM_THEMECHANGED(OnSettingsChange)
		NOTIFY_CODE_HANDLER_EX(LCN_ENDEDIT,OnEndEdit)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorListBox)
		CHAIN_MSG_MAP(CDoubleBufferImpl< CListImpl >)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()
	
	int OnCreate( LPCREATESTRUCT /*lpCreateStruct*/ )
	{
		T* pT = static_cast<T*>(this);
		return pT->Initialise() ? 0 : -1;
	}
	
	void OnDestroy()
	{
		m_oleDragDrop.Revoke();
		
		if ( m_wndDropArrows.IsWindow() )
			m_wndDropArrows.DestroyWindow();
		
		if ( m_wndTitleTip.IsWindow() )
			m_wndTitleTip.DestroyWindow();
		
		if ( m_wndItemEdit.IsWindow() )
			m_wndItemEdit.DestroyWindow();
		
		if ( m_wndItemCombo.IsWindow() )
			m_wndItemCombo.DestroyWindow();
		
		if ( m_wndItemDate.IsWindow() )
			m_wndItemDate.DestroyWindow();
		
		if ( m_ttToolTip.IsWindow() )
			m_ttToolTip.DestroyWindow();
	}
	
	void OnSetFocus( HWND /*hOldWnd*/ )
	{
		Invalidate();
	}
	
	void OnKillFocus( HWND /*hNewWnd*/ )
	{
		Invalidate();
	}
	
	UINT OnGetDlgCode( LPMSG /*lpMessage*/ )
	{
		return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
	}
	
	void OnSize( UINT /*nType*/, CSize /*size*/ )
	{
		// stop any pending scroll
		EndScroll();
		
		// end any pending edit
		if ( m_bEditItem )
			SetFocus();
			
		ResetScrollBars( SB_BOTH, -1, FALSE );
		Invalidate();
	}
	
	void OnHScroll( int nSBCode, short /*nPos*/, HWND /*hScrollBar*/ )
	{
		// stop any pending scroll
		EndScroll();
		
		// end any pending edit
		if ( m_bEditItem )
			SetFocus();
		
		HideTitleTip();
			
		CRect rcClient;
		GetClientRect( rcClient );
		
		int nScrollPos = GetScrollPos( SB_HORZ );

		switch ( nSBCode )
		{
			case SB_LEFT:			nScrollPos = 0;
									break;
			case SB_LINELEFT:		nScrollPos = max( nScrollPos - ITEM_SCROLL_OFFSET, 0 );
									break;
			case SB_PAGELEFT:		nScrollPos = max( nScrollPos - rcClient.Width(), 0 );
									break;
			case SB_RIGHT:			nScrollPos = rcClient.Width();
									break;
			case SB_LINERIGHT:		nScrollPos = min( nScrollPos + ITEM_SCROLL_OFFSET, GetTotalWidth() );
									break;
			case SB_PAGERIGHT:		nScrollPos = min( nScrollPos + rcClient.Width(), GetTotalWidth() );
									break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:		{
										SCROLLINFO infoScroll;
										ZeroMemory( &infoScroll, sizeof( SCROLLINFO ) );
										infoScroll.cbSize = sizeof( SCROLLINFO );
										infoScroll.fMask = SIF_TRACKPOS;
										
										// get 32-bit scroll position
										if ( !GetScrollInfo( SB_HORZ, &infoScroll ) )
											return;
										
										// has scroll position changed?
										if ( infoScroll.nTrackPos == nScrollPos )
											return;
										
										nScrollPos = infoScroll.nTrackPos;
									}
									break;
			default:				return;
		}

		ResetScrollBars( SB_HORZ, nScrollPos, FALSE );	
		Invalidate();
	}

	void OnVScroll( int nSBCode, short /*nPos*/, HWND /*hScrollBar*/ )
	{
		T* pT = static_cast<T*>(this);
		
		// end any pending edit
		if ( m_bEditItem )
			SetFocus();
		
		HideTitleTip();

		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		int nScrollPos = GetScrollPos( SB_VERT );
		BOOL bScrollList = m_bSmoothScroll;
		
		switch ( nSBCode )
		{
			case SB_TOP:			nScrollPos = 0;
									bScrollList = FALSE;
									break;
			case SB_LINEUP:			nScrollPos = max( nScrollPos - m_nItemHeight, 0 );
									break;
			case SB_PAGEUP:			nScrollPos = max( nScrollPos - rcClient.Height(), 0 );
									break;
			case SB_BOTTOM:			nScrollPos = pT->GetItemCount() * m_nItemHeight;
									bScrollList = FALSE;
									break;
			case SB_LINEDOWN:		nScrollPos += m_nItemHeight;
									break;
			case SB_PAGEDOWN:		nScrollPos += rcClient.Height();
									break;
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION:	{
										SCROLLINFO infoScroll;
										ZeroMemory( &infoScroll, sizeof( SCROLLINFO ) );
										infoScroll.cbSize = sizeof( SCROLLINFO );
										infoScroll.fMask = SIF_TRACKPOS;
										
										// get 32-bit scroll position
										if ( !GetScrollInfo( SB_VERT, &infoScroll ) )
											return;
										
										// has scroll position changed?
										if ( infoScroll.nTrackPos == nScrollPos )
											return;
										
										nScrollPos = infoScroll.nTrackPos;
										bScrollList = FALSE;
									}
									break;
			case SB_ENDSCROLL:		m_bScrolling = FALSE;
			default:				return;
		}
		
		// store original top item before scrolling
		int nTopItem = GetTopItem();
		ResetScrollBars( SB_VERT, nScrollPos, FALSE );
		
		if ( bScrollList && !m_bScrolling )
			m_bScrolling = BeginScroll( nTopItem, GetTopItem() );
		else
			EndScroll();
	}
	
	void OnCancelMode() 
	{
		if ( m_bButtonDown )
			ReleaseCapture();
	
		HideTitleTip();
		m_wndDropArrows.Hide();
		m_nDragColumn = NULL_COLUMN;
		m_nHighlightColumn = NULL_COLUMN;
	}
	
	LRESULT OnMouseRange( UINT nMessage, WPARAM wParam, LPARAM lParam )
	{
		if ( m_ttToolTip.IsWindow() )
		{
			MSG msgRelay = { m_hWnd, nMessage, wParam, lParam };
			m_ttToolTip.RelayEvent( &msgRelay );
		}
		SetMsgHandled( FALSE );
		return 0;
	}
	
	void OnLButtonDown( UINT nFlags, CPoint point )
	{
		T* pT = static_cast<T*>(this);
				
		// We have a bug here with setcapture() and the tooltip notiying the parent with mouse messages.
		// Hard to explain, but what I think happens is that this click is sent by the tooltip, which then
		// releases capture, so it gets no more mouse events, thus not receiving the actual double click
		// on the tool tip, and what results is two single clicks for this parent control.
		// Not sure how to fix - Rowan
		
		// Explorer doesn't actually hide the tip when clicked on, so we can remove this code and it shouldn't really matter
		//HideTitleTip(FALSE);
					
		m_bButtonDown = TRUE;
		m_ptDownPoint = point;
		m_ptSelectPoint = CPoint( point.x + GetScrollPos( SB_HORZ ), point.y + GetScrollPos( SB_VERT ) );

		// stop any pending scroll
		EndScroll();
		
		SetFocus();
		
		// capture all mouse input
		SetCapture();
		
		int nColumn = NULL_COLUMN;
		UINT nHeaderFlags = HITTEST_FLAG_NONE;
				
		// are we over the header?
		if ( HitTestHeader( point, nColumn, nHeaderFlags ) )
		{
			CListColumn listColumn;
			if ( !GetColumn( nColumn, listColumn ) )
				return;
				
			if ( !listColumn.m_bFixed && ( nHeaderFlags & HITTEST_FLAG_HEADER_DIVIDER ) )
			{
				SetCursor( m_curDivider );
			
				// begin column resizing
				m_bColumnSizing = TRUE;
				m_nColumnSizing = nColumn;
				m_nStartSize = listColumn.m_nWidth;
				m_nStartPos = GET_X_LPARAM( GetMessagePos() );
			}
			else if (m_bSortEnabled) // Added by Rowan 05/12/2006
			{
				m_nHighlightColumn = nColumn;
				InvalidateHeader();
			}
			
			return;			
		}
		
		int nItem = NULL_ITEM;
		int nSubItem = NULL_SUBITEM;
		
		if ( !HitTest( point, nItem, nSubItem ) )
		{
			m_nFirstSelected = NULL_ITEM;
			m_bBeginSelect = TRUE;		
		}
		else
		{
			// do not begin group select from first columns
			if ( !( nFlags & MK_SHIFT ) && !( nFlags & MK_CONTROL ) && nSubItem != 0 )
			{
				m_bBeginSelect = TRUE;
				m_nFirstSelected = nItem;
			}
			
			// only select item if not already selected
			if ( ( nFlags & MK_SHIFT ) || ( nFlags & MK_CONTROL ) || !IsSelected( nItem ) || m_setSelectedItems.size() <= 1 )
				SelectItem( nItem, nSubItem, nFlags );
			
			int nIndex = GetColumnIndex( nSubItem );
			if ( !( pT->GetItemFlags( nItem, nIndex ) & ITEM_FLAGS_READ_ONLY ) )
			{
				switch ( pT->GetItemFormat( nItem, nIndex ) )
				{
					case ITEM_FORMAT_CHECKBOX:			m_bBeginSelect = FALSE;
														pT->SetItemText( nItem, nIndex, _ttoi( pT->GetItemText( nItem, nIndex ) ) > 0 ? _T( "0" ) : _T( "1" ) );
														NotifyParent( nItem, nSubItem, LCN_MODIFIED );
														InvalidateItem( nItem );
														break;
					case ITEM_FORMAT_CHECKBOX_3STATE:	{
															m_bBeginSelect = FALSE;
															
															int nCheckImage = _ttoi( pT->GetItemText( nItem, nIndex ) );
															if ( nCheckImage < 0 )
																pT->SetItemText( nItem, nIndex, _T( "0" ) );
															else if ( nCheckImage > 0 )
																pT->SetItemText( nItem, nIndex, _T( "-1" ) );
															else
																pT->SetItemText( nItem, nIndex, _T( "1" ) );
															
															NotifyParent( nItem, nSubItem, LCN_MODIFIED );
															InvalidateItem( nItem );
														}
														break;
					case ITEM_FORMAT_HYPERLINK:			m_bBeginSelect = FALSE;
														SetCursor( m_curHyperLink );
														NotifyParent( nItem, nSubItem, LCN_HYPERLINK );
														break;
				}

				if (( pT->GetItemFlags( nItem, nIndex ) & ITEM_FLAGS_HIT_NOTIFY ) != 0)
				{
					NotifyParent( nItem, nSubItem, LCN_HITTEST );	
				}
			}
		}
	}
	
	void OnLButtonUp( UINT nFlags, CPoint point )
	{
		if ( m_bButtonDown )
			ReleaseCapture();
		
		// finish resizing or selecting a column
		if ( m_bColumnSizing || m_nHighlightColumn != NULL_COLUMN )
		{
			// are we changing the sort order?
			if ( !m_bColumnSizing && m_nHighlightColumn != NULL_COLUMN && m_bSortEnabled) // Changed by Rowan 05/12/2006
			//if ( !m_bColumnSizing && m_nHighlightColumn != NULL_COLUMN)
				SortColumn( m_nHighlightColumn );
			
			m_bColumnSizing = FALSE;
			m_nColumnSizing = NULL_COLUMN;
			m_nHighlightColumn = NULL_COLUMN;
			m_nStartSize = 0;
			m_nStartPos = 0;

			InvalidateHeader();
		}
		
		m_bBeginSelect = FALSE;
		m_bButtonDown = FALSE;
		m_ptDownPoint = 0;
		m_ptSelectPoint = 0;
		
		// have we finished a group select?
		if ( m_bGroupSelect )
		{
			m_bGroupSelect = FALSE;
			Invalidate();
		}
		else
		{
			int nItem = NULL_ITEM;
			int nSubItem = NULL_SUBITEM;
			
			// de-select item if current item is selected
			if ( HitTest( point, nItem, nSubItem ) && IsSelected( nItem ) && m_setSelectedItems.size() > 1 && !( nFlags & MK_SHIFT ) && !( nFlags & MK_CONTROL ) )
				SelectItem( nItem, nSubItem, nFlags );
				
			// notify parent of left-click item
			NotifyParent( nItem, nSubItem, LCN_LEFTCLICK );
		}
	}
	
	void OnLButtonDblClk( UINT /*nFlags*/, CPoint point ) 
	{
		
		HideTitleTip( FALSE );
		
		// handle double-clicks (for drawing)
		SendMessage( WM_LBUTTONDOWN, 0, MAKELPARAM( point.x, point.y ) );

		int nColumn = NULL_COLUMN;
		UINT nHeaderFlags = HITTEST_FLAG_NONE;
		
		// resize column if double-click on a divider
		if ( HitTestHeader( point, nColumn, nHeaderFlags ) && ( nHeaderFlags & HITTEST_FLAG_HEADER_DIVIDER ) )
			AutoSizeColumn( nColumn );
		
		int nItem = NULL_ITEM;
		int nSubItem = NULL_SUBITEM;
		
		HitTest( point, nItem, nSubItem );
		
		//WriteTraceF(TraceInfo, "List Ctrl Double Click, Item: %d", nItem);
		
		// notify parent of double-clicked item
		NotifyParent( nItem, nSubItem, LCN_DBLCLICK );
	}
	
	void OnRButtonDown( UINT nFlags, CPoint point ) 
	{
		// stop any pending scroll
		EndScroll();
		
		SetFocus();
		
		HideTitleTip( FALSE );
		
		int nItem = NULL_ITEM;
		int nSubItem = NULL_SUBITEM;
		
		if (m_bRightClickSelect)
		{
			// only select item if not already selected (de-select in OnLButtonUp)
			if (HitTest(point, nItem, nSubItem) && !IsSelected(nItem))
				SelectItem(nItem, nSubItem, nFlags);
		}
	}

	void OnRButtonUp( UINT /*nFlags*/, CPoint point )
	{
		int nItem = NULL_ITEM;
		int nSubItem = NULL_SUBITEM;
		
		if ( !HitTest( point, nItem, nSubItem ) )
			ResetSelected();
		
		// notify parent of right-click item
		NotifyParent( nItem, nSubItem, LCN_RIGHTCLICK );
	}

	void OnMouseMove( UINT nFlags, CPoint point )
	{
		T* pT = static_cast<T*>(this);
		
		if ( !( nFlags & MK_LBUTTON ) )
		{
			if ( m_bButtonDown )
				ReleaseCapture();
			
			m_bButtonDown = FALSE;
		}
		
		if ( !m_bMouseOver )
		{
			m_bMouseOver = TRUE;
			
			TRACKMOUSEEVENT trkMouse;
			trkMouse.cbSize = sizeof( TRACKMOUSEEVENT );
			trkMouse.dwFlags = TME_LEAVE;
			trkMouse.hwndTrack = m_hWnd;
			
			// notify when the mouse leaves button
			_TrackMouseEvent( &trkMouse );
		}
		
		if ( m_bButtonDown )
		{
			// are we resizing a column?
			if ( m_bColumnSizing )
			{
				ResizeColumn();
				return;
			}
			
			// are we beginning to drag a column? 
			if ( m_nHighlightColumn != NULL_COLUMN && ( point.x < m_ptDownPoint.x - DRAG_HEADER_OFFSET || point.x > m_ptDownPoint.x + DRAG_HEADER_OFFSET || point.y < m_ptDownPoint.y - DRAG_HEADER_OFFSET || point.y > m_ptDownPoint.y + DRAG_HEADER_OFFSET ) )
			{
				DragColumn();
				return;
			}
			
			// are we beginning a group select or dragging an item?
			if ( point.x < m_ptDownPoint.x - DRAG_ITEM_OFFSET || point.x > m_ptDownPoint.x + DRAG_ITEM_OFFSET || point.y < m_ptDownPoint.y - DRAG_ITEM_OFFSET || point.y > m_ptDownPoint.y + DRAG_ITEM_OFFSET )
			{
				if ( m_bBeginSelect || !m_bDragDrop )
					m_bGroupSelect = ( !m_bSingleSelect && !m_bEditItem );
				else
				{
					int nItem = NULL_ITEM;
					int nSubItem = NULL_SUBITEM;
					
					if ( HitTest( point, nItem, nSubItem ) )
					{
						// select the drag item (if not already selected)
						if ( !IsSelected( nItem ) )
							SelectItem( nItem, nSubItem, nFlags );
						
						// begin drag item operation
						pT->DragItem();
					}
				}
			}							
			
			if ( m_bGroupSelect )
			{
				GroupSelect( point );
				return;
			}
		}
		else
		{
			int nColumn = NULL_COLUMN;
			UINT nHeaderFlags = HITTEST_FLAG_NONE;
			
			// are we over the header?
			BOOL bHitTestHeader = HitTestHeader( point, nColumn, nHeaderFlags );
						
			if ( bHitTestHeader )
			{
				HideTitleTip();
				CListColumn listColumn;
				if ( GetColumn( nColumn, listColumn ) && !listColumn.m_bFixed && ( nHeaderFlags & HITTEST_FLAG_HEADER_DIVIDER ) )
					SetCursor( m_curDivider );
				else
				{
					// get tooltip for this item
					stdstr strToolTip = pT->GetHeaderToolTip(nColumn);
					if(!strToolTip.empty())
					{
						CRect rcColumn;
						if(!GetColumnRect(nColumn, rcColumn))
							return;
						rcColumn.bottom = m_nHeaderHeight;
						m_ttToolTip.Activate( TRUE );
						m_ttToolTip.AddTool( m_hWnd, (LPCTSTR)strToolTip.c_str(), rcColumn,TOOLTIP_TOOL_ID);						
					}
				}
				return;
			}
			
			int nItem = NULL_ITEM;
			int nSubItem = NULL_SUBITEM;
			
			if ( !HitTest( point, nItem, nSubItem ) )
			{
				if ( m_nHotItem != NULL_ITEM && m_nHotSubItem != NULL_SUBITEM )
				{
					// redraw old hot item
					int nIndex = GetColumnIndex( m_nHotSubItem );
					if ( pT->GetItemFormat( m_nHotItem, nIndex ) == ITEM_FORMAT_HYPERLINK && !( pT->GetItemFlags( m_nHotItem, nIndex ) & ITEM_FLAGS_READ_ONLY ) )
						InvalidateItem( m_nHotItem, m_nHotSubItem );
				}
				
				m_ttToolTip.Activate( FALSE );
				m_ttToolTip.DelTool( m_hWnd, TOOLTIP_TOOL_ID );
					
				m_nHotItem = NULL_ITEM;
				m_nHotSubItem = NULL_SUBITEM;
				HideTitleTip();
			}
			else
			{
				// has the hot item changed?
				if ( nItem != m_nHotItem || nSubItem != m_nHotSubItem )
				{
					// redraw old hot item
					int nIndex = GetColumnIndex( m_nHotSubItem );
					if ( pT->GetItemFormat( m_nHotItem, nIndex ) == ITEM_FORMAT_HYPERLINK && !( pT->GetItemFlags( m_nHotItem, nIndex ) & ITEM_FLAGS_READ_ONLY ) )
						InvalidateItem( m_nHotItem, m_nHotSubItem );
					
					m_nHotItem = nItem;
					m_nHotSubItem = nSubItem;

					NotifyParent(nItem, nSubItem, LCN_HOTITEMCHANGED);
				}
				
				int nIndex = GetColumnIndex( m_nHotSubItem );
				UINT nItemFormat = pT->GetItemFormat( m_nHotItem, nIndex );
				UINT nItemFlags = pT->GetItemFlags( m_nHotItem, nIndex );
				
				// draw new hot hyperlink item
				if ( nItemFormat == ITEM_FORMAT_HYPERLINK && !( nItemFlags & ITEM_FLAGS_READ_ONLY ) )
				{
					InvalidateItem( m_nHotItem, m_nHotSubItem );
					SetCursor( m_curHyperLink );
				}
				
				// get tooltip for this item
				stdstr strToolTip = pT->GetItemToolTip( m_nHotItem, nIndex );
				
				CRect rcSubItem;
				if ( !strToolTip.empty() && GetItemRect( m_nHotItem, rcSubItem ) )
				{
					m_ttToolTip.Activate( TRUE );
					m_ttToolTip.AddTool( m_hWnd, (LPCTSTR)strToolTip.substr(0, SHRT_MAX ).c_str(), rcSubItem, TOOLTIP_TOOL_ID );
				}
				else
				{
					m_ttToolTip.Activate( FALSE );
					m_ttToolTip.DelTool( m_hWnd, TOOLTIP_TOOL_ID );
				}
				
				// show titletips for this item
				ShowTitleTip( point, m_nHotItem, m_nHotSubItem );
			}
		}
	}
	
	void OnMouseLeave()
	{
		m_bMouseOver = FALSE;
		
		if ( m_nHotColumn != NULL_COLUMN )
		{
			m_nHotColumn = NULL_COLUMN;
			InvalidateHeader();
		}
		
		if ( m_nHotItem != NULL_ITEM || m_nHotSubItem != NULL_SUBITEM )
		{
			m_nHotItem = NULL_ITEM;
			m_nHotSubItem = NULL_SUBITEM;
			Invalidate();
		}
	}
	
	BOOL OnMouseWheel( UINT /*nFlags*/, short nDelta, CPoint /*point*/ )
	{
		HideTitleTip();
		
		// end any pending edit
		if ( m_bEditItem )
			SetFocus();
		
		int nRowsScrolled = m_nMouseWheelScroll * nDelta / 120;
		int nScrollPos = GetScrollPos( SB_VERT );
		
		if ( nRowsScrolled > 0 )
			nScrollPos = max( nScrollPos - ( nRowsScrolled * m_nItemHeight ), 0 );
		else
			nScrollPos += ( -nRowsScrolled * m_nItemHeight );
		
		ResetScrollBars( SB_VERT, nScrollPos, FALSE );
		Invalidate();
		
		return TRUE;
	}
	
	void OnTimer( UINT nIDEvent )
	{
		switch ( nIDEvent )
		{
			case RESIZE_COLUMN_TIMER:	ResizeColumn( TRUE );
										break;
			case ITEM_VISIBLE_TIMER:	{
											KillTimer( ITEM_VISIBLE_TIMER );
											
											int nFocusItem = NULL_ITEM;
											int nFocusSubItem = NULL_SUBITEM;
											
											// get current focus item
											if ( !GetFocusItem( nFocusItem, nFocusSubItem ) )
												break;
											
											// make sure current focus item is visible before editing
											if ( !EditItem( nFocusItem, nFocusSubItem ) )
												break;
										}
										break;
			case ITEM_AUTOSCROLL_TIMER:	if ( !m_bGroupSelect )
											KillTimer( ITEM_AUTOSCROLL_TIMER );
										else
										{
											DWORD dwPoint = GetMessagePos();
											CPoint ptMouse( GET_X_LPARAM( dwPoint ), GET_Y_LPARAM( dwPoint ) );
											ScreenToClient( &ptMouse );
		
											// automatically scroll when group selecting
											AutoScroll( ptMouse );
											AutoSelect( ptMouse );
										}
										break;
			case ITEM_SCROLL_TIMER:		if ( !ScrollList() )
											EndScroll();
										break;
		}
	}
	
	void OnKeyDown( TCHAR nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ )
	{
		T* pT = static_cast<T*>(this);
		
		// stop any pending scroll
		EndScroll();
		
		BOOL bCtrlKey = ( ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0 );
		BOOL bShiftKey = ( ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 );
		
		CRect rcClient;
		GetClientRect( rcClient );
		
		int nFocusItem = NULL_ITEM;
		int nFocusSubItem = NULL_SUBITEM;
		GetFocusItem( nFocusItem, nFocusSubItem );
		
		switch ( nChar )
		{
			case VK_DOWN:	SetFocusItem( min( nFocusItem + 1, pT->GetItemCount() - 1 ), nFocusSubItem );
							break;
			case VK_UP:		SetFocusItem( max( nFocusItem - 1, 0 ), nFocusSubItem );
							break;
			case VK_NEXT:	SetFocusItem( min( nFocusItem + GetCountPerPage( FALSE ) - 1, pT->GetItemCount() - 1 ), nFocusSubItem );
							break;
			case VK_PRIOR:	SetFocusItem( max( nFocusItem - GetCountPerPage( FALSE ) + 1, 0 ), nFocusSubItem );
							break;
			case VK_HOME:	SetFocusItem( 0, nFocusSubItem );
							break;
			case VK_END:	SetFocusItem( pT->GetItemCount() - 1, nFocusSubItem );
							break;
			case VK_LEFT:	if ( m_bFocusSubItem )
								SetFocusItem( nFocusItem, max( nFocusSubItem - 1, 0 ) );
							else
								SetScrollPos( SB_HORZ, max( GetScrollPos( SB_HORZ ) - ( bCtrlKey ? ITEM_SCROLL_OFFSET * 10 : ITEM_SCROLL_OFFSET ), 0 ) );
							break;
			case VK_RIGHT:	if ( m_bFocusSubItem )
								SetFocusItem( nFocusItem, min( nFocusSubItem + 1, GetColumnCount() - 1 ) );
							else
								SetScrollPos( SB_HORZ, min( GetScrollPos( SB_HORZ ) + ( bCtrlKey ? ITEM_SCROLL_OFFSET * 10 : ITEM_SCROLL_OFFSET ), rcClient.Width() ) );
							break;
			case VK_TAB:	if ( !bCtrlKey && m_bFocusSubItem )
								SetFocusItem( nFocusItem, bShiftKey ? max( nFocusSubItem - 1, 0 ) : min( nFocusSubItem + 1, GetColumnCount() - 1 ) );
							break;
			default:		if ( nChar == VK_SPACE )
							{
								int nIndex = GetColumnIndex( nFocusSubItem );
								if ( !( pT->GetItemFlags( nFocusItem, nIndex ) & ITEM_FLAGS_READ_ONLY ) )
								{
									switch ( pT->GetItemFormat( nFocusItem, nIndex ) )
									{
										case ITEM_FORMAT_CHECKBOX:			pT->SetItemText( nFocusItem, nIndex, _ttoi( pT->GetItemText( nFocusItem, nIndex ) ) > 0 ? _T( "0" ) : _T( "1" ) );
																			NotifyParent( nFocusItem, nFocusSubItem, LCN_MODIFIED );
																			InvalidateItem( nFocusItem );
																			return;
										case ITEM_FORMAT_CHECKBOX_3STATE:	{
																				int nCheckImage = _ttoi( pT->GetItemText( nFocusItem, nIndex ) );
																				if ( nCheckImage < 0 )
																					pT->SetItemText( nFocusItem, nIndex, _T( "0" ) );
																				else if ( nCheckImage > 0 )
																					pT->SetItemText( nFocusItem, nIndex, _T( "-1" ) );
																				else
																					pT->SetItemText( nFocusItem, nIndex, _T( "1" ) );
																				
																				NotifyParent( nFocusItem, nFocusSubItem, LCN_MODIFIED );
																				InvalidateItem( nFocusItem );
																			}
																			return;
									}
								}
							}
							
							if ( bCtrlKey && nChar == _T( 'A' ) && !m_bSingleSelect )
							{
								m_setSelectedItems.clear();
								for ( int nItem = 0; nItem < pT->GetItemCount(); nItem++ )
									m_setSelectedItems.insert( nItem );
								Invalidate();
								return;
							}
							
							if ( !bCtrlKey && iswprint( nChar ) && iswupper( nChar ) )
							{
								int nSortIndex = GetColumnIndex( m_nSortColumn );
								int nStartItem = nFocusItem + 1;
								DWORD dwCurrentTick = GetTickCount();
								
								stdstr strStart;
								strStart += nChar;
								
								// has there been another keypress since last search period?
								if ( ( dwCurrentTick - m_dwSearchTick ) < SEARCH_PERIOD )
								{
									if ( m_strSearchString.substr(0, 1 ) != strStart )
										m_strSearchString += nChar;
									
									stdstr strFocusText = pT->GetItemText( nFocusItem, nSortIndex );
									
									// are we continuing to type characters under current focus item?
									if ( m_strSearchString.length() > 1 && _tcsicmp(m_strSearchString.c_str(),strFocusText.substr(0, m_strSearchString.length() ).c_str() ) == 0 )
									{
										m_dwSearchTick = GetTickCount();
										return;
									}
								}
								else
								{
									if ( m_strSearchString.substr(0, 1 ) != strStart )
										nStartItem = 0;
									m_strSearchString = strStart;
								}
								
								m_dwSearchTick = GetTickCount();
								
								// scan for next search string
								for ( int nFirst = nStartItem; nFirst < pT->GetItemCount(); nFirst++ )
								{
									stdstr strItemText = pT->GetItemText( nFirst, nSortIndex );
									
									if ( _tcsicmp(m_strSearchString.c_str(), strItemText.substr(0, m_strSearchString.length() ).c_str() ) == 0 )
									{
										SelectItem( nFirst, nFocusSubItem, TRUE );
										EnsureItemVisible( nFirst, nFocusSubItem );
										return;
									}
								}
								
								// re-scan from top if not found search string
								for ( int nSecond = 0; nSecond < pT->GetItemCount(); nSecond++ )
								{
									stdstr strItemText = pT->GetItemText( nSecond, nSortIndex );
									
									if ( _tcsicmp(m_strSearchString.c_str(), strItemText.substr(0, m_strSearchString.length() ).c_str() ) == 0 )
									{
										SelectItem( nSecond, nFocusSubItem, TRUE );
										EnsureItemVisible( nSecond, nFocusSubItem );
										return;
									}
								}
							}
							return;
		}

		if ( !bCtrlKey )
			SelectItem( m_nFocusItem, m_nFocusSubItem, bShiftKey ? MK_SHIFT : 0 );
	}
	
	void OnSysKeyDown( TCHAR /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/ )
	{
		HideTitleTip( FALSE );
		SetMsgHandled( FALSE );		
	}
	
	void OnSettingsChange( UINT /*nFlags*/, LPCTSTR /*lpszSection*/ )
	{
		OnSettingsChange();
	}
	
	void OnSettingsChange()
	{
		LoadSettings();		
		ResetScrollBars();
		Invalidate();
	}
	
	LRESULT OnCtlColorListBox( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
	{
		return DefWindowProc( nMsg, wParam, lParam );
	}
	
	LRESULT OnEndEdit( LPNMHDR lpNMHDR )
	{
		T* pT = static_cast<T*>(this);
		CListNotify *pListNotify = reinterpret_cast<CListNotify *>( lpNMHDR );
		
		m_bEditItem = FALSE;
		int nIndex = GetColumnIndex( pListNotify->m_nSubItem );
		
		switch ( pListNotify->m_nExitChar )
		{
			case VK_ESCAPE:	break; // do nothing
			case VK_DELETE:	pT->SetItemText( pListNotify->m_nItem, nIndex, _T( "" ) );
							NotifyParent( pListNotify->m_nItem, pListNotify->m_nSubItem, LCN_MODIFIED );
							break;
			default:		if ( pListNotify->m_lpItemDate == NULL )
								pT->SetItemText( pListNotify->m_nItem, nIndex, pListNotify->m_lpszItemText );
							else
							{
								if ( _ttoi( pListNotify->m_lpszItemText ) == 0 )
									pT->SetItemText( pListNotify->m_nItem, nIndex, _T( "" ) );
								else
									pT->SetItemDate( pListNotify->m_nItem, nIndex, *pListNotify->m_lpItemDate );
							}
							if ( pListNotify->m_nExitChar == VK_TAB )
								PostMessage( WM_KEYDOWN, (WPARAM)VK_TAB );
							NotifyParent( pListNotify->m_nItem, pListNotify->m_nSubItem, LCN_MODIFIED );
							break;
		}
		
		InvalidateItem( pListNotify->m_nItem );
		
		return 0;	
	}
	
	DWORD OnDragEnter( FORMATETC& FormatEtc, STGMEDIUM& StgMedium, DWORD /*dwKeyState*/, CPoint point )
	{
		DWORD dwEffect = DROPEFFECT_NONE;
		
		if ( FormatEtc.cfFormat == m_nHeaderClipboardFormat )
		{
			LPBYTE lpDragHeader = (LPBYTE)GlobalLock( StgMedium.hGlobal );
			if ( lpDragHeader == NULL )
				return DROPEFFECT_NONE;
			
			// dragged column must originate from this control
			if ( *( (HWND*)lpDragHeader ) == m_hWnd )
				dwEffect = DropColumn( point ) ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
			
			GlobalUnlock( StgMedium.hGlobal );
		}
		
		return dwEffect;
	}
	
	DWORD OnDragOver( FORMATETC& FormatEtc, STGMEDIUM& StgMedium, DWORD /*dwKeyState*/, CPoint point )
	{
		DWORD dwEffect = DROPEFFECT_NONE;
		
		if ( FormatEtc.cfFormat == m_nHeaderClipboardFormat )
		{
			LPBYTE lpDragHeader = (LPBYTE)GlobalLock( StgMedium.hGlobal );
			if ( lpDragHeader == NULL )
				return DROPEFFECT_NONE;
			
			// dragged column must originate from this control
			if ( *( (HWND*)lpDragHeader ) == m_hWnd )
				dwEffect = DropColumn( point ) ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
			
			GlobalUnlock( StgMedium.hGlobal );
		}
		
		return dwEffect;
	}
	
	BOOL OnDrop( FORMATETC& FormatEtc, STGMEDIUM& /*StgMedium*/, DWORD /*dwEffect*/, CPoint /*point*/ )
	{
		if ( FormatEtc.cfFormat == m_nHeaderClipboardFormat )
		{
			if ( m_nDragColumn != NULL_COLUMN && m_nHotDivider != NULL_COLUMN )
			{
				CListColumn listColumn;
				if ( !GetColumn( m_nDragColumn, listColumn ) )
					return FALSE;
			
				// move column to new position
				m_aColumns.RemoveAt( m_nDragColumn );
				m_aColumns.InsertAt( ( m_nDragColumn < m_nHotColumn ? ( m_nHotDivider == 0 ? 0 : m_nHotDivider - 1 ) : m_nHotDivider ), listColumn );
				Invalidate();
			}

			return TRUE;
		}			
		
		// not supported
		return FALSE;
	}
	
	void OnDragLeave()
	{
	}
	
	BOOL OnRenderData( FORMATETC& FormatEtc, STGMEDIUM *pStgMedium, BOOL /*bDropComplete*/ )
	{
		if ( FormatEtc.cfFormat == m_nHeaderClipboardFormat )
		{
			pStgMedium->tymed = TYMED_HGLOBAL;
			pStgMedium->hGlobal = GlobalAlloc( GMEM_MOVEABLE, sizeof( HWND ) );
			if ( pStgMedium->hGlobal == NULL )
				return FALSE;
			
			LPBYTE lpDragHeader = (LPBYTE)GlobalLock( pStgMedium->hGlobal );
			if ( lpDragHeader == NULL )
				return FALSE;
			
			// store this window handle
			*( (HWND*)lpDragHeader ) = m_hWnd;
			
			GlobalUnlock( pStgMedium->hGlobal );
		
			return TRUE;
		}
			
		return FALSE;
	}
	
	void DoPaint( CDCHandle dcPaint )
	{
		T* pT = static_cast<T*>(this);
		
		int nContextState = dcPaint.SaveDC();
		
		pT->DrawBkgnd( dcPaint );
		pT->DrawList( dcPaint );
		pT->DrawSelect( dcPaint );
		pT->DrawHeader( dcPaint );
		
		dcPaint.RestoreDC( nContextState );
	}
	
	void DrawBkgnd( CDCHandle dcPaint )
	{
		CRect rcClip;
		if ( dcPaint.GetClipBox( rcClip ) == ERROR )
			return;
		
		dcPaint.SetBkColor( m_rgbBackground );
		dcPaint.ExtTextOut( rcClip.left, rcClip.top, ETO_OPAQUE, rcClip, _T( "" ), 0, NULL );
		
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		if ( !m_bmpBackground.IsNull() && rcClip.bottom > rcClient.top )
		{
			CSize sizBackground;
			m_bmpBackground.GetSize( sizBackground );
		
			CDC dcBackgroundImage;
			dcBackgroundImage.CreateCompatibleDC( dcPaint );
		
			HBITMAP hOldBitmap = dcBackgroundImage.SelectBitmap( m_bmpBackground );
			
			if ( m_bTileBackground )
			{
				// calculate tile image maximum rows and columns
				div_t divRows = div( (int)rcClient.Height(), (int)sizBackground.cy );
				int nTileRows = divRows.rem > 0 ? divRows.quot + 1 : divRows.quot;
				div_t divColumns = div( (int)rcClient.Width(), (int)sizBackground.cx );
				int nTileColumns = divColumns.rem > 0 ? divColumns.quot + 1 : divColumns.quot;
				
				// draw tiled background image
				for ( int nRow = 0; nRow <= nTileRows; nRow++ )
				{
					for ( int nColumn = 0; nColumn <= nTileColumns; nColumn++ )
						dcPaint.BitBlt( nColumn * sizBackground.cx, nRow * sizBackground.cy, sizBackground.cx, sizBackground.cy, dcBackgroundImage, 0, 0, SRCCOPY );
				}
			}
			else
			{
				CRect rcCentreImage( rcClient );
				
				// horizontally centre image if smaller than the client width
				if ( sizBackground.cx < rcClient.Width() )
				{
					rcCentreImage.left = ( rcClient.Width() / 2 ) - (int)( sizBackground.cx / 2 );
					rcCentreImage.right = rcCentreImage.left + sizBackground.cx;
				}
				
				// vertically centre image if smaller than the client height
				if ( sizBackground.cy + 16 < rcClient.Height() )
				{
					rcCentreImage.top = ( rcClient.Height() / 2 ) - (int)( ( sizBackground.cy + 16 ) / 2 );
					rcCentreImage.bottom = rcCentreImage.top + sizBackground.cy;
				}
				
				// draw centred background image
				dcPaint.BitBlt( rcCentreImage.left, rcCentreImage.top, rcCentreImage.Width(), rcCentreImage.Height(), dcBackgroundImage, 0, 0, SRCCOPY );
			}

			dcBackgroundImage.SelectBitmap( hOldBitmap );
		}		
	}
	
	void DrawHeader( CDCHandle dcPaint )
	{
		if ( !m_bShowHeader )
			return;

		CRect rcClip;
		if ( dcPaint.GetClipBox( rcClip ) == ERROR )
			return;

		CRect rcHeader;
		GetClientRect( rcHeader );
		rcHeader.bottom = m_nHeaderHeight;

		if ( rcClip.top > rcHeader.bottom )
			return;

		dcPaint.SetBkColor( m_rgbHeaderBackground );
		dcPaint.ExtTextOut( rcHeader.left, rcHeader.top, ETO_OPAQUE, rcHeader, _T( "" ), 0, NULL );

		CPen penHighlight;
		penHighlight.CreatePen( PS_SOLID, 1, m_rgbHeaderBorder );
		CPen penShadow;
		penShadow.CreatePen( PS_SOLID, 1, m_rgbHeaderShadow );

		CRect rcHeaderItem( rcHeader );
		rcHeaderItem.OffsetRect( -GetScrollPos( SB_HORZ ), 0 );

		int nHeaderWidth = 0;

		for ( int nColumn = 0, nColumnCount = GetColumnCount(); nColumn < nColumnCount; rcHeaderItem.left = rcHeaderItem.right, nColumn++ )
		{
			CListColumn listColumn;
			if ( !GetColumn( nColumn, listColumn ) )
				break;

			rcHeaderItem.right = rcHeaderItem.left + listColumn.m_nWidth;
			nHeaderWidth += rcHeaderItem.Width();

			if ( rcHeaderItem.right < rcClip.left )
				continue;
			if ( rcHeaderItem.left > rcClip.right )
				break;

			// draw header and divider
			if ( nColumn == m_nHighlightColumn )
			{
				dcPaint.SetBkColor( m_rgbHeaderHighlight );
				dcPaint.ExtTextOut( rcHeaderItem.left, rcHeaderItem.top, ETO_OPAQUE, rcHeaderItem, _T( "" ), 0, NULL );
			}


			dcPaint.SelectPen( penShadow );
			dcPaint.MoveTo( rcHeaderItem.right - 1, rcHeaderItem.top + 1 );
			dcPaint.LineTo( rcHeaderItem.right - 1, m_nHeaderHeight - 1 );

			dcPaint.SelectPen( penHighlight );
			dcPaint.MoveTo( rcHeaderItem.right, rcHeaderItem.top + 1 );
			dcPaint.LineTo( rcHeaderItem.right, m_nHeaderHeight - 1 );


			CRect rcHeaderText( rcHeaderItem );
			rcHeaderText.left += nColumn == 0 ? 4 : 3;
			rcHeaderText.OffsetRect( 0, 1 );

			BOOL bShowArrow = m_bShowSort && ( rcHeaderItem.Width() > 15 );


			if(listColumn.m_nImage == ITEM_IMAGE_NONE )
			{
				// offset text bounding rectangle to account for sorting arrow
				if ( bShowArrow && !listColumn.m_bFixed && listColumn.m_nIndex == m_nSortColumn )
					rcHeaderText.right -= 15;
			}

			// margin header text
			rcHeaderText.DeflateRect( 4, 0, 5, 0 );

			// has this header item an associated image?
			if ( listColumn.m_nImage != ITEM_IMAGE_NONE )
			{
				CSize sizeIcon;
				m_ilListItems.GetIconSize( sizeIcon );

				CRect rcHeaderImage;
				rcHeaderImage.left = listColumn.m_strText.empty() ? ( ( rcHeaderText.left + rcHeaderText.right ) / 2 ) - ( sizeIcon.cx / 2 ) - ( 0 ) : rcHeaderText.left;
				rcHeaderImage.right = min( rcHeaderImage.left + sizeIcon.cx, rcHeaderItem.right - 2 );
				rcHeaderImage.top = ( ( rcHeaderItem.top + rcHeaderItem.bottom ) / 2 ) - ( sizeIcon.cy / 2 );
				rcHeaderImage.bottom = min( rcHeaderImage.top + sizeIcon.cy, rcHeaderItem.bottom );

				if(listColumn.m_nIndex == m_nSortColumn)
					m_ilListItems.DrawEx( listColumn.m_nImage, dcPaint, rcHeaderImage, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT | ILD_SELECTED );
				else
					m_ilListItems.DrawEx( listColumn.m_nImage, dcPaint, rcHeaderImage, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );

				// offset header text (for image)
				rcHeaderText.left += sizeIcon.cx + 4;
			}

			dcPaint.SelectFont( m_fntListFont );
			dcPaint.SetTextColor( m_rgbHeaderText );
			dcPaint.SetBkMode( TRANSPARENT );

			UINT nFormat = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;

			if ( listColumn.m_nFlags & ITEM_FLAGS_CENTRE )
				nFormat |= DT_CENTER;
			else if ( listColumn.m_nFlags & ITEM_FLAGS_RIGHT )
				nFormat |= DT_RIGHT;
			else
				nFormat |= DT_LEFT;

			// draw header text
			if ( !rcHeaderText.IsRectEmpty() && !listColumn.m_strText.empty() )
				dcPaint.DrawText( listColumn.m_strText.c_str(), (int)listColumn.m_strText.length(), rcHeaderText, nFormat );

			// draw sorting arrow
			if ( bShowArrow && !listColumn.m_bFixed && listColumn.m_nIndex == m_nSortColumn )
			{
				CSize sizeIcon;
				m_ilListItems.GetIconSize( sizeIcon );

				CRect rcSortArrow;
				rcSortArrow.left = rcHeaderText.right + 4;
				rcSortArrow.right = min( rcSortArrow.left + sizeIcon.cx, rcHeaderItem.right );
				rcSortArrow.top = rcHeaderItem.Height() / 2 - 3;
				rcSortArrow.bottom = min( rcSortArrow.top + sizeIcon.cy, rcHeaderItem.bottom );

				m_ilListItems.DrawEx( m_bSortAscending ? ITEM_IMAGE_UP : ITEM_IMAGE_DOWN, dcPaint, rcSortArrow, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );
			}
		}

		// draw a frame around all header columns

		if ( nHeaderWidth > 0 )
			dcPaint.Draw3dRect( CRect( rcHeader.left, rcHeader.top, rcHeader.right + 2, rcHeader.bottom ), m_rgbHeaderBorder, m_rgbHeaderShadow );

	}
	
	void DrawRoundRect( CDCHandle dcPaint, CRect& rcRect, COLORREF rgbOuter, COLORREF rgbInner )
	{
		CRect rcRoundRect( rcRect );
					
		CPen penBorder;
		penBorder.CreatePen( PS_SOLID, 1, rgbOuter );
		CBrush bshInterior;
		bshInterior.CreateSolidBrush( m_rgbBackground );
		
		dcPaint.SelectPen( penBorder );
		dcPaint.SelectBrush( bshInterior );
		
		dcPaint.RoundRect( rcRoundRect, CPoint( 5, 5 ) );
		rcRoundRect.DeflateRect( 1, 1 );
		
		CPen penInnerBorder;
		penInnerBorder.CreatePen( PS_SOLID, 1, rgbInner );
		dcPaint.SelectPen( penInnerBorder );
		
		dcPaint.RoundRect( rcRoundRect, CPoint( 2, 2 ) );
	}
	
	void DrawGradient( CDCHandle dcPaint, CRect& rcRect, COLORREF rgbTop, COLORREF rgbBottom )
	{
		GRADIENT_RECT grdRect = { 0, 1 };
		TRIVERTEX triVertext[ 2 ] = {
										rcRect.left,
										rcRect.top,
										GetRValue( rgbTop ) << 8,
										GetGValue( rgbTop ) << 8,
										GetBValue( rgbTop ) << 8,
										0x0000,			
										rcRect.right,
										rcRect.bottom,
										GetRValue( rgbBottom ) << 8,
										GetGValue( rgbBottom ) << 8,
										GetBValue( rgbBottom ) << 8,
										0x0000
									};
		
		dcPaint.GradientFill( triVertext, 2, &grdRect, 1, GRADIENT_FILL_RECT_V );
	}
	
	void DrawList( CDCHandle dcPaint )
	{
		T* pT = static_cast<T*>(this);
		
		CRect rcClip;
		if ( dcPaint.GetClipBox( rcClip ) == ERROR )
			return;
		
		CRect rcItem;
		rcItem.left = -GetScrollPos( SB_HORZ );
		rcItem.right = GetTotalWidth();
		rcItem.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		rcItem.bottom = rcItem.top;
		
		// draw all visible items
		for ( int nItem = GetTopItem(); nItem < pT->GetItemCount(); rcItem.top = rcItem.bottom, nItem++ )
		{
			rcItem.bottom = rcItem.top + m_nItemHeight;
			
			if ( rcItem.bottom < rcClip.top || rcItem.right < rcClip.left )
				continue;			
			if ( rcItem.top > rcClip.bottom || rcItem.left > rcClip.right )
				break;
			
			// may be implemented in a derived class
			pT->DrawItem( dcPaint, nItem, rcItem );
		}
	}
	
	void DrawItem( CDCHandle dcPaint, int nItem, CRect& rcItem )
	{
		T* pT = static_cast<T*>(this);
		
		CRect rcClip;
		if ( dcPaint.GetClipBox( rcClip ) == ERROR )
			return;
			
		int nFocusItem = NULL_ITEM;
		int nFocusSubItem = NULL_SUBITEM;
		GetFocusItem( nFocusItem, nFocusSubItem );
		
		BOOL bSelectedItem = IsSelected( nItem );
		//BOOL bControlFocus = ( GetFocus() == m_hWnd || m_bEditItem );
		
		// draw selected background
		if ( bSelectedItem )
		{
			dcPaint.SetBkColor( m_rgbSelectedItem );
			dcPaint.ExtTextOut( rcItem.left, rcItem.top, ETO_OPAQUE, rcItem, _T( "" ), 0, NULL );
		}
		
		CRect rcSubItem( rcItem );
		rcSubItem.right = rcSubItem.left;
		
		for ( int nSubItem = 0, nColumnCount = GetColumnCount(); nSubItem < nColumnCount; rcSubItem.left = rcSubItem.right + 1, nSubItem++ )
		{
			CListColumn listColumn;
			if ( !GetColumn( nSubItem, listColumn ) )
				break;
			
			rcSubItem.right = rcSubItem.left + listColumn.m_nWidth - 1;
			
			if ( rcSubItem.right < rcClip.left || rcSubItem.Width() == 0 )
				continue;
			if ( rcSubItem.left > rcClip.right )
				break;
			
			LPCTSTR strItemText = pT->GetItemText( nItem, listColumn.m_nIndex );
			int nItemImage = pT->GetItemImage( nItem, listColumn.m_nIndex );
			UINT nItemFormat = pT->GetItemFormat( nItem, listColumn.m_nIndex );
			UINT nItemFlags = pT->GetItemFlags( nItem, listColumn.m_nIndex );
			
			// custom draw subitem format
			if ( nItemFormat == ITEM_FORMAT_CUSTOM )
			{
				pT->DrawCustomItem( dcPaint, nItem, nSubItem, rcSubItem );
				return;
			}
			
			BOOL bFocusSubItem = ( m_bFocusSubItem && nFocusItem == nItem && nFocusSubItem == nSubItem );
			
			COLORREF rgbBackground = m_rgbBackground;
			COLORREF rgbText = m_rgbItemText;
			
			if ( bFocusSubItem )
			{
				dcPaint.SetBkColor( m_bEditItem ? m_rgbBackground : m_rgbItemFocus );
				dcPaint.ExtTextOut( rcSubItem.left, rcSubItem.top, ETO_OPAQUE, rcSubItem, _T( "" ), 0, NULL );
				
				if ( m_bEditItem )
				{
					CBrush bshSelectFrame;
					bshSelectFrame.CreateSolidBrush( m_rgbItemFocus );
					dcPaint.FrameRect( rcSubItem, bshSelectFrame );
				}
			}
			else if ( pT->GetItemColours( nItem, nSubItem, rgbBackground, rgbText ) && rgbBackground != m_rgbBackground )
			{
				CPen penBorder;
				penBorder.CreatePen( PS_SOLID, 1, rgbBackground );
				CBrush bshInterior;
				bshInterior.CreateSolidBrush( rgbBackground );
				
				dcPaint.SelectPen( penBorder );
				dcPaint.SelectBrush( bshInterior );
				
				dcPaint.RoundRect( rcSubItem, CPoint( 3, 3 ) );
			}
			
			CRect rcItemText( rcSubItem );
			
			// margin item text
			//rcItemText.left += nSubItem == 0 ? 4 : 3;
			//rcItemText.DeflateRect( 4, 0 );
			
			// draw subitem image if supplied
			if ( !m_ilItemImages.IsNull() && nItemImage != ITEM_IMAGE_NONE && ( !m_bEditItem || ( m_bEditItem && !bFocusSubItem ) ) )
			{
				CSize sizeIcon;
				m_ilItemImages.GetIconSize( sizeIcon );
				
				CRect rcItemImage;
				rcItemImage.left = (strItemText[0] == 0) ? ( ( rcItemText.left + rcItemText.right ) / 2 ) - ( sizeIcon.cx / 2 ) - ( 0 ) : rcItemText.left;
				rcItemImage.right = min( rcItemImage.left + sizeIcon.cx, rcSubItem.right );
				rcItemImage.top = ( ( rcSubItem.top + rcSubItem.bottom ) / 2 ) - ( sizeIcon.cy / 2 );
				rcItemImage.bottom = min( rcItemImage.top + sizeIcon.cy, rcSubItem.bottom );
				
				m_ilItemImages.DrawEx( nItemImage, dcPaint, rcItemImage, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );
					
				// offset item text (for image)
				rcItemText.left += sizeIcon.cx + 4;
			}
			
			if ( rcItemText.IsRectEmpty() )
				continue;
			
			COLORREF rgbSelectedText = m_rgbSelectedText;
			pT->GetItemSelectedColours( nItem, nSubItem, rgbSelectedText );

			dcPaint.SelectFont( pT->GetItemFont( nItem, nSubItem ) );
			dcPaint.SetTextColor( ( bSelectedItem && !bFocusSubItem ) ? rgbSelectedText : rgbText );
			dcPaint.SetBkMode( TRANSPARENT );

			UINT nFormat = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;

			if ( nItemFlags & ITEM_FLAGS_CENTRE )
				nFormat |= DT_CENTER;
			else if ( nItemFlags & ITEM_FLAGS_RIGHT )
				nFormat |= DT_RIGHT;
			else
				nFormat |= DT_LEFT;
		
			switch ( nItemFormat )
			{
				case ITEM_FORMAT_DATETIME:			if ( strItemText[0] != 0 )
													{
														SYSTEMTIME stItemDate;
														if ( !GetItemDate( nItem, listColumn.m_nIndex, stItemDate ) )
															break;
														
														stdstr strItemDate;
														if ( nItemFlags & ITEM_FLAGS_DATE_ONLY )
															strItemDate = FormatDate( stItemDate );
														else if ( nItemFlags & ITEM_FLAGS_TIME_ONLY )
															strItemDate = FormatTime( stItemDate );
														else
															strItemDate = FormatDate( stItemDate ) + _T( " " ) + FormatTime( stItemDate );															
														dcPaint.DrawText( strItemDate.c_str(), (int)strItemDate.length(), rcItemText, nFormat );
													}
													break;
				case ITEM_FORMAT_CHECKBOX:			
				case ITEM_FORMAT_CHECKBOX_3STATE:	{
														CSize sizeIcon;
														m_ilListItems.GetIconSize( sizeIcon );
				
														CRect rcCheckBox;
														rcCheckBox.left = ( ( rcItemText.left + rcItemText.right ) / 2 ) - ( sizeIcon.cx / 2 ) - 1;
														rcCheckBox.right = min( rcCheckBox.left + sizeIcon.cx, rcSubItem.right );
														rcCheckBox.top = ( ( rcSubItem.top + rcSubItem.bottom ) / 2 ) - ( sizeIcon.cy / 2 );
														rcCheckBox.bottom = min( rcCheckBox.top + sizeIcon.cy, rcSubItem.bottom );
														
														int nCheckValue = _ttoi( strItemText );
																
														if ( nItemFormat == ITEM_FORMAT_CHECKBOX )
															m_ilListItems.DrawEx( nCheckValue > 0 ? ITEM_IMAGE_CHECK_ON : ITEM_IMAGE_CHECK_OFF, dcPaint, rcCheckBox, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );
														else
														{
															int nCheckImage = ITEM_IMAGE_3STATE_UNDEF;
															if ( nCheckValue < 0 )
																nCheckImage = ITEM_IMAGE_3STATE_OFF;
															else if ( nCheckValue > 0 )
																nCheckImage = ITEM_IMAGE_3STATE_ON;
															m_ilListItems.DrawEx( nCheckImage, dcPaint, rcCheckBox, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT );
														}
													}														
													break;
				case ITEM_FORMAT_PROGRESS:			{
														CRect rcProgress( rcSubItem );
														rcProgress.DeflateRect( 3, 2 );
														
														// draw progress border
														DrawRoundRect( dcPaint, rcProgress, m_rgbHeaderShadow, m_rgbHeaderBackground );
														
														// fill progress bar area
														rcProgress.DeflateRect( 3, 3 );
														rcProgress.right = rcProgress.left + (int)( (double)rcProgress.Width() * ( ( max( min( atof( strItemText ), 100 ), 0 ) ) / 100.0 ) );
														DrawGradient( dcPaint, rcProgress, m_rgbProgressTop, m_rgbProgressBottom );
													}					
													break;
				case ITEM_FORMAT_HYPERLINK:			if ( nItem == m_nHotItem && nSubItem == m_nHotSubItem && !( nItemFlags & ITEM_FLAGS_READ_ONLY ) )
													{
														dcPaint.SelectFont( m_fntUnderlineFont );
														dcPaint.SetTextColor( m_rgbHyperLink );
													}
				default:							// draw item text
					{
						size_t len = strlen(strItemText);
						if ( len > 0 )
							dcPaint.DrawText( strItemText, (int)len, rcItemText, nFormat );

					}
			
													break;
			}
		}
	}
	
	void DrawSelect( CDCHandle dcPaint )
	{
		if ( !m_bGroupSelect )
			return;
		
		int nHorzScroll = GetScrollPos( SB_HORZ );
		int nVertScroll = GetScrollPos( SB_VERT );
		
		CRect rcGroupSelect( m_rcGroupSelect );
		rcGroupSelect.OffsetRect( -nHorzScroll, -nVertScroll );
		
		CRect rcClient;
		GetClientRect( rcClient );
		rcClient.top = ( m_bShowHeader ? m_nHeaderHeight : 0 );
		
		// limit box to list client area if scrolled to limits
		if ( nHorzScroll > ( GetTotalWidth() - rcClient.Width() ) )
			rcGroupSelect.right = min( rcClient.right, rcGroupSelect.right );
		if ( nHorzScroll == 0 )
			rcGroupSelect.left = max( rcClient.left, rcGroupSelect.left );
		if ( nVertScroll > ( GetTotalHeight() - rcClient.Height() ) )
			rcGroupSelect.bottom = min( rcClient.bottom, rcGroupSelect.bottom );
		if ( nVertScroll == 0 )
			rcGroupSelect.top = max( rcClient.top, rcGroupSelect.top );
		
		// limit bitmap to client area
		CRect rcSelectArea( rcGroupSelect );
		rcSelectArea.IntersectRect( rcSelectArea, rcClient );
		
		CDC dcBackground;
		dcBackground.CreateCompatibleDC( dcPaint );
		
		int nBackgroundContext = dcBackground.SaveDC();
		
		CBitmap bmpBackground;
		bmpBackground.CreateCompatibleBitmap( dcPaint, rcSelectArea.Width(), rcSelectArea.Height() ); 
		dcBackground.SelectBitmap( bmpBackground );
		
		// take a copy of existing backgroud
		dcBackground.BitBlt( 0, 0, rcSelectArea.Width(), rcSelectArea.Height(), dcPaint, rcSelectArea.left, rcSelectArea.top, SRCCOPY );
		
		CDC dcGroupSelect;
		dcGroupSelect.CreateCompatibleDC( dcPaint );
		
		int nGroupSelectContext = dcGroupSelect.SaveDC();
		
		CBitmap bmpGroupSelect;
		bmpGroupSelect.CreateCompatibleBitmap( dcPaint, rcSelectArea.Width(), rcSelectArea.Height() ); 
		dcGroupSelect.SelectBitmap( bmpGroupSelect );
		
		// draw group select box
		dcGroupSelect.SetBkColor( m_rgbItemFocus );
		dcGroupSelect.ExtTextOut( 0, 0, ETO_OPAQUE, CRect( CPoint( 0 ), rcSelectArea.Size() ), _T( "" ), 0, NULL );
		
		BLENDFUNCTION blendFunction;
		blendFunction.BlendOp = AC_SRC_OVER;
		blendFunction.BlendFlags = 0;
		blendFunction.SourceConstantAlpha = 180;
		blendFunction.AlphaFormat = 0;
		
		// blend existing background with selection box
		dcGroupSelect.AlphaBlend( 0, 0, rcSelectArea.Width(), rcSelectArea.Height(), dcBackground, 0, 0, rcSelectArea.Width(), rcSelectArea.Height(), blendFunction ); 
		
		// draw blended selection box
		dcPaint.BitBlt( rcSelectArea.left, rcSelectArea.top, rcSelectArea.Width(), rcSelectArea.Height(), dcGroupSelect, 0, 0, SRCCOPY );
		
		// draw selection box frame
		CBrush bshSelectFrame;
		bshSelectFrame.CreateSolidBrush( m_rgbItemText );
		dcPaint.FrameRect( rcGroupSelect, bshSelectFrame );
		
		dcBackground.RestoreDC( nBackgroundContext );
		dcGroupSelect.RestoreDC( nGroupSelectContext );
	}
	
	void DrawCustomItem( CDCHandle dcPaint, int /*nItem*/, int /*nSubItem*/, CRect& /*rcSubItem*/ )
	{
		ATLASSERT( FALSE ); // must be implemented in a derived class
	}
};

struct CSubItem
{
	stdstr m_strText;
	int m_nImage;
	UINT m_nFormat;
	UINT m_nFlags;
	UINT m_nMaxEditLen;
	CListArray < stdstr > m_aComboList;
	HFONT m_hFont;
	COLORREF m_rgbBackground;
	COLORREF m_rgbText;
	COLORREF m_rgbSelectedText;
};

template < class TData = DWORD >
struct CListItem
{
	CListArray < CSubItem > m_aSubItems;
	stdstr m_strToolTip;
	TData m_tData;	
};

template < class TData >
class CListCtrlData : public CListImpl< CListCtrlData< TData > >
{
public:
	DECLARE_WND_CLASS( _T( "ListCtrl" ) )

protected:
	CListArray < CListItem< TData > > m_aItems;
	
public:
	int AddItem( CListItem< TData >& listItem )
	{
		if ( !m_aItems.Add( listItem ) )
			return -1;
		return CListImpl< CListCtrlData >::AddItem() ? GetItemCount() - 1 : -1;
	}
	
	int AddItemAt( CListItem< TData >& listItem, int Index )
	{
		if (Index < 0 )
		{
			Index = 0;
		}
		if (Index > GetItemCount())
		{
			Index = GetItemCount();
		}
		if ( !m_aItems.AddAt( listItem, Index ) )
			return -1;
		return CListImpl< CListCtrlData >::AddItem() ? Index : -1;
	}

	int AddItem( LPCTSTR lpszText, int nImage = ITEM_IMAGE_NONE, UINT nFormat = ITEM_FORMAT_NONE, UINT nFlags = ITEM_FLAGS_NONE )
	{
		CSubItem listSubItem;
		listSubItem.m_nImage = ITEM_IMAGE_NONE;
		listSubItem.m_nFormat = nFormat;
		listSubItem.m_nFlags = ValidateFlags( nFlags );
		listSubItem.m_hFont = NULL;
		listSubItem.m_rgbBackground = m_rgbBackground;
		listSubItem.m_rgbText = m_rgbItemText;
		listSubItem.m_rgbSelectedText = m_rgbSelectedText;
		listSubItem.m_nMaxEditLen = -1;

		CListItem< TData > listItem;
		for ( int nSubItem = 0; nSubItem < GetColumnCount(); nSubItem++ )
			listItem.m_aSubItems.Add( listSubItem );

		// set item details for first subitem
		listItem.m_aSubItems[ 0 ].m_strText = lpszText;
		listItem.m_aSubItems[ 0 ].m_nImage = nImage;

		return AddItem( listItem );
	}

	int AddItemAt(int Index, LPCTSTR lpszText, int nImage = ITEM_IMAGE_NONE, UINT nFormat = ITEM_FORMAT_NONE, UINT nFlags = ITEM_FLAGS_NONE )
	{
		CSubItem listSubItem;
		listSubItem.m_nImage = ITEM_IMAGE_NONE;
		listSubItem.m_nFormat = nFormat;
		listSubItem.m_nFlags = ValidateFlags( nFlags );
		listSubItem.m_hFont = NULL;
		listSubItem.m_rgbBackground = m_rgbBackground;
		listSubItem.m_rgbText = m_rgbItemText;
		listSubItem.m_rgbSelectedText = m_rgbSelectedText;
		listSubItem.m_nMaxEditLen = (UINT)-1;

		CListItem< TData > listItem;
		for ( int nSubItem = 0; nSubItem < GetColumnCount(); nSubItem++ )
			listItem.m_aSubItems.Add( listSubItem );

		// set item details for first subitem
		listItem.m_aSubItems[ 0 ].m_strText = lpszText;
		listItem.m_aSubItems[ 0 ].m_nImage = nImage;

		return AddItemAt( listItem, Index );
	}
	
	BOOL DeleteItem( int nItem )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		return m_aItems.RemoveAt( nItem ) ? CListImpl< CListCtrlData >::DeleteItem( nItem ) : FALSE;
	}
	
	BOOL DeleteAllItems()
	{
		m_aItems.RemoveAll();
		return CListImpl< CListCtrlData >::DeleteAllItems();
	}
	
	int GetItemCount()
	{
		return m_aItems.GetSize();
	}
	
	BOOL GetItem( int nItem, CListItem< TData >& listItem )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		listItem = m_aItems[ nItem ];
		return TRUE;
	}
	
	BOOL GetItem( int nItem, CListItem< TData >*& listItem )
	{
		if ( nItem < 0 || nItem >= GetItemCount() )
		{
			listItem = NULL;
			return FALSE;
		}
		listItem = &m_aItems[ nItem ];
		return TRUE;
	}

	BOOL GetSubItem( int nItem, int nSubItem, CSubItem& listSubItem )
	{
		CListItem< TData > * listItem;
		if ( !GetItem( nItem, listItem ) )
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)listItem->m_aSubItems.GetSize() )
			return FALSE;
		listSubItem = listItem->m_aSubItems[ nSubItem ];
		return TRUE;
	}
	
	BOOL GetSubItem( int nItem, int nSubItem, CSubItem *& listSubItem )
	{
		CListItem< TData > * listItem;
		if ( !GetItem( nItem, listItem ) )
		{
			listSubItem = NULL;
			return FALSE;
		}
		if ( nSubItem < 0 || nSubItem >= (int)listItem->m_aSubItems.GetSize() )
		{
			listSubItem = NULL;
			return FALSE;
		}
		listSubItem = &listItem->m_aSubItems[ nSubItem ];
		return TRUE;
	}

	LPCTSTR GetItemText( int nItem, int nSubItem )
	{
		CSubItem * listSubItem;
		return GetSubItem( nItem, nSubItem, listSubItem ) ? listSubItem->m_strText.c_str() : _T( "" );
	}
	
	UINT GetItemMaxEditLen( int nItem, int nSubItem )
	{
		CSubItem * listSubItem;
		return GetSubItem( nItem, nSubItem, listSubItem ) ? listSubItem->m_nMaxEditLen : 0;
	}

	int GetItemImage( int nItem, int nSubItem )
	{
		CSubItem *listSubItem;
		return GetSubItem( nItem, nSubItem, listSubItem ) ? listSubItem->m_nImage : ITEM_IMAGE_NONE;
	}
	
	UINT GetItemFormat( int nItem, int nSubItem )
	{
		CSubItem * listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		return listSubItem->m_nFormat == ITEM_FORMAT_NONE ? GetColumnFormat( IndexToOrder( nSubItem ) ) : listSubItem->m_nFormat;
	}
	
	UINT GetItemFlags( int nItem, int nSubItem )
	{
		CSubItem *listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		return listSubItem->m_nFlags == ITEM_FLAGS_NONE ? GetColumnFlags( IndexToOrder( nSubItem ) ) : listSubItem->m_nFlags;
	}
	
	BOOL GetItemComboList( int nItem, int nSubItem, CListArray < stdstr >& aComboList )
	{
		CSubItem listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		aComboList = listSubItem.m_aComboList;
		return aComboList.IsEmpty() ? GetColumnComboList( IndexToOrder( nSubItem ), aComboList ) : !aComboList.IsEmpty();
	}
	
	HFONT GetItemFont( int nItem, int nSubItem )
	{
		CSubItem * listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		return listSubItem->m_hFont == NULL ? CListImpl< CListCtrlData >::GetItemFont( nItem, nSubItem ) : listSubItem->m_hFont;
	}
	
	BOOL GetItemColours( int nItem, int nSubItem, COLORREF& rgbBackground, COLORREF& rgbText )
	{
		CSubItem *listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		rgbBackground = listSubItem->m_rgbBackground;
		rgbText = listSubItem->m_rgbText;
		return TRUE;
	}
	
	BOOL GetItemSelectedColours( int nItem, int nSubItem, COLORREF& rgbSelectedText )
	{
		CSubItem *listSubItem;
		if ( !GetSubItem( nItem, nSubItem, listSubItem ) )
			return FALSE;
		rgbSelectedText = listSubItem->m_rgbSelectedText;
		return TRUE;
	}

	stdstr GetItemToolTip( int nItem, int /*nSubItem*/ )
	{
		CListItem< TData > listItem;
		return GetItem( nItem, listItem ) ? listItem.m_strToolTip : _T( "" );
	}
	
	BOOL GetItemData( int nItem, TData& tData )
	{
		CListItem< TData > listItem;
		if ( !GetItem( nItem, listItem ) )
			return FALSE;
		tData = listItem.m_tData;
		return TRUE;
	}
	
	BOOL SetItemText( int nItem, int nSubItem, LPCTSTR lpszText, bool bInvalidateItem = true)
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_strText = lpszText;
		
		// Added by Rowan - 11/12/2006 to fix an updating bug - can add this to the parameter list if needed
		if (bInvalidateItem)
			InvalidateItem(nItem, nSubItem);
		
		return TRUE;
	}
	
	BOOL SetItemComboIndex( int nItem, int nSubItem, int nIndex )
	{
		CListArray < stdstr > aComboList;
		if ( !GetItemComboList( nItem, nSubItem, aComboList ) )
			return FALSE;
		return SetItemText( nItem, nSubItem, nIndex < 0 || nIndex >= aComboList.GetSize() ? _T( "" ) : aComboList[ nIndex ] );
	}
	
	BOOL SetItemImage( int nItem, int nSubItem, int nImage )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nImage = nImage;
		return TRUE;
	}
	
	BOOL SetItemFormat( int nItem, int nSubItem, UINT nFormat, UINT nFlags = ITEM_FLAGS_NONE )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nFormat = nFormat;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nFlags = nFlags;
		return TRUE;
	}
	
	BOOL SetItemFormat( int nItem, int nSubItem, UINT nFormat, UINT nFlags, CListArray < stdstr >& aComboList )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nFormat = nFormat;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nFlags = nFlags;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_aComboList = aComboList;
		return TRUE;
	}
	
	BOOL SetItemMaxEditLen( int nItem, int nSubItem, UINT nMaxEditLen )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_nMaxEditLen = nMaxEditLen;
		return TRUE;
	}
	
	BOOL SetItemFont( int nItem, int nSubItem, HFONT hFont )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_hFont = hFont;
		return TRUE;
	}

	BOOL SetItemColours( int nItem, int nSubItem, COLORREF rgbBackground, COLORREF rgbText )
	{
		if ( nItem < 0 || nItem >= GetItemCount() )
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_rgbBackground = rgbBackground;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_rgbText = rgbText;
		return TRUE;
	}
	
	BOOL SetItemHighlightColours( int nItem, int nSubItem, COLORREF rgbSelectedText )
	{
		if ( nItem < 0 || nItem >= GetItemCount() )
			return FALSE;
		if ( nSubItem < 0 || nSubItem >= (int)m_aItems[ nItem ].m_aSubItems.GetSize() )
			return FALSE;
		m_aItems[ nItem ].m_aSubItems[ nSubItem ].m_rgbSelectedText = rgbSelectedText;
		return TRUE;
	}

	void ReverseItems()
	{
		m_aItems.Reverse();
	}
	
	class CompareItem
	{
	public:
		CompareItem( int nColumn ) : m_nColumn( nColumn ) {}
		inline bool operator() ( const CListItem< TData >& listItem1, const CListItem< TData >& listItem2 )
		{
			return ( _tcscmp(listItem1.m_aSubItems[ m_nColumn ].m_strText.c_str(), listItem2.m_aSubItems[ m_nColumn ].m_strText.c_str() ) < 0 );
		}
		
	protected:
		int m_nColumn;
	};
	
	void SortItems( int nColumn, BOOL /*bAscending*/ )
	{
		m_aItems.Sort( CompareItem( nColumn ) );
	}
	
	BOOL SetItemToolTip( int nItem, LPCTSTR lpszToolTip )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		m_aItems[ nItem ].m_strToolTip = lpszToolTip;
		return TRUE;
	}
	
	BOOL SetItemData( int nItem, TData& tData )
	{
		if ( nItem < 0 || nItem >= GetItemCount() ) 
			return FALSE;
		m_aItems[ nItem ].m_tData = tData;
		return TRUE;
	}
};

typedef CListCtrlData< DWORD > CListCtrl;
