class CCodeSection;


class CCodeBlock;


typedef struct {
	CCodeSection * Parent;
	CJumpInfo     * JumpInfo;
} BLOCK_PARENT;

typedef std::vector<BLOCK_PARENT> BLOCK_PARENT_LIST;
