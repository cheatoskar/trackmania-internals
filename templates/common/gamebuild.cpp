// gamebuild.cpp - see gamebuild.h. MIT, see LICENSE-MIT.
#include "gamebuild.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace tmplt {

namespace {

constexpr uintptr_t kLinkBase = 0x00400000;

bool readBytes(const void* address, uint8_t* out, size_t length) {
    __try {
        memcpy(out, address, length);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

} // namespace

ExeIdentity identifyExe() {
    ExeIdentity id;
    auto base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    id.base = reinterpret_cast<uintptr_t>(base);
    id.relocated = id.base != kLinkBase;
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature == IMAGE_NT_SIGNATURE) id.timestamp = nt->FileHeader.TimeDateStamp;
    }
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (n > 0 && n < MAX_PATH && GetFileAttributesExW(path, GetFileExInfoStandard, &data) && data.nFileSizeHigh == 0) {
        id.fileSize = data.nFileSizeLow;
    }
    return id;
}

const GameBuild* findBuild(const ExeIdentity& exe) {
    for (const GameBuild& build : kBuilds) {
        if (build.timestamp == exe.timestamp && build.fileSize == exe.fileSize) return &build;
    }
    return nullptr;
}

uintptr_t rebase(const ExeIdentity& exe, uint32_t va) {
    return exe.base - kLinkBase + va;
}

bool verifyCheckSite(const ExeIdentity& exe, const GameBuild& build, char* found, size_t foundSize) {
    const CheckSite& site = build.check;
    uint8_t seen[sizeof(site.bytes)] = {};
    if (found && foundSize) found[0] = '\0';
    if (site.length == 0 || site.length > sizeof(seen)) return false;
    if (!readBytes(reinterpret_cast<const void*>(rebase(exe, site.va)), seen, site.length)) {
        if (found) _snprintf_s(found, foundSize, _TRUNCATE, "(not readable)");
        return false;
    }
    if (memcmp(seen, site.bytes, site.length) == 0) return true;
    if (found) {
        size_t used = 0;
        for (size_t i = 0; i < site.length && used + 4 < foundSize; i++) {
            used += static_cast<size_t>(_snprintf_s(found + used, foundSize - used, _TRUNCATE, "%02x ", seen[i]));
        }
    }
    return false;
}

} // namespace tmplt
