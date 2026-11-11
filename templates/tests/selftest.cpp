// selftest.cpp - checks the template code without any game. MIT, see LICENSE-MIT.
//
// Usage: selftest.exe <TemplatePlugin.dll> <dinput8.dll>
// Runs: inline hook on a hand-made function (and two refusals), slot hook on a read-only table,
// build lookup, the frame counter on a real Direct3D 9 device (SKIP without one), and loads both
// DLLs: each must write its log and refuse this exe as an unknown build.
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <d3d9.h>
#include <dinput.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "frame_counter.h"
#include "gamebuild.h"
#include "hook.h"
#include "log.h"

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) g_failures++;
}

// int f(int a) { return a + 1; } with a hot-patch prologue: mov edi,edi; push ebp; mov ebp,esp
const uint8_t kFunction[] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08, 0x40, 0x5D, 0xC3};
const uint8_t kPrologue[] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
using IntFn = int(__cdecl*)(int);
IntFn g_original = nullptr;

int __cdecl timesTen(int a) {
    return g_original(a) * 10;
}

void testInline() {
    auto code = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    memcpy(code, kFunction, sizeof(kFunction));
    auto f = reinterpret_cast<IntFn>(code);
    check(f(4) == 5, "inline: function works before the hook");

    const uint8_t wrong[] = {0x55, 0x8B, 0xEC, 0x8B, 0x45};
    void* tramp = nullptr;
    check(!tmplt::hookInline(code, wrong, sizeof(wrong), reinterpret_cast<void*>(&timesTen), &tramp),
          "inline: refuses when the bytes differ");
    const uint8_t jump[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    check(!tmplt::hookInline(code, jump, sizeof(jump), reinterpret_cast<void*>(&timesTen), &tramp),
          "inline: refuses a relative first instruction");
    check(memcmp(code, kFunction, sizeof(kFunction)) == 0, "inline: nothing written after a refusal");

    bool hooked = tmplt::hookInline(code, kPrologue, sizeof(kPrologue), reinterpret_cast<void*>(&timesTen), &tramp);
    g_original = reinterpret_cast<IntFn>(tramp);
    check(hooked, "inline: hooks a known prologue");
    check(hooked && f(4) == 50, "inline: detour runs and calls the original through the trampoline");
}

using SlotFn = int(__cdecl*)();
int __cdecl one() { return 1; }
int __cdecl two() { return 2; }

void testSlot() {
    auto table = static_cast<void**>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    table[3] = reinterpret_cast<void*>(&one);
    DWORD old = 0;
    VirtualProtect(table, 4096, PAGE_READONLY, &old); // like a vtable in .rdata
    void* previous = nullptr;
    bool ok = tmplt::hookSlot(&table[3], reinterpret_cast<void*>(&two), &previous);
    check(ok && previous == reinterpret_cast<void*>(&one), "slot: swaps a read-only slot, returns the old value");
    check(ok && reinterpret_cast<SlotFn>(table[3])() == 2, "slot: new value is called");
    MEMORY_BASIC_INFORMATION info{};
    VirtualQuery(table, &info, sizeof(info));
    check(info.Protect == PAGE_READONLY, "slot: protection restored");
}

void testBuilds() {
    tmplt::ExeIdentity exe = tmplt::identifyExe();
    check(exe.timestamp != 0 && exe.fileSize != 0, "build: reads timestamp and file size of this exe");
    check(tmplt::findBuild(exe) == nullptr, "build: this exe is not a known game build");
    tmplt::ExeIdentity moved;
    moved.base = 0x01230000;
    check(tmplt::rebase(moved, 0x0047c0d0) == 0x012ac0d0, "build: rebase = base - 0x400000 + VA");
    tmplt::ExeIdentity fake;
    fake.timestamp = 0x4d431494;
    fake.fileSize = 10498048;
    const tmplt::GameBuild* b = tmplt::findBuild(fake);
    check(b && strstr(b->name, "TMNF"), "build: TMNF entry found by timestamp and size");
}

LRESULT CALLBACK windowProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(w, m, wp, lp);
}

void testFrameCounter() {
    if (IDirect3D9* probe = Direct3DCreate9(D3D_SDK_VERSION)) {
        probe->Release();
    } else {
        printf("SKIP  frames: Direct3D 9 not available on this machine\n");
        return;
    }
    check(tmplt::frameCounterInstall(), "frames: hooks installed (probe device or CreateDevice)");
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TemplateSelftest";
    RegisterClassExW(&wc);
    HWND window = CreateWindowExW(0, wc.lpszClassName, L"selftest", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr,
                                  wc.hInstance, nullptr);
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3DDevice9* device = nullptr;
    if (d3d && window) {
        D3DPRESENT_PARAMETERS pp{};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window;
        pp.BackBufferWidth = 64;
        pp.BackBufferHeight = 64;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device);
    }
    if (!device) {
        printf("SKIP  frames: no Direct3D 9 device on this machine\n");
    } else {
        // Normally the CreateDevice hook has hooked this device already. An overlay injected into
        // the process (Steam, GPU tools, streaming) may replace the CreateDevice slot after us; then
        // attach by hand so the Present part is still tested.
        void* presentSlot = (*reinterpret_cast<void***>(device))[17];
        tmplt::frameCounterAttach(device);
        printf("      %s\n", (*reinterpret_cast<void***>(device))[17] == presentSlot
                                 ? "device hooked through the CreateDevice hook"
                                 : "CreateDevice hook bypassed (overlay?); device attached by hand");
        const unsigned long before = tmplt::frameCounterFrames();
        for (int i = 0; i < 30; i++) {
            device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            device->Present(nullptr, nullptr, nullptr, nullptr);
        }
        check(tmplt::frameCounterFrames() - before == 30, "frames: every Present counted, the original still called");
        device->Release();
    }
    if (d3d) d3d->Release();
    if (window) DestroyWindow(window);
}

std::string readBeside(const wchar_t* dll, const wchar_t* logName) {
    wchar_t path[MAX_PATH];
    if (!GetFullPathNameW(dll, MAX_PATH, path, nullptr)) return {};
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return {};
    wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), logName);
    for (int i = 0; i < 50; i++) { // the mod thread writes the log shortly after the start
        HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            char buf[4096] = {};
            DWORD got = 0;
            ReadFile(f, buf, sizeof(buf) - 1, &got, nullptr);
            CloseHandle(f);
            if (strstr(buf, "UNKNOWN exe")) return buf;
        }
        Sleep(100);
    }
    return {};
}

void testDlls(const wchar_t* plugin, const wchar_t* proxy) {
    HMODULE p = LoadLibraryW(plugin);
    check(p != nullptr, "plugin: DLL loads");
    check(p && GetProcAddress(p, "_DllMain@12") != nullptr, "plugin: exports _DllMain@12");
    check(!readBeside(plugin, L"TemplatePlugin.log").empty(), "plugin: log written, unknown exe refused");

    wchar_t full[MAX_PATH];
    GetFullPathNameW(proxy, MAX_PATH, full, nullptr);
    HMODULE d = LoadLibraryW(full);
    check(d != nullptr, "proxy: DLL loads");
    const char* const exports[] = {"DirectInput8Create", "DllCanUnloadNow",     "DllGetClassObject",
                                   "DllRegisterServer",  "DllUnregisterServer", "GetdfDIJoystick"};
    bool all = d != nullptr;
    for (const char* name : exports) all = all && GetProcAddress(d, name) != nullptr;
    check(all, "proxy: exports all six dinput8 functions");
    using CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    auto create = d ? reinterpret_cast<CreateFn>(GetProcAddress(d, "DirectInput8Create")) : nullptr;
    IDirectInput8W* input = nullptr;
    HRESULT hr = create ? create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                 reinterpret_cast<void**>(&input), nullptr)
                        : E_FAIL;
    check(SUCCEEDED(hr) && input, "proxy: DirectInput8Create forwarded to the system DLL");
    if (input) input->Release();
    check(!readBeside(proxy, L"dinput8-template.log").empty(), "proxy: log written, unknown exe refused");
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    tmplt::logOpen(GetModuleHandleW(nullptr), L"selftest.log");
    testInline();
    testSlot();
    testBuilds();
    testFrameCounter();
    if (argc >= 3) testDlls(argv[1], argv[2]);
    else printf("SKIP  DLL tests: pass the plugin and proxy DLL paths\n");
    printf(g_failures ? "%d FAILED\n" : "all passed\n", g_failures);
    return g_failures ? 1 : 0;
}
