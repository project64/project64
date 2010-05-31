class CJumpInfo
{
public:
	CJumpInfo();

	DWORD		TargetPC;
	stdstr		BranchLabel;
	DWORD *		LinkLocation;
	DWORD *		LinkLocation2;	
	BOOL		FallThrough;	
	BOOL		PermLoop;
	BOOL		DoneDelaySlot;
	CRegInfo	RegSet;
};
