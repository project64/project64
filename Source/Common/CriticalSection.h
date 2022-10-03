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
