#pragma once
#if defined(__amd64__) || defined(_M_X64)

class CCodeBlock;

class CX64Ops
{
public:
    CX64Ops(CCodeBlock & CodeBlock);

private:
    CX64Ops(void);
    CX64Ops(const CX64Ops&);
    CX64Ops& operator=(const CX64Ops&);

    CCodeBlock & m_CodeBlock;
};

#endif
