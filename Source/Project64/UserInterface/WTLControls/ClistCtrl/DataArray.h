#pragma once

#include <atldef.h>

#ifdef malloc
#pragma push_macro("malloc")
#undef malloc
#define pop_malloc
#endif

#ifdef realloc
#pragma push_macro("realloc")
#undef realloc
#define pop_realloc
#endif

#ifdef free
#pragma push_macro("free")
#undef free
#define pop_free
#endif

#ifdef new
#pragma push_macro("new")
#undef new
#define pop_new
#endif

// template class helpers with functions for comparing elements
// override if using complex types without operator==

template <class T>
class CListCtrlArrayEqualHelper
{
public:
	static bool IsEqual(const T& t1, const T& t2)
	{
		return (t1 == t2);
	}
};

template <class T, class TEqual = CListCtrlArrayEqualHelper< T > >
class CListCtrlArray
{
public:
	// Construction/destruction
	CListCtrlArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{ }

	~CListCtrlArray()
	{
		RemoveAll();
	}

	CListCtrlArray(const CListCtrlArray< T, TEqual >& src) : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{
		m_aT = (T*)malloc(src.GetSize() * sizeof(T));
		if (m_aT != NULL)
		{
			m_nAllocSize = src.GetSize();
			for (int i=0; i<src.GetSize(); i++)
				Add(src[i]);
		}
	}
	CListCtrlArray< T, TEqual >& operator=(const CListCtrlArray< T, TEqual >& src)
	{
		if (GetSize() != src.GetSize())
		{
			RemoveAll();
			m_aT = (T*)malloc(src.GetSize() * sizeof(T));
			if (m_aT != NULL)
				m_nAllocSize = src.GetSize();
		}
		else
		{
			for (int i = GetSize(); i > 0; i--)
				RemoveAt(i - 1);
		}
		for (int i=0; i<src.GetSize(); i++)
			Add(src[i]);
		return *this;
	}

	// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(const T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		InternalSetAtIndex(m_nSize, t);
		m_nSize++;
		return TRUE;
	}
	BOOL AddAt(const T& t, int index)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		int NewItem = (m_nSize - index);
		if (NewItem > 0)
		{
			memmove(&m_aT[index + 1],&m_aT[index],NewItem * sizeof(T));
		}
		InternalSetAtIndex(index, t);
		m_nSize++;
		return TRUE;
	}
	BOOL Remove(const T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}
	BOOL RemoveAt(int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if (nIndex < 0 || nIndex >= m_nSize)
			return FALSE;
		m_aT[nIndex].~T();
		if(nIndex != (m_nSize - 1))
			memmove((void*)(m_aT + nIndex), (void*)(m_aT + nIndex + 1), (m_nSize - (nIndex + 1)) * sizeof(T));
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_aT != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
				m_aT[i].~T();
			free(m_aT);
			m_aT = NULL;
		}
		m_nSize = 0;
		m_nAllocSize = 0;
	}
	const T& operator[] (int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			RaiseException(EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_NONCONTINUABLE, 0, NULL);					
		}
		return m_aT[nIndex];
	}
	T& operator[] (int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			RaiseException(EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_NONCONTINUABLE, 0, NULL);					
		}
		return m_aT[nIndex];
	}
	T* GetData() const
	{
		return m_aT;
	}

	int Find(const T& t) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(TEqual::IsEqual(m_aT[i], t))
				return i;
		}
		return -1;  // Not found
	}

	BOOL SetAtIndex(int nIndex, const T& t)
	{
		if (nIndex < 0 || nIndex >= m_nSize)
			return FALSE;
		InternalSetAtIndex(nIndex, t);
		return TRUE;
	}

	// Implementation
	class Wrapper
	{
	public:
		Wrapper(const T& _t) : t(_t)
		{
		}
		template <class _Ty>
			void * __cdecl operator new(size_t, _Ty* p)
		{
			return p;
		}
		template <class _Ty>
			void __cdecl operator delete(void* /* pv */, _Ty* /* p */)
		{
		}
		T t;
	};

	// Implementation
	void InternalSetAtIndex(int nIndex, const T& t)
	{
		new(m_aT + nIndex) Wrapper(t);
	}

	typedef T _ArrayElementType;
	T* m_aT;
	int m_nSize;
	int m_nAllocSize;

};

#ifdef pop_new
#pragma pop_macro("new")
#endif

#ifdef pop_free
#pragma pop_macro("free")
#endif

#ifdef pop_realloc
#pragma pop_macro("realloc")
#endif

#ifdef pop_malloc
#pragma pop_macro("malloc")
#endif
