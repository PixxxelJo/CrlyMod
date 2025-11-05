#include <windows.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include "crlylog.h"

// Kleine Hilfsfunktion, damit wir Formattierungen machen können,
// ohne CrlyLog selbst anzupassen.
static void CrlyLogFmt(const char* tag, const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, sizeof(buf), fmt, args);
    va_end(args);
    CrlyLog(tag, buf);
}

// --- String-Scanner für "PhotonChat" ---------------------------
static void DisablePhotonChatStrings()
{
    const char target[] = "PhotonChat";
    const SIZE_T len = sizeof(target) - 1;

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    unsigned char* addr = static_cast<unsigned char*>(si.lpMinimumApplicationAddress);
    unsigned char* maxAddr = static_cast<unsigned char*>(si.lpMaximumApplicationAddress);
    MEMORY_BASIC_INFORMATION mbi;

    while (addr < maxAddr && VirtualQuery(addr, &mbi, sizeof(mbi)) != 0) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            unsigned char* base = static_cast<unsigned char*>(mbi.BaseAddress);
            SIZE_T regionSize = mbi.RegionSize;

            for (SIZE_T i = 0; i + len <= regionSize; ++i) {
                if (memcmp(base + i, target, len) == 0) {
                    DWORD oldProtect;
                    if (VirtualProtect(base + i, len, PAGE_READWRITE, &oldProtect)) {
                        memset(base + i, 0, len);
                        DWORD tmp;
                        VirtualProtect(base + i, len, oldProtect, &tmp);
                    }
                }
            }
        }
        addr += mbi.RegionSize;
    }
}

// --- Direkte Adressblockierung ------------------------------
static void DisablePhotonAddresses()
{
    constexpr uintptr_t targets[] = {
        0x25D1030,
        0x25D1260
    };

    const SIZE_T patchSize = 16;

    for (uintptr_t t : targets) {
        void* pAddr = reinterpret_cast<void*>(t);
        MEMORY_BASIC_INFORMATION mbi;

        if (VirtualQuery(pAddr, &mbi, sizeof(mbi)) == 0) {
            CrlyLogFmt("DvarPatcher", "VirtualQuery failed for address %p", pAddr);
            continue;
        }

        if (mbi.State != MEM_COMMIT) {
            CrlyLogFmt("DvarPatcher", "Address %p not committed.", pAddr);
            continue;
        }

        DWORD oldProtect;
        if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &oldProtect)) {
            CrlyLogFmt("DvarPatcher", "VirtualProtect failed for %p", pAddr);
            continue;
        }

        uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t regionEnd = regionStart + mbi.RegionSize;
        uintptr_t writeStart = t;
        SIZE_T maxWritable = (writeStart < regionEnd) ? (regionEnd - writeStart) : 0;
        SIZE_T toWrite = (patchSize <= maxWritable) ? patchSize : maxWritable;

        if (toWrite > 0) {
            memset(reinterpret_cast<void*>(writeStart), 0, toWrite);
            CrlyLogFmt("DvarPatcher", "Patched %zu bytes at %p", toWrite, reinterpret_cast<void*>(writeStart));
        }
        else {
            CrlyLogFmt("DvarPatcher", "No writable space at %p", pAddr);
        }

        DWORD tmp;
        VirtualProtect(mbi.BaseAddress, mbi.RegionSize, oldProtect, &tmp);
    }
}

// --- Initialisierung auf eigenem Thread --------------------
void InitializeDvarPatcher() {
    HANDLE h = CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        CrlyLog("DvarPatcher", "Initialization thread started.");
        DisablePhotonChatStrings();
        DisablePhotonAddresses();
        CrlyLog("DvarPatcher", "Initialization thread finished.");
        return 0;
        }, nullptr, 0, nullptr);

    if (h) CloseHandle(h);
}

// --- Automatischer Start beim Laden -------------------------
struct PhotonChatDisabler {
    PhotonChatDisabler() {
        CrlyLog("DvarPatcher", "The Chat is disabled. Starting patcher...");
        InitializeDvarPatcher();
    }
};
static PhotonChatDisabler g_photonChatDisabler;
