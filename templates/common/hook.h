// hook.h - two small hook helpers. MIT, see LICENSE-MIT.
//
// Prefer, in this order: engine reflection by name, a vtable/IAT slot, then an inline hook
// (docs/building-mods.md). Both helpers refuse rather than guess.
#pragma once

#include <cstddef>
#include <cstdint>

namespace tmplt {

// Swaps one pointer (a vtable or import slot) for `detour` and returns the previous value in
// `original`. The previous value is whatever is there now: if another mod swapped the slot
// first, your detour calls into theirs, which is what you want.
bool hookSlot(void** slot, void* detour, void** original);

// Inline hook with a byte-exact prologue check.
//
// `expected`/`length` are the first bytes of the target as your table lists them: whole
// instructions only, 5..16 bytes, no relative operands (no E8/E9/EB/Jcc/RIP-style operand inside
// the copied bytes). Absolute operands are fine if you put the REBASED value into `expected`
// (e.g. the SEH handler in `6a ff 68 <handler>`). If the bytes in memory differ in any way
// (other build, another mod's jump, a typo in the table), nothing is written and false is returned.
//
// On success `trampoline` runs the copied prologue and jumps back to target + length; call it
// as the original function.
bool hookInline(void* target, const uint8_t* expected, size_t length, void* detour, void** trampoline);

} // namespace tmplt
