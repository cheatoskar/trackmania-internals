// log.cpp - see log.h. MIT, see LICENSE-MIT.
#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace tmplt {
namespace {

HANDLE g_file = INVALID_HANDLE_VALUE;
CRITICAL_SECTION g_lock;
wchar_t g_path[MAX_PATH] = {};

} // namespace

bool logOpen(HMODULE module, const wchar_t* fileName) {
    if (g_file != INVALID_HANDLE_VALUE) return true;
    DWORD n = GetModuleFileNameW(module, g_path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return false;
    wchar_t* slash = wcsrchr(g_path, L'\\');
    if (!slash) return false;
    size_t room = MAX_PATH - static_cast<size_t>(slash + 1 - g_path);
    if (wcscpy_s(slash + 1, room, fileName) != 0) return false;
    InitializeCriticalSection(&g_lock);
    g_file = CreateFileW(g_path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    return g_file != INVALID_HANDLE_VALUE;
}

void logf(const char* format, ...) {
    if (g_file == INVALID_HANDLE_VALUE) return;
    char line[1024];
    SYSTEMTIME t;
    GetLocalTime(&t);
    int n = _snprintf_s(line, sizeof(line), _TRUNCATE, "%02u:%02u:%02u.%03u ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    if (n < 0) n = 0;
    va_list args;
    va_start(args, format);
    int m = _vsnprintf_s(line + n, sizeof(line) - n - 2, _TRUNCATE, format, args);
    va_end(args);
    n += m < 0 ? static_cast<int>(strlen(line + n)) : m;
    line[n++] = '\r';
    line[n++] = '\n';
    EnterCriticalSection(&g_lock);
    DWORD written = 0;
    WriteFile(g_file, line, static_cast<DWORD>(n), &written, nullptr);
    LeaveCriticalSection(&g_lock);
}

const wchar_t* logPath() {
    return g_path;
}

} // namespace tmplt
