// proxy.cpp - dinput8.dll proxy entry. MIT, see LICENSE-MIT.
// The forwarding and chain-load part is adapted from the author's own dinput8 loader
// (MIT, Copyright (c) 2026 cheatoskar).
//
// The classic games load dinput8.dll from their own folder before the system one (TMNF/TMUF and
// ESWC import it statically; Sunrise loads it by name). This DLL:
//   1. passes every DirectInput export on, unchanged, to the real dinput8.dll;
//   2. starts the mod (common/mod.cpp) on its own thread when it is loaded. DllMain only creates
//      that thread (loader lock); the thread runs once the loader is done, early enough to hook
//      CreateDevice before the game creates its Direct3D device. If you move the start to the first
//      DirectInput8Create call instead (the game's main thread, as the Horizon loader does), check
//      that the game has not created its device by then.
// Chain: if dinput8.chain.dll lies next to this DLL (another dinput8 proxy, renamed), every export
// goes to that DLL instead of the system one, so the other mod keeps working.
//
// Never import dinput8.dll by name from here (that would be this file): the system copy is loaded
// by full path.
#include <windows.h>
#include <unknwn.h>

#include "mod.h"

namespace {

HMODULE g_self = nullptr;
HMODULE volatile g_real = nullptr; // set last, after g_procs (volatile: release order on MSVC x86)
FARPROC g_procs[6] = {};
const char* const kNames[6] = {"DirectInput8Create", "DllCanUnloadNow",     "DllGetClassObject",
                               "DllRegisterServer",  "DllUnregisterServer", "GetdfDIJoystick"};
volatile LONG g_loading = 0;

// <folder of this DLL>\<name> into `out`.
bool besideSelf(const wchar_t* name, wchar_t* out, size_t size) {
    DWORD n = GetModuleFileNameW(g_self, out, static_cast<DWORD>(size));
    if (n == 0 || n >= size) return false;
    wchar_t* slash = nullptr;
    for (wchar_t* p = out; *p; p++)
        if (*p == L'\\' || *p == L'/') slash = p;
    if (!slash) return false;
    return wcscpy_s(slash + 1, size - static_cast<size_t>(slash + 1 - out), name) == 0;
}

HMODULE loadChain() {
    wchar_t path[MAX_PATH];
    if (!besideSelf(L"dinput8.chain.dll", path, MAX_PATH)) return nullptr;
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return nullptr;
    HMODULE chain = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!chain || chain == g_self) return nullptr;
    return chain;
}

bool loadReal() {
    if (g_real) return true;
    // One thread loads; a second caller waits briefly (the game calls these from one thread anyway).
    if (InterlockedCompareExchange(&g_loading, 1, 0) != 0) {
        for (int i = 0; i < 2000 && !g_real; i++) Sleep(1);
        return g_real != nullptr;
    }
    HMODULE chain = loadChain();
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH); // SysWOW64 for this 32-bit process (WOW64 redirection)
    HMODULE real = nullptr;
    if (n > 0 && n <= MAX_PATH - 16 && wcscat_s(path, MAX_PATH, L"\\dinput8.dll") == 0) real = LoadLibraryW(path);
    if (!real) {
        InterlockedExchange(&g_loading, 0);
        return false;
    }
    for (int i = 0; i < 6; i++) {
        FARPROC p = chain ? GetProcAddress(chain, kNames[i]) : nullptr;
        g_procs[i] = p ? p : GetProcAddress(real, kNames[i]);
    }
    g_real = real;
    return true;
}

FARPROC proc(int i) {
    return loadReal() ? g_procs[i] : nullptr;
}

} // namespace

extern "C" {

HRESULT WINAPI Proxy_DirectInput8Create(HINSTANCE inst, DWORD version, REFIID riid, LPVOID* out, LPUNKNOWN outer) {
    using Fn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    Fn fn = reinterpret_cast<Fn>(proc(0));
    return fn ? fn(inst, version, riid, out, outer) : E_FAIL;
}

HRESULT WINAPI Proxy_DllCanUnloadNow() {
    using Fn = HRESULT(WINAPI*)();
    Fn fn = reinterpret_cast<Fn>(proc(1));
    return fn ? fn() : S_FALSE;
}

HRESULT WINAPI Proxy_DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out) {
    using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
    Fn fn = reinterpret_cast<Fn>(proc(2));
    return fn ? fn(clsid, riid, out) : CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI Proxy_DllRegisterServer() {
    using Fn = HRESULT(WINAPI*)();
    Fn fn = reinterpret_cast<Fn>(proc(3));
    return fn ? fn() : E_FAIL;
}

HRESULT WINAPI Proxy_DllUnregisterServer() {
    using Fn = HRESULT(WINAPI*)();
    Fn fn = reinterpret_cast<Fn>(proc(4));
    return fn ? fn() : E_FAIL;
}

LPCVOID WINAPI Proxy_GetdfDIJoystick() {
    using Fn = LPCVOID(WINAPI*)();
    Fn fn = reinterpret_cast<Fn>(proc(5));
    return fn ? fn() : nullptr;
}

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        tmplt::modStart(instance, L"dinput8-template.log", "dinput8 proxy"); // only creates a thread
    }
    return TRUE;
}
