// log.h - a plain text log next to the mod DLL. MIT, see LICENSE-MIT.
#pragma once

#include <windows.h>

namespace tmplt {

// Opens (truncates) <folder of module>\<fileName>. Call once, outside DllMain.
bool logOpen(HMODULE module, const wchar_t* fileName);
// printf-style line with a time stamp. Thread-safe; cheap enough for a line every few seconds,
// not for every frame.
void logf(const char* format, ...);
// Full path of the log file (empty before logOpen).
const wchar_t* logPath();

} // namespace tmplt
