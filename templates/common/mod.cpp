// mod.cpp - see mod.h. MIT, see LICENSE-MIT.
//
// Put your own code where "your mod starts here" says. Keep the order: identify, check, then hook.
#include "mod.h"

#include "frame_counter.h"
#include "gamebuild.h"
#include "log.h"

namespace tmplt {

namespace {

HMODULE g_self = nullptr;
const wchar_t* g_logName = nullptr;
const char* g_route = "";

DWORD WINAPI modThread(LPVOID) {
    if (!logOpen(g_self, g_logName)) return 0; // nowhere to report: do nothing at all
    logf("template mod starting (%s)", g_route);
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        logf("Shift held at start: mod off for this session");
        return 0;
    }

    const ExeIdentity exe = identifyExe();
    logf("exe: PE timestamp %08x, %lu bytes, loaded at %08x%s", exe.timestamp, static_cast<unsigned long>(exe.fileSize),
         static_cast<unsigned>(exe.base), exe.relocated ? " (relocated)" : "");

    const GameBuild* build = findBuild(exe);
    if (!build) {
        logf("UNKNOWN exe: not in the build table (templates/common/builds.h). Nothing installed.");
        return 0;
    }
    logf("build: %s", build->name);

    char found[64];
    if (!verifyCheckSite(exe, *build, found, sizeof(found))) {
        logf("check site %s at VA %08x does not match (found %s). Patched exe, another mod's hook, or a wrong table "
             "row. Nothing installed.",
             build->check.name, build->check.va, found);
        return 0;
    }
    logf("check site %s at VA %08x matches", build->check.name, build->check.va);

    // ---- your mod starts here. The example below only counts frames. ----
    if (!frameCounterInstall()) {
        logf("example hook not installed");
        return 0;
    }
    logf("example hook installed; the frame rate is logged every 10 s");

    // If the game created its device before we hooked and with another vtable, no frame arrives.
    Sleep(20000);
    if (frameCounterFrames() == 0) {
        logf("no frame seen after 20 s: the game's device was probably created before the hook, with a different "
             "vtable. Load the mod earlier, or hook the device the game actually uses.");
    }
    return 0;
}

} // namespace

void modStart(HMODULE self, const wchar_t* logName, const char* route) {
    static volatile LONG started = 0;
    if (InterlockedExchange(&started, 1) != 0) return;
    g_self = self;
    g_logName = logName;
    g_route = route;
    // Keep this DLL loaded for the life of the process: it owns hook code.
    HMODULE pinned = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       reinterpret_cast<LPCWSTR>(&modStart), &pinned);
    HANDLE thread = CreateThread(nullptr, 0, modThread, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}

} // namespace tmplt
