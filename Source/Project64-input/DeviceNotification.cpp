#include "DeviceNotification.h"
#include "CProject64Input.h"
#include <dbt.h>

DeviceNotification::DeviceNotification()
{
    Create(nullptr);
}

DeviceNotification::~DeviceNotification()
{
    DestroyWindow();
}

int DeviceNotification::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter = { 0 };
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_size = sizeof(notificationFilter);

    HDEVNOTIFY hDevNotify;
    hDevNotify = RegisterDeviceNotification(m_hWnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    return TRUE;
}

BOOL DeviceNotification::OnDeviceChange(UINT nEventType, DWORD /*dwData*/)
{
    if (g_InputPlugin != nullptr && (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE))
    {
        g_InputPlugin->DevicesChanged();
    }
    return TRUE;
}
