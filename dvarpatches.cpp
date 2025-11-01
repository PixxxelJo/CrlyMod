#include <windows.h>
#include <cstring>
#include <iostream>

// This startup patch scans the process memory for occurrences of the literal
// "PhotonChat" and overwrites them with zero bytes so code that looks up
// dvars or features by that exact string will fail to find PhotonChat.
// Note: keep this minimal and deterministic to avoid runtime side-effects.

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

            // simple byte-scan within the region
            for (SIZE_T i = 0; i + len <= regionSize; ++i) {
                if (memcmp(base + i, target, len) == 0) {
                    DWORD oldProtect;
                    if (VirtualProtect(base + i, len, PAGE_READWRITE, &oldProtect)) {
                        // overwrite the literal so lookups fail
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

// Exposed initialization function
void InitializeDvarPatcher() {
    DisablePhotonChatStrings();
}

struct PhotonChatDisabler {
    PhotonChatDisabler() {
        std::cout << "[Component/DvarPatcher] The Chat is disabled.\n";
    }
};