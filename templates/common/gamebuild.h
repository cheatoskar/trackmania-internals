// gamebuild.h - which exe is running, and is it one the tables cover. MIT, see LICENSE-MIT.
#pragma once

#include "builds.h"

#include <cstdint>

namespace tmplt {

struct ExeIdentity {
    uint32_t timestamp = 0;
    uint32_t fileSize = 0;
    uintptr_t base = 0;     // where the exe is loaded (GetModuleHandle(nullptr))
    bool relocated = false; // base != 0x00400000 (TMLoader 2.12.0 default profile)
};

// Reads the running exe's PE header (in memory) and its file size (on disk).
ExeIdentity identifyExe();

// The table entry for this exe, or nullptr for an unknown build.
const GameBuild* findBuild(const ExeIdentity& exe);

// Link-time VA -> address in this process. Never use a VA directly.
uintptr_t rebase(const ExeIdentity& exe, uint32_t va);

// Reads (never writes) the build's check site. Returns false and fills `found` (hex text) when the
// bytes differ or cannot be read.
bool verifyCheckSite(const ExeIdentity& exe, const GameBuild& build, char* found, size_t foundSize);

} // namespace tmplt
