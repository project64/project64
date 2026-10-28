#include <stdio.h>

#include "RspProfiling.h"
#pragma warning(disable : 4786)
#include <Common/File.h>
#include <Common/Log.h>
#include <Common/StdString.h>
#include <Common/path.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Settings/Settings.h>
#include <chrono>
#include <intrin.h>
#include <map>
#include <thread>
#include <vector>
#ifdef WIN32
#include <Windows.h>
#endif

class CRspProfiling
{
    typedef std::map<uint32_t, int64_t> PROFILE_ENRTIES;
    typedef PROFILE_ENRTIES::iterator PROFILE_ENRTY;
    typedef PROFILE_ENRTIES::value_type PROFILE_VALUE;
    typedef struct
    {
        SPECIAL_TIMERS Timer;
        char * Name;
    } TIMER_NAME;

    uint32_t m_CurrentTimerAddr, CurrentDisplayCount;
#if defined(_M_IX86) && defined(_MSC_VER)
    uint32_t m_StartTimeHi, m_StartTimeLo; // The current timer start time
#else
    uint64_t m_StartTime;
#endif
    PROFILE_ENRTIES m_Entries;
    double m_CpuFrequencyGHz;

public:
    CRspProfiling()
    {
        m_CurrentTimerAddr = Timer_None;
#ifdef WIN32
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        m_CpuFrequencyGHz = MeasureCpuFrequencyGHz();
#else
        g_Notify->BreakPoint(__FILE__, __LINE__);
        m_CpuFrequencyGHz = 0;
#endif
    }

    uint32_t StartTimer(uint32_t Address)
    {
        uint32_t OldTimerAddr = StopTimer();
        m_CurrentTimerAddr = Address;

#if defined(_M_IX86) && defined(_MSC_VER)
        uint32_t HiValue, LoValue;
        _asm {
			pushad
			rdtsc
			mov HiValue, edx
			mov LoValue, eax
			popad
        }
        m_StartTimeHi = HiValue;
        m_StartTimeLo = LoValue;
#elif defined(_M_X64) || defined(__amd64__)
        _mm_lfence();
        m_StartTime = __rdtsc();
        _mm_lfence();
#else
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        m_StartTime = (uint64_t)counter.QuadPart;
#endif
        return OldTimerAddr;
    }
    uint32_t StopTimer(void)
    {
        if (m_CurrentTimerAddr == Timer_None)
        {
            return m_CurrentTimerAddr;
        }

#if defined(_M_IX86) && defined(_MSC_VER)
        uint32_t HiValue, LoValue;
        _asm {
			pushad
			rdtsc
			mov HiValue, edx
			mov LoValue, eax
			popad
        }

        int64_t StopTime = ((uint64_t)HiValue << 32) + (uint64_t)LoValue;
        int64_t StartTime = ((uint64_t)m_StartTimeHi << 32) + (uint64_t)m_StartTimeLo;
        int64_t TimeTaken = StopTime - StartTime;
#elif defined(_M_X64) || defined(__amd64__)
        _mm_lfence();
        uint64_t currentTime = __rdtsc();
        _mm_lfence();
        int64_t TimeTaken = currentTime - m_StartTime;
#else
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        int64_t TimeTaken = (int64_t)((uint64_t)counter.QuadPart - m_StartTime);
#endif
        PROFILE_ENRTY Entry = m_Entries.find(m_CurrentTimerAddr);
        if (Entry != m_Entries.end())
        {
            Entry->second += TimeTaken;
        }
        else
        {
            m_Entries.insert(PROFILE_ENRTIES::value_type(m_CurrentTimerAddr, TimeTaken));
        }

        uint32_t OldTimerAddr = m_CurrentTimerAddr;
        m_CurrentTimerAddr = Timer_None;
        return OldTimerAddr;
    }

    void ResetCounters(void)
    {
        m_Entries.clear();
    }

    double ConvertCyclesToMilliseconds(uint64_t cycles, double cpuFreqGHz)
    {
        return (cycles / (cpuFreqGHz * 1e9)) * 1000.0;
    }

    double MeasureCpuFrequencyGHz()
    {
        DWORD_PTR oldMask = SetThreadAffinityMask(GetCurrentThread(), 1);
        auto start_time = std::chrono::high_resolution_clock::now();
#if defined(_M_ARM64) || defined(__aarch64__)
        LARGE_INTEGER startCounter, endCounter;
        QueryPerformanceCounter(&startCounter);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        QueryPerformanceCounter(&endCounter);
        uint64_t start_cycles = (uint64_t)startCounter.QuadPart;
        uint64_t end_cycles = (uint64_t)endCounter.QuadPart;
#else
        uint64_t start_cycles = __rdtsc();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        uint64_t end_cycles = __rdtsc();
#endif
        auto end_time = std::chrono::high_resolution_clock::now();
        SetThreadAffinityMask(GetCurrentThread(), oldMask);
        double elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        uint64_t elapsed_cycles = end_cycles - start_cycles;
        return static_cast<double>(elapsed_cycles) / (elapsed_seconds * 1e9);
    }
    void GenerateLog(void)
    {
        stdstr LogFileName;
        {
            char LogDir[260];
            CPath LogFilePath(GetSystemSettingSz(Set_DirectoryLog, LogDir, sizeof(LogDir)), "RSP_Profiling.txt");

            CLog Log;
            Log.Open(LogFilePath);
            LogFileName = Log.FileName();

            // Get the total time
            int64_t TotalTime = 0;
            for (PROFILE_ENRTY itemTime = m_Entries.begin(); itemTime != m_Entries.end(); itemTime++)
            {
                TotalTime += itemTime->second;
            }

            // Create a sortable list of items
            std::vector<PROFILE_VALUE *> ItemList;
            for (PROFILE_ENRTY Entry = m_Entries.begin(); Entry != m_Entries.end(); Entry++)
            {
                ItemList.push_back(&(*Entry));
            }

            // Sort the list with a basic bubble sort
            if (ItemList.size() > 0)
            {
                for (size_t OuterPass = 0; OuterPass < (ItemList.size() - 1); OuterPass++)
                {
                    for (size_t InnerPass = 0; InnerPass < (ItemList.size() - 1); InnerPass++)
                    {
                        if (ItemList[InnerPass]->second < ItemList[InnerPass + 1]->second)
                        {
                            PROFILE_VALUE * TempPtr = ItemList[InnerPass];
                            ItemList[InnerPass] = ItemList[InnerPass + 1];
                            ItemList[InnerPass + 1] = TempPtr;
                        }
                    }
                }
            }

            TIMER_NAME TimerNames[] = {
                {Timer_Compiling, "Compiling"},
                {Timer_RSP_Running, "RSP: Running"},
                {Timer_RDP_Running, "RDP: Running"},
            };

            for (size_t i = 0; i < ItemList.size(); i++)
            {
                float CpuUsage = (float)(((double)ItemList[i]->second / (double)TotalTime) * 100);

                stdstr_f name("Function 0x%08X", ItemList[i]->first);
                for (int NameID = 0; NameID < (sizeof(TimerNames) / sizeof(TIMER_NAME)); NameID++)
                {
                    if (ItemList[i]->first == (uint32_t)TimerNames[NameID].Timer)
                    {
                        name = TimerNames[NameID].Name;
                        break;
                    }
                }
                Log.LogF("%s\t%2.2f\t%llu\t%2.2f\n", name.c_str(), CpuUsage, ItemList[i]->second, ConvertCyclesToMilliseconds(ItemList[i]->second, m_CpuFrequencyGHz));
            }
        }
        ResetCounters();
    }
};

CRspProfiling & GetProfiler(void)
{
    static CRspProfiling Profile;
    return Profile;
}

void ResetTimerList(void)
{
    GetProfiler().ResetCounters();
}

uint32_t StartTimer(uint32_t Address)
{
    return GetProfiler().StartTimer(Address);
}

void StopTimer(void)
{
    GetProfiler().StopTimer();
}

void GenerateTimerResults(void)
{
    GetProfiler().GenerateLog();
}