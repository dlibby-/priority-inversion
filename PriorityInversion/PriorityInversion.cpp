#include <iostream>
#include <Windows.h>
#include <thread>


DWORD WINAPI LoadSlowDll(LPVOID param) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
    SetEvent(static_cast<HANDLE>(param));
    LoadLibrary(L"SpinInDllMain.dll");
    return 0;
}

DWORD WINAPI SpinForever(LPVOID param) {
    while (true);
    return 0;
}

int main()
{
    // Put some pressure on the system scheduler by spinning 2x the number
    // of threads as there are logical cores.
    unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < 2 * concurentThreadsSupported; i++) {
        HANDLE spin_thread = ::CreateThread(nullptr, 0, &SpinForever, 0, 0, nullptr);
        CloseHandle(spin_thread);
    }

    // Create a thread that will load the DLL that spins with the loader lock
    // held. That thread will run at background priority.
    // Then try to acquire the loader lock with this normal priority thread.
    HANDLE load_started = CreateEvent(nullptr, FALSE, FALSE, L"StartLoad");
    HANDLE loader_thread = ::CreateThread(nullptr, 0, &LoadSlowDll, load_started, 0, nullptr);

    WaitForSingleObject(load_started, MAXDWORD);
    CloseHandle(load_started);
    // The thread signals prior to actually loading the library - give it
    // a chance to grab the loader lock before proceeding.
    Sleep(100);


    LoadLibrary(L"crypt32.dll");
    CloseHandle(loader_thread);
}

    