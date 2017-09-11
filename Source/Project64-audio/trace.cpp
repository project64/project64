/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "trace.h"
#include <Settings/Settings.h>
#include <Common/path.h>
#include <Common/LogClass.h>
#include "AudioSettings.h"

#ifdef ANDROID
#include <android/log.h>

class AndroidLogger : public CTraceModule
{
    void Write(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message)
    {
        switch (severity)
        {
        case TraceError: __android_log_print(ANDROID_LOG_ERROR, TraceModule(module), "%s: %s", function, Message); break;
        case TraceWarning:  __android_log_print(ANDROID_LOG_WARN, TraceModule(module), "%s: %s", function, Message); break;
        case TraceNotice: __android_log_print(ANDROID_LOG_INFO, TraceModule(module), "%s: %s", function, Message); break;
        case TraceInfo: __android_log_print(ANDROID_LOG_INFO, TraceModule(module), "%s: %s", function, Message); break;
        case TraceDebug: __android_log_print(ANDROID_LOG_DEBUG, TraceModule(module), "%s: %s", function, Message); break;
        case TraceVerbose: __android_log_print(ANDROID_LOG_VERBOSE, TraceModule(module), "%s: %s", function, Message); break;
        default: __android_log_print(ANDROID_LOG_UNKNOWN, TraceModule(module), "%s: %s", function, Message); break;
        }
    }
    void FlushTrace(void)
    {
    }
};
static AndroidLogger * g_AndroidLogger = NULL;
#endif
static CTraceFileLog * g_LogFile = NULL;

void SetupTrace(void)
{
    if (g_LogFile != NULL)
    {
        return;
    }

#ifdef ANDROID
    if (g_AndroidLogger == NULL)
    {
        g_AndroidLogger = new AndroidLogger();
    }
    TraceAddModule(g_AndroidLogger);
#endif
#ifdef _DEBUG
    TraceSetMaxModule(MaxTraceModulePluginAudio, TraceInfo);
#else
    TraceSetMaxModule(MaxTraceModulePluginAudio, TraceError);
#endif
    TraceSetModuleName(TraceAudioInitShutdown, "AudioInitShutdown");
    TraceSetModuleName(TraceAudioInterface, "AudioInterface");
    TraceSetModuleName(TraceAudioDriver, "AudioDriver");
}

void StartTrace(void)
{
    char log_dir[260];
    memset(log_dir, 0, sizeof(log_dir));
    short logDirSetting = FindSystemSettingId("Dir:Log");
    short logFlushSetting = FindSystemSettingId("Log Auto Flush");
    if (logDirSetting != 0)
    {
        GetSystemSettingSz(logDirSetting, log_dir, sizeof(log_dir));
    }

    if (strlen(log_dir) == 0)
    {
        return;
    }

    CPath LogFilePath(log_dir, "Project64-audio.log");
    if (!LogFilePath.DirectoryExists())
    {
        LogFilePath.DirectoryCreate();
    }
    g_LogFile = new CTraceFileLog(LogFilePath, GetSystemSetting(logFlushSetting) != 0, CLog::Log_New, 500);
    TraceAddModule(g_LogFile);
}