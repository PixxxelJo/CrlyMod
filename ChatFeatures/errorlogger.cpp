#include <windows.h>
#include <cstdio>
#include <ctime>

static void AppendLogA(const char* msg)
{
    char tempPath[MAX_PATH] = {0};
    if (GetTempPathA(MAX_PATH, tempPath) == 0)
        tempPath[0] = 0;

    char path[MAX_PATH] = {0};
    strcpy_s(path, tempPath);
    strcat_s(path, "CrlyError.log");

    FILE* f = nullptr;
    if (fopen_s(&f, path, "a, ccs=UTF-8") == 0 && f) {
        time_t t = time(nullptr);
        struct tm tm{};
        localtime_s(&tm, &t);

        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
        fprintf(f, "[%s] %s\n", timeBuf, msg ? msg : "(null)");
        fclose(f);
    }

    OutputDebugStringA(msg ? msg : "(null)");
    OutputDebugStringA("\n");
}

static void AppendLogW(const wchar_t* msg)
{
    char buf[1024];
    if (msg) {
        WideCharToMultiByte(CP_UTF8, 0, msg, -1, buf, (int)sizeof(buf), nullptr, nullptr);
    } else {
        strcpy_s(buf, "(null)");
    }
    AppendLogA(buf);
}

extern "C" __declspec(dllexport) void ReportErrorA(const char* msg)
{
    AppendLogA(msg);
}

extern "C" __declspec(dllexport) void ReportErrorW(const wchar_t* msg)
{
    AppendLogW(msg);
}