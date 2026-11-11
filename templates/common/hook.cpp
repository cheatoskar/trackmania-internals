// hook.cpp - see hook.h. MIT, see LICENSE-MIT.
#include "hook.h"

#include <windows.h>

#include <cstring>

namespace tmplt {

namespace {

bool readable(const void* address, size_t length) {
    __try {
        volatile uint8_t sink = 0;
        for (size_t i = 0; i < length; i++) sink = static_cast<const volatile uint8_t*>(address)[i];
        (void)sink;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// First byte of an instruction that has a relative operand right after it (only the common
// ones; the caller is still responsible for passing whole, position-independent instructions).
bool startsRelative(const uint8_t* code) {
    const uint8_t op = code[0];
    if (op == 0xE8 || op == 0xE9 || op == 0xEB) return true;   // call/jmp rel
    if (op >= 0x70 && op <= 0x7F) return true;                  // jcc rel8
    if (op == 0xE0 || op == 0xE1 || op == 0xE2 || op == 0xE3) return true; // loop/jecxz
    if (op == 0x0F && code[1] >= 0x80 && code[1] <= 0x8F) return true;     // jcc rel32
    return false;
}

void writeJump(uint8_t* at, const void* to) {
    at[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(reinterpret_cast<intptr_t>(to) - reinterpret_cast<intptr_t>(at + 5));
    memcpy(at + 1, &rel, sizeof(rel));
}

} // namespace

bool hookSlot(void** slot, void* detour, void** original) {
    if (!slot || !detour || !original || !readable(slot, sizeof(void*))) return false;
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
    *original = InterlockedExchangePointer(slot, detour);
    VirtualProtect(slot, sizeof(void*), old, &old);
    return true;
}

bool hookInline(void* target, const uint8_t* expected, size_t length, void* detour, void** trampoline) {
    if (!target || !expected || !detour || !trampoline) return false;
    if (length < 5 || length > 16) return false;
    if (startsRelative(expected)) return false;
    auto code = static_cast<uint8_t*>(target);
    if (!readable(code, length) || memcmp(code, expected, length) != 0) return false; // unknown bytes: refuse

    // Trampoline: the copied prologue, then a jump back behind it.
    auto tramp = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!tramp) return false;
    memcpy(tramp, code, length);
    writeJump(tramp + length, code + length);
    FlushInstructionCache(GetCurrentProcess(), tramp, 64);
    DWORD old = 0;
    VirtualProtect(tramp, 64, PAGE_EXECUTE_READ, &old);

    if (!VirtualProtect(code, length, PAGE_EXECUTE_READWRITE, &old)) {
        VirtualFree(tramp, 0, MEM_RELEASE);
        return false;
    }
    *trampoline = tramp; // set before the jump goes live: the detour may run at once
    // Not atomic: a thread executing these bytes while they change can crash. Install inline hooks
    // before the game first calls the function, or from the thread that calls it.
    writeJump(code, detour);
    for (size_t i = 5; i < length; i++) code[i] = 0x90; // NOP padding up to the instruction boundary
    VirtualProtect(code, length, old, &old);
    FlushInstructionCache(GetCurrentProcess(), code, length);
    return true;
}

} // namespace tmplt
