#include <stdio.h>

#include "RspProfiling.h"
#pragma warning(disable : 4786)
#include <Common/File.h>
#include <Common/Log.h>
#include <Common/StdString.h>
#include <map>
#include <vector>

class CRspProfiling
{
    typedef std::map<uint32_t, __int64> PROFILE_ENRTIES;
    typedef PROFILE_ENRTIES::iterator PROFILE_ENRTY;
    typedef PROFILE_ENRTIES::value_type PROFILE_VALUE;
    typedef struct
    {
        SPECIAL_TIMERS Timer;
        char * Name;
    } TIMER_NAME;

    uint32_t m_CurrentTimerAddr, CurrentDisplayCount;
    uint32_t m_StartTimeHi, m_StartTimeLo; // The current timer start time
    PROFILE_ENRTIES m_Entries;

public:
    CRspProfiling()
    {
        m_CurrentTimerAddr = Timer_None;
    }

    // Recording timing against current timer, returns the address of the timer stopped
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
#else
        __debugbreak();
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

        __int64 StopTime = ((unsigned __int64)HiValue << 32) + (unsigned __int64)LoValue;
        __int64 StartTime = ((unsigned __int64)m_StartTimeHi << 32) + (unsigned __int64)m_StartTimeLo;
        __int64 TimeTaken = StopTime - StartTime;

        PROFILE_ENRTY Entry = m_Entries.find(m_CurrentTimerAddr);
        if (Entry != m_Entries.end())
        {
            Entry->second += TimeTaken;
        }
        else
        {
            m_Entries.insert(PROFILE_ENRTIES::value_type(m_CurrentTimerAddr, TimeTaken));
        }
#else
        __debugbreak();
#endif

        uint32_t OldTimerAddr = m_CurrentTimerAddr;
        m_CurrentTimerAddr = Timer_None;
        return OldTimerAddr;
    }

    // Reset all the counters back to 0
    void ResetCounters(void)
    {
        m_Entries.clear();
    }

    // Generate a log file with the current results, this will also reset the counters
    void GenerateLog(void)
    {
        stdstr LogFileName;
        {
            CLog Log;
            Log.Open("RSP Profiling.txt");
            LogFileName = Log.FileName();

            // Get the total time
            __int64 TotalTime = 0;
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
                {Timer_Compiling, "RSP: Compiling"},
                {Timer_RSP_Running, "RSP: Running"},
                {Timer_R4300_Running, "R4300i: Running"},
                {Timer_RDP_Running, "RDP: Running"},
            };

            for (size_t count = 0; count < ItemList.size(); count++)
            {
                char Buffer[255];
                float CpuUsage = (float)(((double)ItemList[count]->second / (double)TotalTime) * 100);

                if (CpuUsage <= 0.2)
                {
                    continue;
                }
                sprintf(Buffer, "Function 0x%08X", ItemList[count]->first);
                for (int NameID = 0; NameID < (sizeof(TimerNames) / sizeof(TIMER_NAME)); NameID++)
                {
                    if (ItemList[count]->first == (uint32_t)TimerNames[NameID].Timer)
                    {
                        strcpy(Buffer, TimerNames[NameID].Name);
                        break;
                    }
                }
                Log.LogF("%s\t%2.2f", Buffer, CpuUsage);
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