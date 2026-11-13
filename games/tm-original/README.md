# TrackMania Original (Online Patch v2 community exe)

TrackMania Original (2006) runs on the TM Sunrise engine. The only build covered here is the exe of the community **"TrackMania Original Online Patch v2"** by GreffMASTER (2023). Its readme says it is a modified `TMORIGINAL.EXE` that changes the obsolete master server address `www.trackmania.com` to `game.trackmania.com`, made on top of the community build without StarForce. Nothing in this folder covers the retail exe, the master server or the login.

Pages: [addresses.md](addresses.md) (functions, globals, members), [engine-hooks.md](engine-hooks.md) (how a mod gets into the process: loader, dgVoodoo, Direct3D 9, DirectInput, race events).

## Exe identification

| Item | Value | Source | Status |
|---|---|---|---|
| File | `TMORIGINAL.EXE` (upper case in the install) | install folder | verified |
| Size | 7,883,264 bytes | file | verified |
| SHA-256 | `75e129d769f4595ddde54a8ef484162915bc2bf998c1a2139cc963294fe8175e` | [scripts/re/pe_info.py](../../scripts/re/pe_info.py) | verified |
| PE timestamp | `5d337477` (2019-07-20), the same year as the ESWC (`5ce7272f`) and Sunrise (`5cc8f0cf`) community builds | pe_info.py | verified |
| Image base / size of image | 0x00400000 / 0x007d9000, no `.reloc` section, no ASLR flag: always loads at 0x00400000 | pe_info.py | verified |
| Sections | `.text`, `.rdata`, `.data`, `.rsrc` | pe_info.py | verified |
| Version resource | none (the exe carries no VERSIONINFO) | pefile | verified |
| In-game version string | `0.1.5.0` (string next to `online_game` and `TmOriginal`; Sunrise's repack has the same `0.1.5.0`) | string at 0x00ab37a8 | verified (string), meaning inferred |
| Registry key | `Software\Nadeo\TmOriginal` (Sunrise: `Software\Nadeo\TmSunrise`) | string in the exe | verified (string) |
| Provenance | GreffMASTER, "TrackMania Original Online Patch, Version 2" (2023); "Version 2: Fixed the exe using the correct version as a base" | `OnlinePatchReadMe.txt` of the patch | unverified (readme claim) |
| Engine | Sunrise generation: 521 reflected classes (Sunrise 522; only `CHmsAnalyzerTraj` is missing), class ids `0x24...` like ESWC/Sunrise; 496 of the 514 named classes have the same members and offsets as Sunrise (the 18 others: race classes shifted by Sunrise-only analyser members, `CTrackMania` stunt tuning, `CGameAdvertisingElement`) | [scripts/re/classinfo.py](../../scripts/re/classinfo.py) on both exes, compared 2026-11-11 | verified (static) |

Do not key on the PE timestamp alone: the readme of the patch mentions a broken version 1 exe, and other repacks may keep the timestamp. Use the SHA-256 (and the size as a quick pre-check).

## Imports (what the exe needs next to it)

| DLL | Imported | Note | Status |
|---|---|---|---|
| `d3d9.dll` | `Direct3DCreate9` (IAT 0x00a16398) | the install ships dgVoodoo 2.8.2's `d3d9.dll` (D3D9 on D3D11), which is loaded instead of the system one | verified |
| `dinput8.dll` | `DirectInput8Create` (IAT 0x00a1606c) | static import: a `dinput8.dll` proxy next to the exe is loaded | verified |
| `m4d.dll` | 14 `MMT_*` functions | Massive Incorporated in-game advertising client (`MassiveAdClient` in its export names, PE timestamp 2005-07-21). Static import: the game does not start without the file. Sunrise ships the same file name | verified (imports/exports) |
| `wininet.dll` | `HttpOpenRequestA` (IAT 0x00a162d4), `InternetSetOptionA` (0x00a162dc), ... | online requests | verified |
| `kernel32`, `user32` (no `CreateWindowEx*`), `gdi32`, `advapi32`, `avifil32`, `dsound`, `oleaut32`, `ole32`, `rpcrt4`, `winmm`, `ws2_32` | | window functions are loaded at run time, as in Sunrise (`unicows.dll` ships in the folder) | verified |
| not imported | `tmo.exe`, `tmo.dll`, `tmo.drv`, `tmo.*.dat`, `TmOriginalLauncher.exe` | files of the retail launcher and the settings launcher; not needed by `TMORIGINAL.EXE` according to its import table; not analysed | verified (imports) |

## Other files in the patched install

| File | Size | SHA-256 | What | Status |
|---|---|---|---|---|
| `d3d9.dll` | 468,992 | `e13f7dfd...a326` | dgVoodoo 2.8.2 Direct3D 9 (version resource "dgVoodoo 2.8.2 - Direct3D9"), with `DDraw.dll` and `dgVoodoo.conf`, `dgVoodooCpl.exe` | verified |
| `m4d.dll` | 86,016 | `74e3b44c...fafd` | Massive ad client, see above | verified |
| `TrackManiaServer.exe` | 5,378,048 | `bc5a96e8...b065` | dedicated server, PE timestamp `447c11b4` (2006-05-30, the 2006 server, see [../../server/old-2006-server.md](../../server/old-2006-server.md)); modified by the same patch per its readme | verified (file), unverified (what was changed) |
| `TmOriginalLauncher.exe` | 2,568,192 | `ef1377ac...6c96` | settings launcher ("Launcher MFC Application", 2003) | verified |
| `msvcr71.dll`, `unicows.dll` | | | runtime DLLs | verified |

## Folder layout (tells it apart from other games)

`GameData\` holds `Alpine`, `Island`, `Rally`, `Speed`, `Techno` (environment folders), `Tracks\Campaigns`, `Tracks\Challenges` (`Downloaded`, `My Challenges`), `Tracks\Replays`, `Profiles`, `Scores`, `TrackManiaFull.TrackMania.Gbx`. Maps and replays live under the exe folder (`GameData\Tracks\...`), as in ESWC and Sunrise, not in Documents as in Forever. Status: verified (this install).

## Patch readme and system-wide settings

The patch readme asks users to change Windows Internet Options for the whole system: allow software with invalid signatures, turn off the certificate revocation check, turn on TLS 1.0. The same two transport changes that matter for HTTPS (TLS 1.0 allowed, revocation check off) can be made inside the game process only, as the ESWC login fix does ([../tmn-eswc/addresses.md](../tmn-eswc/addresses.md), "online"): add TLS 1.0 to the `SecureProtocols` value WinINet reads, and set `SECURITY_FLAG_IGNORE_REVOCATION` on the game's HTTPS requests through its `HttpOpenRequestA` import slot 0x00a162d4. Status: inferred (same mechanism as ESWC; not tested against the server). The "invalid signature" option is an Internet Explorer download/ActiveX setting and is probably not needed by the game (unverified).

Rendering (material per draw, delayed draws, point lights; with ESWC and Sunrise): [../../rendering/2006-engine-materials-lights.md](../../rendering/2006-engine-materials-lights.md).
