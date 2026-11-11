// builds.h - the exe builds the templates accept. MIT, see LICENSE-MIT.
//
// Hand-written from the Coverage table in the repository README (README.md#coverage) and the
// per-game pages. When you add a build, take every value from those tables (or from your own
// Ghidra/pe_info.py check of the exe) and keep the source in the comment next to it.
//
// A build is recognised by PE timestamp AND file size. After that, one "check site" is read
// (never written): the first bytes of a function the tables list with its prologue. If they
// differ, the exe is patched, another mod hooked that function, or the table row is wrong, and
// the template installs nothing.
#pragma once

#include <cstddef>
#include <cstdint>

namespace tmplt {

struct CheckSite {
    const char* name;      // function, as named in the table
    uint32_t va;           // link-time VA (image base 0x00400000)
    uint8_t bytes[8];      // expected first bytes (whole instructions)
    size_t length;
};

struct GameBuild {
    const char* name;
    uint32_t timestamp;    // PE header TimeDateStamp
    uint32_t fileSize;     // size of the exe on disk, bytes
    CheckSite check;
};

// clang-format off
constexpr GameBuild kBuilds[] = {
    // TrackMania Nations Forever 2.11.26, SHA-256 3847cf9f... (README Coverage).
    // Check site: CTrackManiaRace::RespawnPlayer, games/tmnf/engine-hooks.md (prologue list).
    {"TMNF 2.11.26", 0x4d431494, 10498048,
     {"CTrackManiaRace::RespawnPlayer", 0x0047c0d0, {0x53, 0x8b, 0x5c, 0x24, 0x08}, 5}},

    // TMLoader 2.12.0 / 2.12.0-compat ship their own TmForever.exe (used for Nations and United):
    // same PE timestamp as TMNF, 10,685,290 bytes, .text identical to TMNF (README Coverage notes).
    // The default 2.12.0 profile relocates it: always rebase.
    {"TMLoader 2.12.0 TmForever (TMNF code)", 0x4d431494, 10685290,
     {"CTrackManiaRace::RespawnPlayer", 0x0047c0d0, {0x53, 0x8b, 0x5c, 0x24, 0x08}, 5}},

    // TrackMania United Forever 2.11.26, SHA-256 4b6a7b31... (README Coverage).
    // Check site: TMUF column of games/tmnf/engine-hooks.md.
    {"TMUF 2.11.26", 0x4b9e8add, 10493952,
     {"CTrackManiaRace::RespawnPlayer", 0x0047bed0, {0x53, 0x8b, 0x5c, 0x24, 0x08}, 5}},

    // TrackMania Nations ESWC, unofficial 1.80 community build, SHA-256 983bd127...
    // (games/tmn-eswc/addresses.md). Other ESWC exes have the same timestamp but other code.
    {"TMN ESWC unofficial 1.80", 0x5ce7272f, 8987136,
     {"CTrackManiaRace::OnFinishLine", 0x006d9f50, {0x8b, 0x44, 0x24, 0x04, 0x83, 0xec, 0x1c}, 7}},

    // TrackMania Sunrise, Win10 community repack, SHA-256 e59dfc7e... (games/tm-sunrise/README.md).
    // Check site: games/tm-sunrise/race-events.md (static only, not run).
    {"TM Sunrise Win10 repack", 0x5cc8f0cf, 7904768,
     {"CTrackManiaRace::OnStartLine", 0x007ec550, {0x8b, 0x44, 0x24, 0x04, 0x56}, 5}},
};
// clang-format on

} // namespace tmplt
