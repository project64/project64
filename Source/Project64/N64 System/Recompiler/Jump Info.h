class CJumpInfo
{
public:
	CJumpInfo();

	DWORD		TargetPC;
	DWORD		JumpPC;
	stdstr		BranchLabel;
	DWORD *		LinkLocation;
	DWORD *		LinkLocation2;	
	bool		FallThrough;	
	bool		PermLoop;
	bool		DoneDelaySlot;  //maybe deletable
	CRegInfo	RegSet;
};
