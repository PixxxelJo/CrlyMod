#include <windows.h>
#include <string>

// NOTE: don't forward-declare the exported functions here —
// the Windows headers already declare them and redeclaring
// with different attributes can cause "redefinition; different binding" errors.
// We'll add __declspec(dllexport) on the implementation definitions below.

// Define all function pointer types we need
typedef DWORD (WINAPI *PFN_GetFileVersionInfoSizeA)(LPCSTR, LPDWORD);
typedef BOOL (WINAPI *PFN_GetFileVersionInfoA)(LPCSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *PFN_VerQueryValueA)(LPCVOID, LPCSTR, LPVOID*, PUINT);
typedef DWORD (WINAPI *PFN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
typedef BOOL (WINAPI *PFN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *PFN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

// Our function pointers to the real DLL functions
static HMODULE g_real = nullptr;
static PFN_GetFileVersionInfoSizeA real_GetFileVersionInfoSizeA = nullptr;
static PFN_GetFileVersionInfoA real_GetFileVersionInfoA = nullptr;
static PFN_VerQueryValueA real_VerQueryValueA = nullptr;
static PFN_GetFileVersionInfoSizeW real_GetFileVersionInfoSizeW = nullptr;
static PFN_GetFileVersionInfoW real_GetFileVersionInfoW = nullptr;
static PFN_VerQueryValueW real_VerQueryValueW = nullptr;

BOOL LoadRealVersion()
{
    if (g_real) return TRUE;
    
    char sysdir[MAX_PATH];
    UINT n = GetSystemDirectoryA(sysdir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return FALSE;
    
    std::string path = std::string(sysdir) + "\\version.dll";
    g_real = LoadLibraryA(path.c_str());
    if (!g_real) return FALSE;
    
    // Load all function pointers
    real_GetFileVersionInfoSizeA = (PFN_GetFileVersionInfoSizeA)GetProcAddress(g_real, "GetFileVersionInfoSizeA");
    real_GetFileVersionInfoA = (PFN_GetFileVersionInfoA)GetProcAddress(g_real, "GetFileVersionInfoA");
    real_VerQueryValueA = (PFN_VerQueryValueA)GetProcAddress(g_real, "VerQueryValueA");
    real_GetFileVersionInfoSizeW = (PFN_GetFileVersionInfoSizeW)GetProcAddress(g_real, "GetFileVersionInfoSizeW");
    real_GetFileVersionInfoW = (PFN_GetFileVersionInfoW)GetProcAddress(g_real, "GetFileVersionInfoW");
    real_VerQueryValueW = (PFN_VerQueryValueW)GetProcAddress(g_real, "VerQueryValueW");
    
    return TRUE;
}

// Implement all the exports. Use C linkage and ensure each definition is
// exported with __declspec(dllexport) to avoid linkage mismatches with the
// Windows headers.
extern "C" {

DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle)
{
    if (!LoadRealVersion() || !real_GetFileVersionInfoSizeA) return 0;
    return real_GetFileVersionInfoSizeA(lptstrFilename, lpdwHandle);
}

BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    if (!LoadRealVersion() || !real_GetFileVersionInfoA) return FALSE;
    return real_GetFileVersionInfoA(lptstrFilename, dwHandle, dwLen, lpData);
}

BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
    if (!LoadRealVersion() || !real_VerQueryValueA) return FALSE;
    return real_VerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
}

DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle)
{
    if (!LoadRealVersion() || !real_GetFileVersionInfoSizeW) return 0;
    return real_GetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);
}

BOOL WINAPI GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    if (!LoadRealVersion() || !real_GetFileVersionInfoW) return FALSE;
    return real_GetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}

BOOL WINAPI VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
    if (!LoadRealVersion() || !real_VerQueryValueW) return FALSE;
    return real_VerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);
}

} // extern "C"

