// plugin.cpp - TMLoader plugin entry. MIT, see LICENSE-MIT.
//
// TMLoader (TrackMania ModLoader 2.12.0) loads the DLL named in the version's description.yaml
// ("executable:") into its TmForever.exe and needs the export `_DllMain@12`; without it the game
// fails to start with 0xc000007b. `extern "C" __declspec(dllexport)` on a WINAPI (stdcall)
// DllMain exports exactly that decorated name on 32-bit MSVC (check: dumpbin /exports).
//
// DllMain runs under the loader lock: it only starts a thread. Everything else happens there.
#include <windows.h>

#include "mod.h"

extern "C" __declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        tmplt::modStart(instance, L"TemplatePlugin.log", "TMLoader plugin");
    }
    return TRUE;
}
