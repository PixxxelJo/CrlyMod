#include <windows.h>
#include <cstdint>

static void DisablePhotonAddresses()
{
    // List of target addresses
    uintptr_t targets[] = {
        0x25D1030,
        0x25D1260
    };

    for (uintptr_t addr : targets) {
        DWORD oldProtect;
        if (VirtualProtect(reinterpret_cast<LPVOID>(addr), 16, PAGE_READWRITE, &oldProtect)) {
            // Zero out a small memory range (adjust size if needed)
            memset(reinterpret_cast<void*>(addr), 0, 16);

            DWORD tmp;
            VirtualProtect(reinterpret_cast<LPVOID>(addr), 16, oldProtect, &tmp);
        }
    }
}

// Run automatically when the DLL or EXE loads
struct DvarPatcher {
    DvarPatcher() { DisablePhotonAddresses(); }
};
static DvarPatcher g_dvarPatcher;
