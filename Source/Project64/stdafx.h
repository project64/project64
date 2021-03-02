#pragma once

#pragma warning(disable:4247)
#pragma warning(disable:4786)

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Common/MemTest.h>
#include <Common/CriticalSection.h>
#include <Project64-core/Multilanguage.h>
#include "UserInterface.h"
#include "N64System.h"
#include <Project64-core/Plugin.h>
#include "Support.h"
#include <Project64-core/Version.h>
#include <windows.h>
#include <mmsystem.h>
#include <Aclapi.h>

#include <Project64-core/3rdParty/Zip.h>
#include <Project64-core/3rdParty/7zip.h>
