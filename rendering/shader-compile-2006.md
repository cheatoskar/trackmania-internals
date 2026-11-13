# Shader compile path of the 2006 engine (Nations ESWC, Sunrise, Original)

How the 2006-engine games turn HLSL into a shader: the shader's compile method, the D3DX functions linked into the
exe, the include handler, and what a disk cache in that path has to respect. Also the large-address-aware state of
the three exes. Complements [frame-loop.md](frame-loop.md) (when compiles happen: first bind of a material).

Builds (identify yours with [../scripts/re/pe_info.py](../scripts/re/pe_info.py); see the [README](../README.md#coverage)):
ESWC unofficial 1.80 `5ce7272f` (`983bd127...`), Sunrise community repack `5cc8f0cf` (`e59dfc7e...`), Original
Online Patch v2 `5d337477` (`75e129d7...`). Addresses are link-time VAs (image base 0x00400000; these exes have no
relocations).

Method: Ghidra 12.1.2 headless (ESWC and Sunrise decompiled), string and constant references (`Compilation of
EntryPoint`, the `CTAB` four-cc 0x42415443), then each function compared instruction by instruction across the
three exes with capstone (operands equal, call/jump targets mapped consistently). Static only, nothing run.
`verified` = seen in that exe's code; `inferred` = a reading of the code that was not run.

## Functions

| Function | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|
| Shader compile method (thiscall, the shader object; called only through two shader vtables; ESWC thunk 0x0051AA50 `jmp`) | 0x00581220 | 0x00703620 | 0x007028A0 | verified (ESWC/Sunrise decompiled; Sunrise vs ESWC: same code, other field offsets; Original vs Sunrise: same code, one re-encoded `sub esp` / `add esp`) |
| `D3DXCompileShader` (stdcall, 10 arguments, `ret 0x28`; one call site, inside the compile method) | 0x0087AA6B | 0x008E3292 | 0x008DFD8F | verified |
| `D3DXCreateBuffer` (stdcall, `ret 8`) | 0x008A110E | 0x00915F23 | 0x00912A24 | verified |
| `D3DXGetShaderConstantTable` (stdcall, `ret 8`; creates the table object and parses the `CTAB` comment block) | 0x0087B669 | 0x008E3E90 | 0x008E098D | verified |
| `D3DXGetShaderSamplers` (the `CTAB` sampler walk, up to 16) | 0x0087B49D | - | - | verified (ESWC) |
| Post-compile step (fastcall, the shader): samplers, input/output semantics and constants read from the bytecode; calls `D3DXGetShaderConstantTable` itself when the shader has no table yet | 0x00580A40 | caller of 0x008E3E90 at 0x00702E1D | caller of 0x008E098D at 0x0070209D | verified (ESWC), inferred (Sunrise/Original by the call) |
| Include handler `Open` / `Close` (vtable ESWC 0x00AE3EA8) | 0x005803F0 / 0x00580740 | 0x00702780 / - | 0x00701A00 / - | verified (`Open` identical in all three) |

The three D3DX functions are the D3DX library code linked statically, byte-identical in all three exes apart from
call distances. Each starts with the 5-byte hot-patch prologue `8b ff 55 8b ec` (`mov edi,edi; push ebp; mov
ebp,esp`); the bytes before them belong to the previous function (`c2 1c 00`), so there is no hot-patch padding.
Start bytes up to the first relative call, the same in all three exes:

| Function | Bytes |
|---|---|
| `D3DXCompileShader` | `8b ff 55 8b ec 81 ec 70 03 00 00 53 56 33 f6 57 8d 8d 90 fc ff ff 89 75 fc 89 75 f8` |
| `D3DXCreateBuffer` | `8b ff 55 8b ec 53 8b 5d 0c 85 db 75 07 b8 6c 08 76 88 eb 49 56 6a 10` |
| `D3DXGetShaderConstantTable` | `8b ff 55 8b ec 53 8b 5d 0c 56 33 f6 3b de 57 74 02 89 33 8b 7d 08 3b fe 74 04 3b de 75 07 b8 6c` |

## What the compile method does

| Fact | Status |
|---|---|
| Asks the shader whether it is a vertex shader (`IsA(0x09074000)`), releases an old constant table, then builds a profile table on the stack: `vs_1_1`, `vs_2_0`, `vs_2_a`, `vs_2_sw`, `vs_3_0` / `ps_1_1` ... `ps_2_0`, `ps_2_b`, `ps_2_a`, `ps_2_sw`, `ps_3_0`, each with its version token. | verified |
| Entry point name from a shader virtual (+0xB4), source text from the shader (ESWC +0x14 length, +0x18 text). Defines: the shader's own list plus `DGbxVersion`, NULL-terminated (ESWC 0x0057F550). | verified |
| Tries the profiles from the last table entry down; a profile is skipped when the viewport's profile check says no or the profile name is not found in the source (the sources carry one `vs_x_y VS_OUTPUT vsMain(...)` / `ps_x_y PS_OUTPUT psMain(...)` line per profile). One `D3DXCompileShader` call per remaining profile until one returns 0. | verified (the search is a substring find on the source string object) |
| `D3DXCompileShader` arguments: source, length, defines, the include handler object (on the stack: vtable, shader, the two common include texts), entry point, profile, the flags from the viewport's pre-compile callback, and out pointers for shader, error buffer and constant table. | verified |
| Success (result 0, shader buffer and constant table both set): the bytecode is copied into the shader (ESWC +0xB4 dword count, +0xB8 data), the constant table kept (ESWC +0xBC, Sunrise/Original +0xB8), then the post-compile step and the viewport's post-compile callback (creates the D3D shader). The error buffer is released unread. | verified |
| All profiles failed: log `Compilation of EntryPoint "<name>" failed` and `Shader compilation error` with the D3DX error text (or `Don't know why`). | verified |
| Three callbacks set by the Dx9 viewport (ESWC writes at 0x007D2F40, globals 0x00CAAFB8 / 0x00CAAFBC / 0x00CAAFC0): pre-compile 0x007E9050 (gets the shader, the flags and the define list; returning 0 skips the compile), profile check 0x007E92D0 (gets the version token), post-compile 0x007E93A0. | verified (writes, call sites); roles inferred |
| Inside `D3DXCompileShader` the compiler makes its constant table with `D3DXGetShaderConstantTable` on its own output buffer (ESWC call at 0x0089D1BD), so a table made the same way from the same bytes is equivalent. | verified |

## Include handler

| Fact | Status |
|---|---|
| System includes (`#include <Common.VHlsl.txt>`, `<Common.PHlsl.txt>`; matched case-insensitively): `Open` returns a copy of a text the engine holds in memory (`malloc` + copy); `Close` frees it. No other effect. | verified |
| Local includes (`#include "..."`): `Open` resolves the name against the engine's file system, reads the file **and appends the file to a dependency list of the shader** (handler +4 = the shader). | verified (code), purpose of the list inferred |
| Unknown system include names: error 0x88760866. | verified |

## For a disk cache in the compile path

Consequences of the facts above (inferred, from the code; a cache built this way passed unit tests on stand-in
functions but has not been run in the games yet):

- `D3DXCompileShader` is the only place that turns source into bytecode, with one caller; a hook there sees every
  compile with all inputs (source, defines, entry point, profile, flags).
- Its result depends on those inputs, the D3DX build (one per exe) and the text of the includes. System includes can
  be re-read without side effects to check them; a compile that opened a local include cannot be skipped without
  also skipping the dependency the engine records.
- A cached answer needs a buffer (`D3DXCreateBuffer`) and a constant table (`D3DXGetShaderConstantTable` on that
  buffer); the engine reads nothing else from the call. A failed profile (`E_FAIL` with an error buffer) is part of
  the normal loop and can be answered the same way.

## Large address aware

| Exe | `IMAGE_FILE_HEADER.Characteristics` | LAA (0x0020) | PE checksum | Status |
|---|---|---|---|---|
| ESWC `983bd127...` | 0x030F (relocations stripped, executable, 32-bit, debug info stripped) | no | 0 | verified (file header) |
| Sunrise `e59dfc7e...` | 0x030F | no | 0 | verified |
| Original `75e129d7...` | 0x030F | no | 0 | verified |
| TMNF `3847cf9f...` / TMUF `4b6a7b31...` | 0x0102 | no | 0x009F4A9E / 0x009F5C5C | verified |

The flag is at file offset 0x56 in the three 2006 exes (`e_lfanew` 0x40 + 0x16). Setting it changes that one byte
(0x0F -> 0x2F) and nothing else, since the checksum field is 0 (Windows does not check the checksum of an exe). The
SHA-256 of the exe changes, so anything that identifies the exe by hash has to know the new value; tools that key
on the PE timestamp are unaffected. Whether the games behave differently with 4 GB of address space has not been
tested.
