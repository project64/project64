#pragma once
#include "ControllerSpec1.1.h"
#include "DirectInput.h"
#include "N64Controller.h"
#include <Common/CriticalSection.h>
#include <memory>

class CProject64Input
{
public:
    CProject64Input(HINSTANCE hinst);
    ~CProject64Input();

    void DevicesChanged(void);
    void DeviceAdded(void);
    void InitiateControllers(CONTROL_INFO * ControlInfo);
    void GetKeys(int32_t Control, BUTTONS * Keys);
    void StartScanDevices(int32_t DisplayCtrlId);
    void EndScanDevices(void);
    CDirectInput::ScanResult ScanDevices(BUTTON & Button);
    std::wstring ButtonAssignment(BUTTON & Button);
    std::wstring ControllerDevices(const N64CONTROLLER & Controller);
    bool SaveController(uint32_t ControlIndex);
    bool ResetController(uint32_t ControlIndex, CONTROL & ControlInfo, N64CONTROLLER & Controller);

    inline HINSTANCE hInst(void) const { return m_hinst; }
    inline bool IsScanning(void) const { return m_Scanning; }
    inline int32_t DisplayCtrlId(void) const { return m_DisplayCtrlId; }
    inline N64CONTROLLER & Controllers(int32_t Controller) { return m_Controllers[Controller]; }
    inline CONTROL & ControlInfo(int32_t Controller) { return m_ControlInfo.Controls[Controller]; }

private:
    CProject64Input();
    CProject64Input(const CProject64Input&);
    CProject64Input& operator=(const CProject64Input&);

    CriticalSection m_CS;
    CONTROL_INFO m_ControlInfo;
    N64CONTROLLER m_Controllers[4];
    std::unique_ptr<CDirectInput> m_DirectInput;
    HINSTANCE m_hinst;
    bool m_Scanning;
    int32_t m_DisplayCtrlId;
    int32_t m_iFirstController;
};

extern CProject64Input * g_InputPlugin;
