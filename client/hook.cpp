#include <windows.h>
#include "detours/detours.h"
#include <cstdio>
#include "crlylog.h"
#pragma comment(lib, "detours.lib")

// hook.cpp no longer defines DllMain. Initialization is performed by
// `main.cpp` which calls `InitializeHooks()` from a dedicated init thread.

// By default we assume `EntryPoint` has signature: void __cdecl EntryPoint(void)
// If the real signature differs, update `EntryPointType` and `HookedEntryPoint`
typedef void (__cdecl *EntryPointType)(void);

// Store original function pointer
static EntryPointType g_originalEntryPoint = nullptr;

// Our detour which will replace Ext.dll's EntryPoint
static void __cdecl HookedEntryPoint(void)
{
    // Log the call
    CrlyLog("Hook", "EntryPoint called.");

    // Call original EntryPoint (if present)
    if (g_originalEntryPoint) {
        g_originalEntryPoint();
    }
}

struct HookInitializer {
    HookInitializer() {
        // Wait for Ext.dll to be loaded by the host process. This runs in the
        // worker thread so sleeping is safe.
        HMODULE hModule = nullptr;
        for (int i = 0; i < 100 && !hModule; ++i) {
            hModule = GetModuleHandleA("Ext.dll");
            if (!hModule) Sleep(50);
        }
        if (!hModule) {
            CrlyLog("Hook", "Failed to find Ext.dll.");
            return;
        }

        // Look up the exported EntryPoint
        FARPROC proc = GetProcAddress(hModule, "EntryPoint");
        if (!proc) {
            CrlyLog("Hook", "Ext.dll does not export EntryPoint.");
            return;
        }

        g_originalEntryPoint = reinterpret_cast<EntryPointType>(proc);

        // Attach detour using Detours
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(reinterpret_cast<PVOID*>(&g_originalEntryPoint), HookedEntryPoint);
        LONG err = DetourTransactionCommit();
        if (err != NO_ERROR) {
            CrlyLog("Hook", "DetourTransactionCommit failed.");
            return;
        }

        CrlyLog("Hook", "EntryPoint hooked.");
    }

    ~HookInitializer() {
        if (g_originalEntryPoint) {
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(reinterpret_cast<PVOID*>(&g_originalEntryPoint), HookedEntryPoint);
            DetourTransactionCommit();
        }
    }
};


// Exposed initialization function
void InitializeHooks() {
    static HookInitializer hookInit;  // Will construct and initialize hooks
}

void intializeWatermark()
{
}

void InitializeChatPatcher()
{
}
