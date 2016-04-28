#include <iostream>
#include <assert.h>
#include <Common/path.h>
#include <Common/Util.h>
#include <Common/Thread.h>
#include <Common/Trace.h>
#include <Common/TraceModulesCommon.h>
#include <Common/SyncEvent.h>
#include <Common/CriticalSection.h>
#include <Common/MemoryManagement.h>


#include <thread>
#include <chrono>
#include <unistd.h>
#include <exception>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>


void FindFiles(CPath& Dir)
{
    Dir.SetNameExtension("*");
    if(Dir.FindFirst(CPath::FIND_ATTRIBUTE_SUBDIR))
    {
        do
        {
            FindFiles(Dir);
        } while (Dir.FindNext());
        Dir.UpDirectory();
    }
    
    Dir.SetNameExtension("*.a");
    if (Dir.FindFirst())
    {
        do{
            std::cout << Dir.GetDriveDirectory() + Dir.GetNameExtension() << std::endl;
        }while(Dir.FindNext());
    }
}

CriticalSection cs;
SyncEvent cv(false);
std::atomic_int gNumber(0);
uint32_t gThreadParameter = 8;
uint32_t gParentThreadSleepFor = 3;

uint32_t Job_1(void * lpThreadParameter)
{
    cs.enter();
    std::cout << "Thread (" << CThread::GetCurrentThreadId() <<"): Entering function Job_1!\n";
    cs.leave();

    cs.enter();
    if(*(int*)lpThreadParameter != gThreadParameter)
    {
    	std::cout << "Thread (" << CThread::GetCurrentThreadId() << "): Got wrong parameter! Expecting " << gThreadParameter <<" got: " << *(int*)lpThreadParameter << std::endl;
	exit(1);
    }
    else
        std::cout << "Thread (" << CThread::GetCurrentThreadId() << "): Got expected parameter: " << *(int*)lpThreadParameter << std::endl;
    cs.leave();

    while (true)
    {
        cv.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
        cs.enter();
        std::cout << "Thread (" << CThread::GetCurrentThreadId() << "): Got new number " <<  std::dec << gNumber << std::endl;
        cs.leave();
    }
    
    cs.enter();
    std::cout << "Thread (" << CThread::GetCurrentThreadId() <<"): Leaving function Job_1!\n";
    cs.leave();
    return 0;
}


uint32_t Job_2(void * lpThreadParameter)
{
    cs.enter();
    std::cout << "Thread (" << CThread::GetCurrentThreadId() <<"): Entering function Job_2!\n";
    cs.leave();   
    
    while(1) 
    {
        gNumber = rand() % 1024 + 5;
        cv.Trigger();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(150));        
    }
    
    std::cout << "Thread (" << CThread::GetCurrentThreadId() << "): Leaving function Job_2!\n";    
    return 0;
}

int main(int argc, const char * argv[])
 {
     // Setup Trace Module
     std::cout << "[ Adding TraceModule ]\n";
     CTraceFileLog logFile("test.log", false, CLog::Log_New);
     TraceAddModule(&logFile);
     TraceSetMaxModule(6, TraceDebug);
     TraceSetModuleName(TraceThread, "CThread");
     
     std::cout << "\n[ Test 1: Testing class CThread, CriticalSection and SyncEvent ]\n";
     {
         CThread thread1(&Job_1);
         CThread thread2(&Job_2);
         CThread thread3(&Job_2);
         CThread thread4(&Job_2);
         
         thread1.Start((void*)&gThreadParameter);
         thread2.Start(NULL);
         thread3.Start(NULL);
         thread4.Start(NULL);
         
         uint32_t currenThreadID = CThread::GetCurrentThreadId();
         if(currenThreadID == thread1.ThreadID())
         {
             cs.enter();
             std::cout <<" Error: thread 1's ID == parent ID!\n";
             cs.leave();
             exit(1);
         }
         else if(currenThreadID == thread2.ThreadID())
         {
             cs.enter();
             std::cout <<" Error: thread 2's ID== parent ID!\n";
             cs.leave();
             exit(1);
         }
         else if(currenThreadID == thread3.ThreadID())
         {
             cs.enter();
             std::cout <<" Error: thread 3's ID== parent ID!\n";
             cs.leave();
             exit(1);
         }
         else if(currenThreadID == thread4.ThreadID())
         {
             cs.enter();
             std::cout <<" Error: thread 4's ID== parent ID!\n";
             cs.leave();
             exit(1);
         }
         
         // Test if all child threads are running
         if(!thread1.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 1 is NOT running!\n";
             cs.leave();
             exit(1);
         }
         else if(!thread2.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 2 is NOT running!\n";
             cs.leave();
             exit(1);
         }
         else if(!thread3.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 3 is NOT running!\n";
             cs.leave();
             exit(1);
         }
         else if(!thread4.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 3 is NOT running!\n";
             cs.leave();
             exit(1);
         }
         
         // Sleep
         cs.enter();
         std::cout << "Parent Thread (" << CThread::GetCurrentThreadId() <<"): Going into sleep for " << std::dec << gParentThreadSleepFor << "s!\n";
         cs.leave();
         
         std::this_thread::sleep_for(std::chrono::seconds(gParentThreadSleepFor));
         
         // Kill threads
         cs.enter();
         std::cout << "Parent Thread (" << CThread::GetCurrentThreadId() <<"): Woke up, going to terminate child threads!\n";
         cs.leave();
         
         thread1.Terminate();
         thread2.Terminate();
         thread3.Terminate();
         thread4.Terminate();
         
         // Test if all child threads are dead
         if(thread1.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 1 is still running, should be dead by now!\n";
             cs.leave();
             exit(1);
         }
         else if(thread2.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 2 is still running, should be dead by now!\n";
             cs.leave();
             exit(1);
         }
         else if(thread3.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 3 is still running, should be dead by now!\n";
             cs.leave();
             exit(1);
         }
         else if(thread4.isRunning())
         {
             cs.enter();
             std::cout <<" Error: thread 4 is still running, should be dead by now!\n";
             cs.leave();
             exit(1);
         }
     }// Test1
     
     
     // Test 2
     std::cout << "\n[ Test 2: CPath ]\n";
     {
         CPath path(CPath::CURRENT_DIRECTORY);
         std::cout << "Current Working Directory: " << (const char*)path << std::endl;
         
         std::cout << "Creating new dir: TestDir" << std::endl;
         path.AppendDirectory("TestDir");
         if(!path.DirectoryCreate(true))
         {
             std::cout << "Error: Creating dir: TestDir failed!";
             exit(1);
         }
         
         std::cout << "Changing working dir to new directory" << std::endl;
         path.ChangeDirectory();
         if(CPath(CPath::CURRENT_DIRECTORY) != path)
         {
             std::cout << "Error: Changing working dir failed!" << std::endl;
             exit(1);
         }
         
         if(path.IsRelative())
         {
             std::cout << "Error: Not Absolute path!";
             exit(1);
         }
         
         if(!path.IsDirectory())
         {
             std::cout << "Error: Path is not Directory!";
             exit(1);
         }
         
     }// Test 2
     



//    // Test Search file in current dir
//    CPath path;
//    //path.CurrentDirectory();
//    bool isTest =  path.IsDirectory();
//    isTest = path.DirectoryExists();
//    
//    path.SetNameExtension("libzlib.a");
//    isTest = path.DirectoryExists();
//    isTest =  path.IsDirectory();
//    isTest =  path.IsFile();
//    path.AppendDirectory("testDir4/testDir4_1/testDir4_1_1");
//    path.DirectoryCreate();
//    
//    path.SetDirectory("");
//    path.SetNameExtension("");
//    path.SetName("libzlib.a copy");
//    path.Delete();
//    
//    path.SetNameExtension("libzlib.a");
//    path.CopyTo("copy_libzlib.a");
//    
//    path.SetNameExtension("copy_libzlib.a");
//    path.MoveTo("copy_libzlib.a.bak");
//    
//    
//    
////    FindFiles(path);
////    
////    path.Exit();
//    
//    // Test pjutil && CPath module
//    path.SetNameExtension("test_mupen64plus-input-sdl.dylib");
//    auto lib = pjutil::DynLibOpen((const char*)path);
//    
//    CPath path2;
//    path2.Module(pjutil::DynLibGetProc(lib, "ControllerCommand"));
//    
//    path2.IsRelative();
    
    // insert code here...

    return 0;
}
