

#include <windows.h>

static const char kTarget[] = "PhotonChat";
static const SIZE_T kTargetLen = sizeof(kTarget) - 1; // do not include trailing NUL

static void PatchRange(void* base, SIZE_T size) {
    unsigned char* region = static_cast<unsigned char*>(base);
    if (size < kTargetLen) return;

    for (SIZE_T off = 0; off + kTargetLen <= size; ++off) {
        if (memcmp(region + off, kTarget, kTargetLen) == 0) {
            // change page protection, write zeros, restore
            DWORD oldProtect;
            unsigned char* addr = region + off;
            if (VirtualProtect(reinterpret_cast<LPVOID>(addr), kTargetLen, PAGE_READWRITE, &oldProtect)) {
                memset(addr, 0, kTargetLen);
                (void)VirtualProtect(reinterpret_cast<LPVOID>(addr), kTargetLen, oldProtect, &oldProtect);
            }
        }
    }
}

static void ScanAndPatchAllMemory() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    unsigned char* addr = reinterpret_cast<unsigned char*>(si.lpMinimumApplicationAddress);
    unsigned char* maxAddr = reinterpret_cast<unsigned char*>(si.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi;
    while (addr < maxAddr) {
        SIZE_T result = VirtualQuery(addr, &mbi, sizeof(mbi));
        if (result == 0) break;

        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_NOACCESS) == 0) {
            // limit scan size to region size
            SIZE_T regionSize = mbi.RegionSize;
            PatchRange(mbi.BaseAddress, regionSize);
        }

        // advance to next region
        addr += mbi.RegionSize;
    }
}

// Run from a new thread to avoid doing work inside DllMain.
static DWORD WINAPI InitThread(LPVOID) {
    ScanAndPatchAllMemory();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Avoid loader lock issues: create a thread to perform scanning.
        // Keep thread stack & work minimal.
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}