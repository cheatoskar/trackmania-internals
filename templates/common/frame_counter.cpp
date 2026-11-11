// frame_counter.cpp - see frame_counter.h. MIT, see LICENSE-MIT.
#include "frame_counter.h"

#include "hook.h"
#include "log.h"

#include <windows.h>
#include <d3d9.h>

namespace tmplt {

namespace {

constexpr int kCreateDeviceSlot = 16; // IDirect3D9::CreateDevice
constexpr int kPresentSlot = 17;      // IDirect3DDevice9::Present
constexpr int kMaxVTables = 8;        // vtables differ per vertex-processing mode, or even per device

using PresentFn = HRESULT(WINAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using CreateDeviceFn = HRESULT(WINAPI*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using Create9Fn = IDirect3D9*(WINAPI*)(UINT);

struct HookedVTable {
    void** vtable;
    PresentFn original;
};

HookedVTable g_vtables[kMaxVTables] = {};
volatile LONG g_vtableCount = 0;
CRITICAL_SECTION g_hookLock;
CreateDeviceFn g_createDevice = nullptr;
volatile LONG g_frames = 0;
ULONGLONG g_windowStart = 0;
LONG g_windowFrames = 0;

BOOL CALLBACK initLock(PINIT_ONCE, PVOID, PVOID*) {
    InitializeCriticalSection(&g_hookLock);
    return TRUE;
}

void ensureLock() {
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&once, initLock, nullptr, nullptr);
}

PresentFn originalFor(IDirect3DDevice9* device) {
    void** vtable = *reinterpret_cast<void***>(device);
    const LONG count = g_vtableCount;
    for (LONG i = 0; i < count; i++) {
        if (g_vtables[i].vtable == vtable) return g_vtables[i].original;
    }
    return nullptr;
}

HRESULT WINAPI presentDetour(IDirect3DDevice9* device, const RECT* src, const RECT* dst, HWND window, const RGNDATA* dirty) {
    const LONG frames = InterlockedIncrement(&g_frames);
    const ULONGLONG now = GetTickCount64();
    if (g_windowStart == 0) {
        g_windowStart = now;
        g_windowFrames = frames;
        logf("first frame presented");
    } else if (now - g_windowStart >= 10000) {
        const double fps = (frames - g_windowFrames) * 1000.0 / static_cast<double>(now - g_windowStart);
        logf("%.1f fps (frame %ld)", fps, frames);
        g_windowStart = now;
        g_windowFrames = frames;
    }
    PresentFn original = originalFor(device);
    return original ? original(device, src, dst, window, dirty) : D3DERR_INVALIDCALL;
}

// Hooks Present in the vtable of `device` unless its slot already points at us.
// Some d3d9 versions give every device its own vtable on the heap, so a released device's vtable
// address can come back for a new device with the original slot: check the slot, not the address.
void hookDevice(IDirect3DDevice9* device, const char* which) {
    void** vtable = *reinterpret_cast<void***>(device);
    EnterCriticalSection(&g_hookLock);
    if (vtable[kPresentSlot] != reinterpret_cast<void*>(&presentDetour)) {
        HookedVTable* entry = nullptr;
        for (LONG i = 0; i < g_vtableCount && !entry; i++) {
            if (g_vtables[i].vtable == vtable) entry = &g_vtables[i];
        }
        if (!entry && g_vtableCount < kMaxVTables) {
            entry = &g_vtables[g_vtableCount];
            entry->vtable = vtable;
            entry->original = reinterpret_cast<PresentFn>(vtable[kPresentSlot]);
            InterlockedIncrement(&g_vtableCount);
        }
        if (!entry) {
            logf("hook: too many device vtables, %p not hooked", static_cast<void*>(vtable));
            LeaveCriticalSection(&g_hookLock);
            return;
        }
        // The entry holds the current slot value before the swap: once the slot points at us,
        // presentDetour must find its original.
        entry->original = reinterpret_cast<PresentFn>(vtable[kPresentSlot]);
        void* previous = nullptr;
        if (hookSlot(&vtable[kPresentSlot], reinterpret_cast<void*>(&presentDetour), &previous)) {
            entry->original = reinterpret_cast<PresentFn>(previous);
            logf("hook: Present hooked in device vtable %p (%s)", static_cast<void*>(vtable), which);
        } else {
            logf("hook: could not write the Present slot of vtable %p", static_cast<void*>(vtable));
        }
    }
    LeaveCriticalSection(&g_hookLock);
}

HRESULT WINAPI createDeviceDetour(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
                                  D3DPRESENT_PARAMETERS* params, IDirect3DDevice9** out) {
    const HRESULT hr = g_createDevice(self, adapter, type, window, flags, params, out);
    if (SUCCEEDED(hr) && out && *out) hookDevice(*out, "device created after install");
    return hr;
}

IDirect3DDevice9* makeProbeDevice(IDirect3D9* d3d, HWND window) {
    const DWORD modes[] = {D3DCREATE_HARDWARE_VERTEXPROCESSING, D3DCREATE_SOFTWARE_VERTEXPROCESSING};
    for (DWORD mode : modes) {
        D3DPRESENT_PARAMETERS params{};
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = window;
        params.BackBufferWidth = 16;
        params.BackBufferHeight = 16;
        IDirect3DDevice9* device = nullptr;
        const DWORD flags = mode | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_FPU_PRESERVE;
        if (SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, flags, &params, &device)) && device) {
            return device;
        }
    }
    return nullptr;
}

} // namespace

bool frameCounterInstall() {
    static volatile LONG once = 0;
    if (InterlockedExchange(&once, 1) != 0) return g_vtableCount > 0;
    ensureLock();

    // The d3d9.dll the game uses: already loaded, or the one the normal search order finds (a d3d9
    // wrapper in the game folder comes first, which is what the game gets as well).
    HMODULE d3d9 = GetModuleHandleW(L"d3d9.dll");
    if (!d3d9) d3d9 = LoadLibraryW(L"d3d9.dll");
    auto create9 = d3d9 ? reinterpret_cast<Create9Fn>(GetProcAddress(d3d9, "Direct3DCreate9")) : nullptr;
    if (!create9) {
        logf("hook: d3d9.dll / Direct3DCreate9 not found");
        return false;
    }
    IDirect3D9* d3d = create9(D3D_SDK_VERSION);
    if (!d3d) {
        logf("hook: Direct3DCreate9 failed");
        return false;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TemplateModProbeWindow";
    RegisterClassExW(&wc);
    HWND window = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 16, 16, nullptr, nullptr, wc.hInstance, nullptr);
    if (window) {
        if (IDirect3DDevice9* probe = makeProbeDevice(d3d, window)) {
            hookDevice(probe, "probe device");
            probe->Release();
        } else {
            logf("hook: no probe device (no Direct3D 9 adapter?)");
        }
        DestroyWindow(window);
    }
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // IDirect3D9 objects of one d3d9.dll share a vtable: the game's factory goes through this too.
    void** factoryVTable = *reinterpret_cast<void***>(d3d);
    // Original first: the game may call CreateDevice the moment the slot changes.
    g_createDevice = reinterpret_cast<CreateDeviceFn>(factoryVTable[kCreateDeviceSlot]);
    void* previous = nullptr;
    if (hookSlot(&factoryVTable[kCreateDeviceSlot], reinterpret_cast<void*>(&createDeviceDetour), &previous)) {
        g_createDevice = reinterpret_cast<CreateDeviceFn>(previous);
        logf("hook: CreateDevice hooked in factory vtable %p (devices created from now on are covered)",
             static_cast<void*>(factoryVTable));
    } else {
        g_createDevice = nullptr;
        logf("hook: could not write the CreateDevice slot");
    }
    d3d->Release();
    return g_vtableCount > 0 || g_createDevice != nullptr;
}

void frameCounterAttach(void* device) {
    if (!device) return;
    ensureLock();
    hookDevice(static_cast<IDirect3DDevice9*>(device), "attached");
}

unsigned long frameCounterFrames() {
    return static_cast<unsigned long>(g_frames);
}

} // namespace tmplt
