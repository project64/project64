class CJumpInfo
{
public:
	CJumpInfo();

	DWORD		TargetPC;
	char *		BranchLabel;
	DWORD *		LinkLocation;
	DWORD *		LinkLocation2;	
	BOOL		FallThrough;	
	BOOL		PermLoop;
	BOOL		DoneDelaySlot;
	CRegInfo	RegSet;
};
