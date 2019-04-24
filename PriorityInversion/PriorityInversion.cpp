#include <iostream>
#include <Windows.h>
#include <thread>

CRITICAL_SECTION g_cs;

DWORD WINAPI LoadSlowDll(LPVOID param) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
    SetEvent(static_cast<HANDLE>(param));
    LoadLibrary(L"SpinInDllMain.dll");
    return 0;
}

DWORD WINAPI LockThenSpin(LPVOID param) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
    EnterCriticalSection(&g_cs);
    SetEvent(static_cast<HANDLE>(param));

    // Run for "a while" (make this cpu cycle based instead
    // of time based in case we're not scheduled very frequently.
    ULONGLONG start = 0;
    QueryThreadCycleTime(GetCurrentThread(), &start);
    ULONGLONG current = start;
    while (current - start < MAXULONG32) {
        QueryThreadCycleTime(GetCurrentThread(), &current);
    }

    LeaveCriticalSection(&g_cs);
    
    return 0;
}

DWORD WINAPI SpinForever(LPVOID param) {
    while (true);
    return 0;
}

int main(int argc, char *argv[])
{
    bool loader_lock = false;
    if (argc > 1) {
        if (strcmp(argv[1], "-loaderlock") == 0) {
            loader_lock = true;
        }
    }
    InitializeCriticalSection(&g_cs);
    
    // Put some pressure on the system scheduler by spinning 2x the number
    // of threads as there are logical cores.
    unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < 2 * concurentThreadsSupported; i++) {
        HANDLE spin_thread = ::CreateThread(nullptr, 0, &SpinForever, 0, 0, nullptr);
        CloseHandle(spin_thread);
    }

    LPTHREAD_START_ROUTINE proc = loader_lock ? &LoadSlowDll : &LockThenSpin;
    // Create a thread that will load the DLL that spins with the loader lock
    // held. That thread will run at background priority.
    // Then try to acquire the loader lock with this normal priority thread.
    HANDLE load_started = CreateEvent(nullptr, FALSE, FALSE, L"StartLoad");
    HANDLE loader_thread = ::CreateThread(nullptr, 0, proc, load_started, 0, nullptr);

    WaitForSingleObject(load_started, MAXDWORD);
    CloseHandle(load_started);
    

    if (loader_lock) {
        // The thread signals prior to actually performing the dll load - give it
        // a chance to grab the loader lock before proceeding.
        Sleep(100);
        LoadLibrary(L"crypt32.dll");
    }
    else {
        EnterCriticalSection(&g_cs);
        LeaveCriticalSection(&g_cs);
    }

    CloseHandle(loader_thread);


    DeleteCriticalSection(&g_cs);
}

    