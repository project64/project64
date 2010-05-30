class CCodeSection;
typedef std::list<CCodeSection *> SECTION_LIST;


class CCodeBlock;


typedef struct {
	CCodeSection * Parent;
	CJumpInfo     * JumpInfo;
} BLOCK_PARENT;

typedef std::vector<BLOCK_PARENT> BLOCK_PARENT_LIST;
