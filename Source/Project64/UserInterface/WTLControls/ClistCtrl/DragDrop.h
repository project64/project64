#pragma once
#include <vector>
#include <shlobj.h>

using namespace std;

class CEnumFormatEtc : public IEnumFORMATETC
{
public:
	CEnumFormatEtc( const vector < FORMATETC >& vFormatEtc )
	{
		m_nRefCount = 0;
		m_nIndex = 0;
		m_vFormatEtc = vFormatEtc;
	}
	
protected:
	vector < FORMATETC > m_vFormatEtc;
	int m_nRefCount;
	int m_nIndex;

public:
	// IUnknown members
	STDMETHOD(QueryInterface)( REFIID refiid, void FAR* FAR* ppvObject )
	{
		*ppvObject = ( refiid == IID_IUnknown || refiid == IID_IEnumFORMATETC ) ? this : nullptr;
		
		if ( *ppvObject != nullptr )
			( (LPUNKNOWN)*ppvObject )->AddRef();
		
		return *ppvObject == nullptr ? E_NOINTERFACE : S_OK;
	}
	
	STDMETHOD_(ULONG, AddRef)( void )
	{
		return ++m_nRefCount;
	}
	
	STDMETHOD_(ULONG, Release)( void )
	{
		int nRefCount = --m_nRefCount;
		if ( nRefCount == 0 )
			delete this;
		return nRefCount;
	}
	
	// IEnumFORMATETC members
	STDMETHOD(Next)( ULONG celt, LPFORMATETC lpFormatEtc, ULONG FAR *pceltFetched )
	{
		if ( pceltFetched != nullptr )
			*pceltFetched=0;

		ULONG cReturn = celt;

		if ( celt <= 0 || lpFormatEtc == nullptr || m_nIndex >= (int)m_vFormatEtc.size() )
			return S_FALSE;

		if ( pceltFetched == nullptr && celt != 1 ) // pceltFetched can be nullptr only for 1 item request
			return S_FALSE;

		while ( m_nIndex < (int)m_vFormatEtc.size() && cReturn > 0 )
		{
			*lpFormatEtc++ = m_vFormatEtc[ m_nIndex++ ];
			cReturn--;
		}
		
		if ( pceltFetched != nullptr )
			*pceltFetched = celt - cReturn;

		return cReturn == 0 ? S_OK : S_FALSE;
	}
	
	STDMETHOD(Skip)( ULONG celt )
	{
		if ( ( m_nIndex + (int)celt ) >= (int)m_vFormatEtc.size() )
			return S_FALSE;
		m_nIndex += celt;
		return S_OK;
	}
	
	STDMETHOD(Reset)( void )
	{
		m_nIndex = 0;
		return S_OK;
	}
	
	STDMETHOD(Clone)( IEnumFORMATETC FAR * FAR* ppCloneEnumFormatEtc )
	{
		if ( ppCloneEnumFormatEtc == nullptr )
			return E_POINTER;

		*ppCloneEnumFormatEtc = new CEnumFormatEtc( m_vFormatEtc );
		( (CEnumFormatEtc*)*ppCloneEnumFormatEtc )->AddRef();
		( (CEnumFormatEtc*)*ppCloneEnumFormatEtc )->m_nIndex = m_nIndex;
		
		return S_OK;
	}
};

class CDropSource : public IDropSource
{
public:
	CDropSource()
	{
		m_nRefCount = 0;
	}

protected:
	int m_nRefCount;
	
public:
	// IUnknown members
	STDMETHOD(QueryInterface)( REFIID refiid, void FAR* FAR* ppvObject )
	{
		*ppvObject = ( refiid == IID_IUnknown || refiid == IID_IDropSource ) ? this : nullptr;
		
		if ( *ppvObject != nullptr )
			( (LPUNKNOWN)*ppvObject )->AddRef();
		
		return *ppvObject == nullptr ? E_NOINTERFACE : S_OK;
	}
	
	STDMETHOD_(ULONG, AddRef)( void )
	{
		return ++m_nRefCount;
	}
	
	STDMETHOD_(ULONG, Release)( void )
	{
		int nRefCount = --m_nRefCount;
		if ( nRefCount == 0 )
			delete this;
		return nRefCount;
	}
	
	// IDropSource members
	STDMETHOD(QueryContinueDrag)( BOOL bEscapePressed, DWORD dwKeyState )
	{
		if ( bEscapePressed )
			return DRAGDROP_S_CANCEL;
			
		if ( !( dwKeyState & ( MK_LBUTTON | MK_RBUTTON ) ) )
			return DRAGDROP_S_DROP;
		
		return S_OK;
	}
	
    STDMETHOD(GiveFeedback)( DWORD /*dwEffect*/ )
    {
		return DRAGDROP_S_USEDEFAULTCURSORS;
    }
};

class CDataObject : public IDataObject
{
public:
	CDataObject( CDropSource *pDropSource )
	{
		m_nRefCount = 0;
		m_pDropSource = pDropSource;
		m_bSwappedButtons = GetSystemMetrics( SM_SWAPBUTTON );
	}
	
	virtual ~CDataObject()
	{
		for ( vector < STGMEDIUM >::iterator posStgMedium = m_vStgMedium.begin(); posStgMedium != m_vStgMedium.end(); posStgMedium++ )
			ReleaseStgMedium( &( *posStgMedium ) );
	}

protected:
	CDropSource *m_pDropSource;
	int m_nRefCount;
	BOOL m_bSwappedButtons;
	
	vector < FORMATETC > m_vFormatEtc;
	vector < STGMEDIUM > m_vStgMedium;

public:
	// IUnknown members
	STDMETHOD(QueryInterface)( REFIID refiid, void FAR* FAR* ppvObject )
	{
		*ppvObject = ( refiid == IID_IUnknown || refiid == IID_IDataObject ) ? this : nullptr;
		
		if ( *ppvObject != nullptr )
			( (LPUNKNOWN)*ppvObject )->AddRef();
		
		return *ppvObject == nullptr ? E_NOINTERFACE : S_OK;
	}
	
	STDMETHOD_(ULONG, AddRef)( void )
	{
		return ++m_nRefCount;
	}
	
	STDMETHOD_(ULONG, Release)( void )
	{
		int nRefCount = --m_nRefCount;
		if ( nRefCount == 0 )
			delete this;
		return nRefCount;
	}
	
	// IDataObject members
	STDMETHOD(GetData)( FORMATETC __RPC_FAR *pformatetcIn, STGMEDIUM __RPC_FAR *pmedium )
	{
		if ( pformatetcIn == nullptr || pmedium == nullptr )
			return E_INVALIDARG;

		ZeroMemory( pmedium, sizeof( STGMEDIUM ) );
		
		for ( int nFormatEtc = 0; nFormatEtc < (int)m_vFormatEtc.size(); nFormatEtc++ )
		{
			if ( pformatetcIn->tymed & m_vFormatEtc[ nFormatEtc ].tymed &&
				 pformatetcIn->dwAspect == m_vFormatEtc[ nFormatEtc ].dwAspect &&
				 pformatetcIn->cfFormat == m_vFormatEtc[ nFormatEtc ].cfFormat )
			{
				if ( m_vStgMedium[ nFormatEtc ].tymed == TYMED_NULL )
					return OnRenderData( m_vFormatEtc[ nFormatEtc ], pmedium, ( GetAsyncKeyState( m_bSwappedButtons ? VK_RBUTTON : VK_LBUTTON ) >= 0 ) ) ? S_OK : DV_E_FORMATETC;
				
				CopyMedium( pmedium, m_vStgMedium[ nFormatEtc ], m_vFormatEtc[ nFormatEtc ] );
				return S_OK;
			}
		}
		
		return DV_E_FORMATETC;
	}
	
	STDMETHOD(GetDataHere)( FORMATETC __RPC_FAR * /*pformatetc*/, STGMEDIUM __RPC_FAR * /*pmedium*/ )
	{
		return E_NOTIMPL;	
	}
	
	STDMETHOD(QueryGetData)( FORMATETC __RPC_FAR *pformatetc )
	{
		if ( pformatetc == nullptr )
			return E_INVALIDARG;

		if ( !( pformatetc->dwAspect & DVASPECT_CONTENT ) )
			return DV_E_DVASPECT;
		
		HRESULT hResult = DV_E_TYMED;
		for ( int nFormatEtc = 0; nFormatEtc < (int)m_vFormatEtc.size(); nFormatEtc++ )
		{
			if ( !( pformatetc->tymed & m_vFormatEtc[ nFormatEtc ].tymed ) )
			{
				hResult = DV_E_TYMED;
				continue;
			}
			
			if ( pformatetc->cfFormat == m_vFormatEtc[ nFormatEtc ].cfFormat )
				return S_OK;
			
			hResult = DV_E_CLIPFORMAT;
		}

		return hResult;
	}
	
	STDMETHOD(GetCanonicalFormatEtc)( FORMATETC __RPC_FAR * /*pformatectIn*/, FORMATETC __RPC_FAR * pformatetcOut )
	{
		return pformatetcOut == nullptr ? E_INVALIDARG : DATA_S_SAMEFORMATETC;
	}
    
    STDMETHOD(SetData)( FORMATETC __RPC_FAR *pformatetc, STGMEDIUM __RPC_FAR *pmedium, BOOL bRelease )
    {
		if ( pformatetc == nullptr || pmedium == nullptr )
			return E_INVALIDARG;

		m_vFormatEtc.push_back( *pformatetc );
		
		STGMEDIUM StgMedium = *pmedium;
		 
		if ( !bRelease )
			CopyMedium( &StgMedium, *pmedium, *pformatetc );

		m_vStgMedium.push_back( StgMedium );

		return S_OK;
    }
    
    STDMETHOD(EnumFormatEtc)( DWORD dwDirection, IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc )
    {
		if ( ppenumFormatEtc == nullptr )
			return E_POINTER;

		switch ( dwDirection )
		{
			case DATADIR_GET:	*ppenumFormatEtc = new CEnumFormatEtc( m_vFormatEtc );
								( (CEnumFormatEtc*)*ppenumFormatEtc )->AddRef();
								return S_OK;
			default:			*ppenumFormatEtc = nullptr;
								return E_NOTIMPL;
		}
    }
    
    STDMETHOD(DAdvise)( FORMATETC __RPC_FAR * /*pformatetc*/, DWORD /*advf*/, IAdviseSink __RPC_FAR * /*pAdvSink*/, DWORD __RPC_FAR * /*pdwConnection*/ )
    {
		return OLE_E_ADVISENOTSUPPORTED;
    }
    
    STDMETHOD(DUnadvise)( DWORD /*dwConnection*/ )
    {
		return E_NOTIMPL;
    }
    
    STDMETHOD(EnumDAdvise)( IEnumSTATDATA __RPC_FAR *__RPC_FAR * /*ppenumAdvise*/ )
    {
		return OLE_E_ADVISENOTSUPPORTED;
    }
       
	void CopyMedium( STGMEDIUM *pMedDest, STGMEDIUM& MedSrc, FORMATETC& FmtSrc )
	{
		switch( MedSrc.tymed )
		{
			case TYMED_HGLOBAL:		pMedDest->hGlobal = (HGLOBAL)OleDuplicateData( MedSrc.hGlobal, FmtSrc.cfFormat, NULL );
									break;
			case TYMED_GDI:			pMedDest->hBitmap = (HBITMAP)OleDuplicateData( MedSrc.hBitmap, FmtSrc.cfFormat, NULL);
									break;
			case TYMED_MFPICT:		pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData( MedSrc.hMetaFilePict, FmtSrc.cfFormat, NULL);
									break;
			case TYMED_ENHMF:		pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData( MedSrc.hEnhMetaFile, FmtSrc.cfFormat, NULL);
									break;
			case TYMED_FILE:		pMedDest->lpszFileName = (LPOLESTR)OleDuplicateData( MedSrc.lpszFileName, FmtSrc.cfFormat, NULL);
									break;
			case TYMED_ISTREAM:		pMedDest->pstm = MedSrc.pstm;
									MedSrc.pstm->AddRef();
									break;
			case TYMED_ISTORAGE:	pMedDest->pstg = MedSrc.pstg;
									MedSrc.pstg->AddRef();
									break;
		}
		
		pMedDest->tymed = MedSrc.tymed;
		pMedDest->pUnkForRelease = nullptr;
		
		if ( MedSrc.pUnkForRelease != nullptr )
		{
			pMedDest->pUnkForRelease = MedSrc.pUnkForRelease;
			MedSrc.pUnkForRelease->AddRef();
		}	
	}
	
	virtual BOOL OnRenderData( FORMATETC& /*FormatEtc*/, STGMEDIUM * /*pStgMedium*/, BOOL /*bDropComplete*/ )
	{
		return FALSE;
	}
};

class CDropTarget : public IDropTarget
{
public:	
	CDropTarget( HWND hTargetWnd )
	{
		m_hTargetWnd = hTargetWnd;
		m_nRefCount = 0;
		m_bAllowDrop = FALSE;
		m_pDropTargetHelper = nullptr;
		ZeroMemory( &m_FormatEtc, sizeof( FORMATETC ) );
		ZeroMemory( &m_StgMedium, sizeof( STGMEDIUM ) );
		
		if ( FAILED( CoCreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (LPVOID*)&m_pDropTargetHelper ) ) )
			m_pDropTargetHelper = nullptr;
	}
	
	virtual ~CDropTarget()
	{
		if ( m_pDropTargetHelper != nullptr )
		{
			m_pDropTargetHelper->Release();
			m_pDropTargetHelper = nullptr;
		}
	}

protected:
	HWND m_hTargetWnd;	
	int m_nRefCount;
	struct IDropTargetHelper *m_pDropTargetHelper;
	vector < FORMATETC > m_vFormatEtc;
	BOOL m_bAllowDrop;
	FORMATETC m_FormatEtc;
	STGMEDIUM m_StgMedium;
	
public:
	// IUnknown members
	STDMETHOD(QueryInterface)( REFIID refiid, void FAR* FAR* ppvObject )
	{
		*ppvObject = ( refiid == IID_IUnknown || refiid == IID_IDropTarget ) ? this : nullptr;
		
		if ( *ppvObject != nullptr )
			( (LPUNKNOWN)*ppvObject )->AddRef();
		
		return *ppvObject == nullptr ? E_NOINTERFACE : S_OK;
	}
	
	STDMETHOD_(ULONG, AddRef)( void )
	{
		return ++m_nRefCount;
	}
	
	STDMETHOD_(ULONG, Release)( void )
	{
		int nRefCount = --m_nRefCount;
		if ( nRefCount == 0 )
			delete this;
		return nRefCount;
	}

	STDMETHOD(DragEnter)( IDataObject __RPC_FAR *pDataObject, DWORD dwKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect )
	{
		if ( pDataObject == nullptr )
			return E_INVALIDARG;

		if ( m_pDropTargetHelper != nullptr )
			m_pDropTargetHelper->DragEnter( m_hTargetWnd, pDataObject, (LPPOINT)&pt, *pdwEffect );
		
		ZeroMemory( &m_FormatEtc, sizeof( FORMATETC ) );
		if ( m_StgMedium.tymed != TYMED_NULL )
			ReleaseStgMedium( &m_StgMedium );
		ZeroMemory( &m_StgMedium, sizeof( STGMEDIUM ) );
		
		for ( int nFormatEtc = 0; nFormatEtc < (int)m_vFormatEtc.size(); nFormatEtc++ )
		{
			STGMEDIUM StgMedium;
			m_bAllowDrop = ( pDataObject->GetData( &m_vFormatEtc[ nFormatEtc ], &StgMedium ) == S_OK );
		
			if ( m_bAllowDrop )
			{
				// Store drag data for later use in DragOver
				m_FormatEtc = m_vFormatEtc[ nFormatEtc ];
				m_StgMedium = StgMedium;
				
				// Get client cursor position
				CWindow hWnd( m_hTargetWnd );
				CPoint point( pt.x, pt.y );
				hWnd.ScreenToClient( &point );
					
				*pdwEffect = OnDragEnter( m_FormatEtc, m_StgMedium, dwKeyState, point );
				
				break;
			}
		}
		
		QueryDrop( dwKeyState, pdwEffect );
		
		return S_OK;	
	}
	
	STDMETHOD(DragOver)( DWORD dwKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect )
	{
		if ( m_pDropTargetHelper )
			m_pDropTargetHelper->DragOver( (LPPOINT)&pt, *pdwEffect );
		
		if ( m_bAllowDrop && m_FormatEtc.cfFormat != CF_NULL && m_StgMedium.tymed != TYMED_NULL )
		{
			// Get client cursor position
			CWindow hWnd( m_hTargetWnd );
			CPoint point( pt.x, pt.y );
			hWnd.ScreenToClient( &point );
				
			*pdwEffect = OnDragOver( m_FormatEtc, m_StgMedium, dwKeyState, point );
		}
		
		QueryDrop( dwKeyState, pdwEffect );
		
		return S_OK;
	}
	
	STDMETHOD(DragLeave)( void )
	{
		if ( m_pDropTargetHelper )
			m_pDropTargetHelper->DragLeave();
		
		OnDragLeave();

		m_bAllowDrop = FALSE;
		
		ZeroMemory( &m_FormatEtc, sizeof( FORMATETC ) );
		if ( m_StgMedium.tymed != TYMED_NULL )
			ReleaseStgMedium( &m_StgMedium );
		ZeroMemory( &m_StgMedium, sizeof( STGMEDIUM ) );
		
		return S_OK;
	}
	
	STDMETHOD(Drop)( IDataObject __RPC_FAR *pDataObject, DWORD dwKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect )
    {
		if ( pDataObject == nullptr )
			return E_INVALIDARG;	

		if ( m_pDropTargetHelper )
			m_pDropTargetHelper->Drop( pDataObject, (LPPOINT)&pt, *pdwEffect );

		if ( m_bAllowDrop && m_FormatEtc.cfFormat != CF_NULL && QueryDrop( dwKeyState, pdwEffect ) )
		{
			STGMEDIUM StgMedium;
			if ( pDataObject->GetData( &m_FormatEtc, &StgMedium ) == S_OK )
			{
				// Get client cursor position
				CWindow hWnd( m_hTargetWnd );
				CPoint point( pt.x, pt.y );
				hWnd.ScreenToClient( &point );
				
				if ( !OnDrop( m_FormatEtc, StgMedium, *pdwEffect, point ) )
					*pdwEffect = DROPEFFECT_NONE;
				
				ReleaseStgMedium( &StgMedium );
			}
		}
		
		m_bAllowDrop = FALSE;
		
		ZeroMemory( &m_FormatEtc, sizeof( FORMATETC ) );
		if ( m_StgMedium.tymed != TYMED_NULL )
			ReleaseStgMedium( &m_StgMedium );
		ZeroMemory( &m_StgMedium, sizeof( STGMEDIUM ) );
		
		return S_OK;
    }    
    
    void AddSupportedFormat( FORMATETC& FormatEtc )
	{
		m_vFormatEtc.push_back( FormatEtc );
	}
	
	void AddSupportedFormat( CLIPFORMAT cfFormat )
	{
		FORMATETC FormatEtc;
		ZeroMemory( &FormatEtc, sizeof( FORMATETC ) );
		
		FormatEtc.cfFormat = cfFormat;
		FormatEtc.dwAspect = DVASPECT_CONTENT;
		FormatEtc.lindex = -1;
		FormatEtc.tymed = TYMED_HGLOBAL;
		
		AddSupportedFormat( FormatEtc );
	}
	
	BOOL QueryDrop( DWORD dwKeyState, LPDWORD pdwEffect )
	{
		DWORD dwEffects = *pdwEffect; 

		if ( !m_bAllowDrop )
		{
			*pdwEffect = DROPEFFECT_NONE;
			return FALSE;
		}
		
		*pdwEffect = ( dwKeyState & MK_CONTROL ) ? ( ( dwKeyState & MK_SHIFT ) ? DROPEFFECT_LINK : DROPEFFECT_COPY ) : ( ( dwKeyState & MK_SHIFT ) ? DROPEFFECT_MOVE : 0 );
		if ( *pdwEffect == 0 ) 
		{
			if ( dwEffects & DROPEFFECT_COPY )
				*pdwEffect = DROPEFFECT_COPY;
			else if ( dwEffects & DROPEFFECT_MOVE )
				*pdwEffect = DROPEFFECT_MOVE; 
			else if (dwEffects & DROPEFFECT_LINK )
				*pdwEffect = DROPEFFECT_LINK; 
			else 
				*pdwEffect = DROPEFFECT_NONE;
		} 
		else if ( !( *pdwEffect & dwEffects ) )
			*pdwEffect = DROPEFFECT_NONE;

		return ( *pdwEffect != DROPEFFECT_NONE );
	}

	virtual DWORD OnDragEnter( FORMATETC& /*FormatEtc*/, STGMEDIUM& /*StgMedium*/, DWORD /*dwKeyState*/, CPoint /*point*/ )
	{
		return FALSE;
	}
	
	virtual DWORD OnDragOver( FORMATETC& /*FormatEtc*/, STGMEDIUM& /*StgMedium*/, DWORD /*dwKeyState*/, CPoint /*point*/ )
	{
		return FALSE;
	}
	
	virtual BOOL OnDrop( FORMATETC& /*FormatEtc*/, STGMEDIUM& /*StgMedium*/, DWORD /*dwEffect*/, CPoint /*point*/ )
	{
		return FALSE;
	}
	
	virtual void OnDragLeave()
	{
	}
};

template < class T >
class CDropTargetT : public CDropTarget
{
public:
	CDropTargetT( HWND hTargetWnd ) : CDropTarget( hTargetWnd )
	{
		m_pDelegate = nullptr;
	}

protected:
	T *m_pDelegate;

public:	
	BOOL Register( T *pDelegate )
	{
		m_pDelegate = pDelegate;
		return TRUE;
	}
	
	virtual DWORD OnDragEnter( FORMATETC& FormatEtc, STGMEDIUM& StgMedium, DWORD dwKeyState, CPoint point )
	{
		return m_pDelegate == nullptr ? DROPEFFECT_NONE : m_pDelegate->OnDragEnter( FormatEtc, StgMedium, dwKeyState, point );
	}
	
	virtual DWORD OnDragOver( FORMATETC& FormatEtc, STGMEDIUM& StgMedium, DWORD dwKeyState, CPoint point )
	{
		return m_pDelegate == nullptr ? DROPEFFECT_NONE : m_pDelegate->OnDragOver( FormatEtc, StgMedium, dwKeyState, point );
	}
	
	virtual BOOL OnDrop( FORMATETC& FormatEtc, STGMEDIUM& StgMedium, DWORD dwEffect, CPoint point )
	{
		return m_pDelegate == nullptr ? FALSE : m_pDelegate->OnDrop( FormatEtc, StgMedium, dwEffect, point );
	}
	
	virtual void OnDragLeave()
	{
		if ( m_pDelegate != nullptr )
			m_pDelegate->OnDragLeave();
	}
};

template < class T >
class CDataObjectT : public CDataObject
{
public:
	CDataObjectT( CDropSource *pDropSource ) : CDataObject( pDropSource )
	{
		m_pDelegate = FALSE;
	}

protected:
	T *m_pDelegate;

public:	
	BOOL Register( T *pDelegate )
	{
		m_pDelegate = pDelegate;
		return TRUE;
	}
	
	virtual BOOL OnRenderData( FORMATETC& FormatEtc, STGMEDIUM *pStgMedium, BOOL bDropComplete )
	{
		return m_pDelegate == nullptr ? FALSE : m_pDelegate->OnRenderData( FormatEtc, pStgMedium, bDropComplete );
	}
};

template < class T >
class CDragDrop
{
public:
	CDragDrop()
	{
		m_pDropSource = nullptr;
		m_pDataObject = nullptr;
		m_pDropTarget = nullptr;
		m_hTargetWnd = nullptr;
	}
		
	virtual ~CDragDrop()
	{
		if ( m_pDropSource != nullptr )
			m_pDropSource->Release();
		if ( m_pDataObject != nullptr )
			m_pDataObject->Release();
	}

protected:
	CDropSource *m_pDropSource;
	CDataObjectT< T > *m_pDataObject;
	CDropTargetT< T > *m_pDropTarget;
	HWND m_hTargetWnd;

public:
	BOOL Register( T *pDelegate, BOOL bDropSource = TRUE )
	{
		m_hTargetWnd = pDelegate->m_hWnd;
		
		// Instantiate new drop target object
		m_pDropTarget = new CDropTargetT< T >( m_hTargetWnd );
		m_pDropTarget->Register( pDelegate );
		
		// Register drop target
		if ( FAILED( RegisterDragDrop( m_hTargetWnd, m_pDropTarget ) ) )
		{
			delete m_pDropTarget;
			m_pDropTarget = nullptr;
			return FALSE;
		}
		
		// Is this a drop target only?
		if ( !bDropSource )
			return TRUE;
		
		// Instantiate new drop source object
		m_pDropSource = new CDropSource;
		m_pDropSource->AddRef();
		
		m_pDataObject = new CDataObjectT< T >( m_pDropSource );
		m_pDataObject->AddRef();
		
		// Register drop source delegate for data render
		return m_pDataObject->Register( pDelegate );
	}
	
	BOOL Revoke()
	{
		m_pDropTarget = nullptr;
		return ( RevokeDragDrop( m_hTargetWnd ) == S_OK );
	}
	
	BOOL AddTargetFormat( CLIPFORMAT cfFormat )
	{
		if ( m_pDropTarget == nullptr )
			return FALSE;
		m_pDropTarget->AddSupportedFormat( cfFormat );
		return TRUE;
	}	
	
	BOOL AddSourceFormat( CLIPFORMAT cfFormat )
	{
		if ( m_pDataObject == nullptr )
			return FALSE;
			
		FORMATETC FormatEtc;
		ZeroMemory( &FormatEtc, sizeof( FORMATETC ) );
		
		FormatEtc.cfFormat = cfFormat;
		FormatEtc.dwAspect = DVASPECT_CONTENT;
		FormatEtc.lindex = -1;
		FormatEtc.tymed = TYMED_HGLOBAL;
		
		STGMEDIUM StgMedium;
		ZeroMemory( &StgMedium, sizeof( STGMEDIUM ) );
		
		return SUCCEEDED( m_pDataObject->SetData( &FormatEtc, &StgMedium, TRUE ) );
	}
	
	BOOL SetClipboard( FORMATETC& FormatEtc, STGMEDIUM& StgMedium )
	{
		if ( m_pDataObject == nullptr )
			return DROPEFFECT_NONE;
		
		if ( FAILED( m_pDataObject->SetData( &FormatEtc, &StgMedium, TRUE ) ) )
			return DROPEFFECT_NONE;
		
		return ( OleSetClipboard( m_pDataObject ) == S_OK );
	}
	
	BOOL FlushClipboard()
	{
		return ( OleFlushClipboard() == S_OK );
	}
	
	DWORD DoDragDrop( SHDRAGIMAGE *pDragImage = nullptr, DWORD dwValidEffects = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK )
	{
		if ( m_pDataObject == nullptr )
			return DROPEFFECT_NONE;
			
		IDragSourceHelper *pDragSourceHelper = nullptr;
		
		// Instantiate drag source helper object
		if ( pDragImage != nullptr )
		{
			if ( FAILED( CoCreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (LPVOID*)&pDragSourceHelper ) ) )
				pDragSourceHelper = nullptr;
			
			if ( pDragSourceHelper != nullptr )
				pDragSourceHelper->InitializeFromBitmap( pDragImage, m_pDataObject );
		}
		
		DWORD dwEffects = DROPEFFECT_NONE;
		dwEffects = ::DoDragDrop( m_pDataObject, m_pDropSource, dwValidEffects, &dwEffects ) == DRAGDROP_S_DROP ? DROPEFFECT_NONE : dwEffects;
		
		// Destroy drag source helper object
		if ( pDragSourceHelper != nullptr )
			pDragSourceHelper->Release();
		
		return dwEffects;
	}	
};
