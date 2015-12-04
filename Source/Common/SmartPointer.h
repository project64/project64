#ifndef __SMART_POINTER__H__
#define __SMART_POINTER__H__

//The template class definition for smart pointer
template<class _Ty>
class AUTO_PTR 
{
public:
    typedef _Ty element_type;
    //ctor
    explicit AUTO_PTR(_Ty *pVal = 0) throw() :
		m_Owns(pVal != 0),
		m_AutoPtr(pVal)

    {
    }

	//copy ctor
    AUTO_PTR(const AUTO_PTR<_Ty>& ptrCopy) throw() :
		m_Owns(ptrCopy.m_Owns), 
		m_AutoPtr(ptrCopy.release())
    {
    }

	//overloading = operator
    AUTO_PTR<_Ty>& operator=(AUTO_PTR<_Ty>& ptrCopy) throw()
    {
		if (this != &ptrCopy)
		{
			if (m_AutoPtr != ptrCopy.get()) 
			{
				if (m_Owns)
				{
					delete m_AutoPtr;
				}
				m_Owns = ptrCopy.m_Owns; 
			}
			else if (ptrCopy.m_Owns)
			{
				m_Owns = true;
			}
			m_AutoPtr = ptrCopy.release(); 
		}
		return (*this); 
    }
    //dtor
    ~AUTO_PTR()
    {
		if (m_Owns)
		{
			delete m_AutoPtr; 
		}
    }
    //overloading * operator
	_Ty& operator*() const throw()
	{
		return (*get()); 
	}

    //overloading -> operator
	_Ty *operator->() const throw()
	{
		return (get()); 
	}

    //function to get the pointer to the class
	_Ty *get() const throw()
	{
		return (m_AutoPtr); 
	}
	
    //function to get the pointer to the class and take ownership
	_Ty *release() const throw()
	{
		((AUTO_PTR<_Ty> *)this)->m_Owns = false;
		return (m_AutoPtr); 
	}

	void reset( _Ty *pVal = 0 ) throw()
	{
		if (m_AutoPtr != pVal) 
		{
			if (m_Owns)
			{
				delete m_AutoPtr;
			}
			m_AutoPtr = pVal;
			m_Owns = pVal != NULL;
		}
	}

private:
    _Ty * m_AutoPtr;
	bool  m_Owns;
};

#endif
