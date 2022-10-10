#pragma once
#if defined(__aarch64__)

class CCodeBlock;

class CAarch64Ops
{
public:
    CAarch64Ops(CCodeBlock & CodeBlock);

private:
    CAarch64Ops(void);
    CAarch64Ops(const CAarch64Ops &);
    CAarch64Ops & operator=(const CAarch64Ops &);

    CCodeBlock & m_CodeBlock;
};

#endif
