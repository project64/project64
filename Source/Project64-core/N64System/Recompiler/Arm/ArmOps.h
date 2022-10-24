#pragma once
#if defined(__arm__) || defined(_M_ARM)

class CCodeBlock;

class CArmOps
{
public:
    CArmOps(CCodeBlock & CodeBlock);

private:
    CArmOps(void);
    CArmOps(const CArmOps &);
    CArmOps & operator=(const CArmOps &);

    CCodeBlock & m_CodeBlock;
};

#endif
