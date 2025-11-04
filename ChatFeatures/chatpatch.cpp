#include <windows.h>
#include <cstring>

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
						// restore protection
						DWORD tmp;
						VirtualProtect(base + i, len, oldProtect, &tmp);
					}
				}
			}
		}

		addr += mbi.RegionSize;
	}
}

// Run at module initialization
struct DvarPatcher {
	DvarPatcher() { DisablePhotonChatStrings(); }
};
static DvarPatcher g_dvarPatcher;
