#include <windows.h>
#include <string>

typedef DWORD (WINAPI *PFN_GetFileVersionInfoSizeA)(LPCSTR, LPDWORD);
static HMODULE g_real = nullptr;
static PFN_GetFileVersionInfoSizeA real_GetFileVersionInfoSizeA = nullptr;

BOOL LoadRealVersion()
{
    if (g_real) return TRUE;
    char sysdir[MAX_PATH];
    UINT n = GetSystemDirectoryA(sysdir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return FALSE;
    std::string path = std::string(sysdir) + "\\version.dll";
    g_real = LoadLibraryA(path.c_str());
    if (!g_real) return FALSE;
    real_GetFileVersionInfoSizeA = (PFN_GetFileVersionInfoSizeA)GetProcAddress(g_real, "GetFileVersionInfoSizeA");
    // load other function pointers similarly...
    return TRUE;
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle)
{
    if (!LoadRealVersion() || !real_GetFileVersionInfoSizeA) return 0;
    return real_GetFileVersionInfoSizeA(lptstrFilename, lpdwHandle);
}

// also forward GetFileVersionInfoA, VerQueryValueA, and wide variants...
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_DETACH && g_real) {
        FreeLibrary(g_real);
    }
    return TRUE;
}