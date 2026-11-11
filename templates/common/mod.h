// mod.h - what both templates do once they run: log, identify the exe, check, hook. MIT, see LICENSE-MIT.
#pragma once

#include <windows.h>

namespace tmplt {

// Starts the mod on a new thread (never does the work in the caller). `self` is this DLL,
// `logName` the log file created next to it, `route` a short text for the log ("TMLoader plugin").
// Safe to call more than once; only the first call counts.
void modStart(HMODULE self, const wchar_t* logName, const char* route);

} // namespace tmplt
