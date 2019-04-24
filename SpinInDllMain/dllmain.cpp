#include <windows.h>
#include <realtimeapiset.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        // Run for "a while" (make this cpu cycle based instead
        // of time based in case we're not scheduled very frequently.
        ULONGLONG start = 0;
        QueryThreadCycleTime(GetCurrentThread(), &start);
        ULONGLONG current = start;
        while (current - start < MAXULONG32) {
            QueryThreadCycleTime(GetCurrentThread(), &current);
        }
    }
                             break;
    case DLL_THREAD_ATTACH:
    
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

