#pragma once

class CriticalSection
{
public:
    CriticalSection();
    ~CriticalSection(void);

    void enter(void);
    void leave(void);

private:
    CriticalSection(const CriticalSection &);
    CriticalSection & operator=(const CriticalSection &);

    void * m_cs;
};

class CGuard
{
public:
    CGuard(CriticalSection & sectionName) :
        m_cs(sectionName)
    {
        m_cs.enter();
    }
    ~CGuard()
    {
        m_cs.leave();
    }

private:
    CriticalSection & m_cs;
    CGuard(const CGuard & copy);
    CGuard & operator=(const CGuard & rhs);
};

class CUniqueLock
{
public:
    CUniqueLock(CriticalSection & sectionName) :
        m_cs(sectionName), m_locked(true)
    {
        m_cs.enter();
    }
    ~CUniqueLock()
    {
        if (m_locked)
            m_cs.leave();
    }

    inline void lock()
    {
        if (!m_locked)
        {
            m_cs.enter();
            m_locked = true;
        }
    }

    inline void unlock()
    {
        if (m_locked)
        {
            m_cs.leave();
            m_locked = false;
        }
    }

private:
    CriticalSection & m_cs;
    bool m_locked;
    CUniqueLock(const CUniqueLock & copy);
    CUniqueLock & operator=(const CUniqueLock & rhs);
};
