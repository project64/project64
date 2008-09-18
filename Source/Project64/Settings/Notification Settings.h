class CNotificationSettings
{
	static void InFullScreenChanged (CNotificationSettings * _this);

protected:
	CNotificationSettings();
	virtual ~CNotificationSettings();

	//Settings that can be changed on the fly
	static bool bInFullScreen;
};
