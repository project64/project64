#pragma once
#include "wtl.h"

typedef CWinTraits<WS_OVERLAPPED, WS_EX_APPWINDOW> DeviceNotificationTraits;

class DeviceNotification :
    public CWindowImpl<DeviceNotification, CWindow, DeviceNotificationTraits>
{
public:
    DECLARE_WND_CLASS(_T("My Window Class"))

    BEGIN_MSG_MAP(DeviceNotification)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DEVICECHANGE(OnDeviceChange)
    END_MSG_MAP()

    DeviceNotification();
    ~DeviceNotification();

private:
    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
};
