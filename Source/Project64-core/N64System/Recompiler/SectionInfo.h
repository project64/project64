#pragma once

class CCodeSection;
class CCodeBlock;
struct CJumpInfo;

struct BLOCK_PARENT
{
    CCodeSection * Parent;
    CJumpInfo * JumpInfo;
};

typedef std::vector<BLOCK_PARENT> BLOCK_PARENT_LIST;
