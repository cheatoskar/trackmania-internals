# Music volume, sound volume and fullscreen (ESWC, Sunrise, Original)

Where TrackMania Nations ESWC 1.80, TrackMania Sunrise (Win10 community repack) and TrackMania Original (Online Patch v2 exe) keep the music volume, the sound volume and the fullscreen flag, how they reach the sound code, and how a mod can change them. The three exes share this engine code; only addresses and a few offsets differ.

Exes (link-time VAs, image base 0x00400000):

| Game | Exe | PE timestamp | SHA-256 |
|---|---|---|---|
| ESWC 1.80 | `TmNationsESWC.exe` | `5ce7272f` | `983bd12724739dcb0f876aeb3a4259154b068940f5167f57ede750ca452d8068` |
| Sunrise | `TmSunrise.exe` | `5cc8f0cf` | `e59dfc7eaa5314e85b2b0df6f40ef7ba01a23bd5a9525f84407f5dff301df4fe` |
| Original | `TMORIGINAL.EXE` | `5d337477` | `75e129d769f4595ddde54a8ef484162915bc2bf998c1a2139cc963294fe8175e` |

How established: Ghidra 12.1.2 decompilation (Sunrise, Original), capstone disassembly and byte-pattern search across the three exes, the games' reflection tables (member descriptors read from the exe), and an in-game test on ESWC. Status per row: `verified` = seen in code/data of that exe (or observed in game where noted), `inferred` = same code shape as a verified exe, not traced in this one.

## Where the values live

`CSystemConfig` is loaded from `Default.SystemConfig.Gbx` at start (the launcher's settings dialog writes that file). The application object keeps it at `app+0x18`.

| What | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|
| Application object global (`[g]`) | 0x00C945E0 | 0x00B8AB60 | 0x00B85DC0 | verified (window procedure reads it, see below) |
| `app+0x18` `CSystemConfig*` | yes | yes | yes | verified (start code passes `[esi+0x18]` to the viewport and audio init) |
| `CSystemConfig::Display` (`CSystemConfigDisplay*`) | +0x20 | +0x20 | +0x20 | verified (reflection, id `0B005001`) |
| `CSystemConfigDisplay::FullScreen` (bool, 4 bytes) | +0x34 | +0x34 | +0x34 | verified (reflection, id `0B013000`) |
| `CSystemConfig::AudioSoundVolume` (float 0..1) | +0xEC (id `0B005023`) | +0xE0 (id `0B005022`) | +0xE0 (id `0B005022`) | verified (reflection) |
| `CSystemConfig::AudioMusicVolume` (float 0..1) | +0xF0 (id `0B005024`) | +0xE4 (id `0B005023`) | +0xE4 (id `0B005023`) | verified (reflection) |
| Reflection descriptor `Display` | 0x00C08A5C | 0x00AC0E3C | 0x00ABCE3C | verified |
| Reflection descriptor `FullScreen` | 0x00C0A7C0 | 0x00AC3DE8 | 0x00ABFDE8 | verified |
| Reflection descriptor `AudioSoundVolume` | 0x00C08FAC | 0x00AC135C | 0x00ABD35C | verified |
| Reflection descriptor `AudioMusicVolume` | 0x00C08FD8 | 0x00AC1388 | 0x00ABD388 | verified |

A reflection descriptor starts `{type, member id, 0, offset, name*, ...}` (type 0x24 = float, 0x05 = node pointer, 0x01 = bool). Checking type, id and offset there is a cheap way to confirm an offset before writing.

## The audio port

| What | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|
| Engine manager global (`[m]`) | 0x00C96140 | 0x00B8B494 | 0x00B866F4 | verified |
| `[[m]+0x28]` engine array, `+0x40` audio engine (engine 0x10), its `+0x2C` `CAudioPort*` | yes | yes | yes | verified (start code, below) |
| `CAudioPort::SoundVolume` (float) | +0x14 (descriptor 0x00C712CC) | +0x14 (0x00ACB724) | +0x14 (0x00AC7714) | verified |
| `CAudioPort::MusicVolume` (float) | +0x18 (descriptor 0x00C712F8) | +0x18 (0x00ACB750) | +0x18 (0x00AC7740) | verified |
| `CAudioPort::HmsSoundVolume` (float) | +0x1C | - | - | verified (reflection) |
| `CAudioPort::ApplySystemConfig(config)` (vtable +0x8C on `CAudioPortDx8`; base part copies the two volumes, clamped to 0..1, into +0x14 / +0x18) | - | - | base 0x00497BA0, Dx8 0x005ECF80 | verified (Original), inferred (ESWC, Sunrise) |
| Volume of one buffer: the music flag of the sound (`sound+0x64`) picks `MusicVolume` or `SoundVolume`, times a constant, times the buffer's own volume, then linear -> DirectSound hundredths of dB | 0x00955110 `(float v, sound)`, reads the flag itself; also times `HmsSoundVolume` when `sound+0x54` is set | 0x005EF480 `(float v, int isMusic)` | 0x005EEE40 `(float v, int isMusic)`, callers pass `sound+0x64` | verified (ESWC, Original); Sunrise by byte pattern |
| Per-frame port update: computes that volume for every playing buffer (and treats very low volumes, about -95 dB and below, as not audible) | callers 0x00954698, 0x00954835 | - | 0x005EE030 (vtable +0xB4) | verified (Original), inferred (ESWC) |
| Buffer volume update: calls `IDirectSoundBuffer::SetVolume` only when the computed value differs from the cached one | 0x00958808 .. 0x0095883E | - | 0x0071A400 | verified |

## Start order (why a value written early is used by the game's own init)

All three run the same application init: input port first, then the viewport, then the audio port.

| Step | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|
| Input port created (`DirectInput8Create` runs in here; a `dinput8.dll` proxy gets control at this point) | 0x00425275 | 0x0041146E | 0x0041149E | verified |
| Viewport init with the config: `mov ecx,[esi+vp]; ...; push [esi+0x18]; call [edx+init]` (Direct3D device, window mode) | 0x004252D3 (`+0x11C`) | 0x00411598 (`+0x110`) | 0x004115C8 (`+0x110`) | verified |
| `engines->audio->port->ApplySystemConfig(app->config)` (`a1 <m> 8b 48 28 8b 51 40 8b 4a 2c 8b 56 18 8b 01 52 ff 90 8c 00 00 00`) | 0x004252ED | 0x004115B2 | 0x004115E2 | verified |

So values written into the `CSystemConfig` from inside the first `DirectInput8Create` call are used by the game's own viewport and audio init of that start.

## Changing them

- **Music and sound volume, live:** write the two `CSystemConfig` floats and the two `CAudioPort` floats (what `ApplySystemConfig` does). The per-frame update picks the new value up. Observed on ESWC: a sound volume change is heard at once in a race; a music volume change made during a race was heard once the main-menu music played. Status: verified (ESWC, in game); Sunrise and Original inferred from the same code.
- **Changing the volume on every frame while a slider is dragged makes ESWC stutter.** Every change makes the update set the DirectSound volume of every playing buffer again, and crossing the "not audible" threshold stops or restarts sounds. Applying at most about ten times a second, plus the final value, avoids it. Status: verified (ESWC, in game).
- **Fullscreen:** there is no live path in the shipped games. The window procedure has a `ToggleFullscreen` command (logs `[GbxWin32App] ToggleFullscreen begins/ends`) that calls viewport vtable +0x9C (to fullscreen) / +0xA0 (to windowed), but it only runs for a `WM_COMMAND` id kept in the application object (`app+0x74` ESWC, `app+0x7C` Sunrise/Original), which the constructor sets to `0xFFFFFFFF` and nothing else writes. `WM_SETFOCUS` / `WM_KILLFOCUS` only log while fullscreen. Writing `FullScreen` in the config takes effect at the next start (if it is written before the viewport init, as above). Status: verified (window procedure ESWC 0x004258BD, Sunrise 0x0043D49D, Original 0x0043D67D; helpers Sunrise 0x00411700 IsFullscreen, 0x004116A0 / 0x004116D0).
- **Persistence:** the game writes `Default.SystemConfig.Gbx` right after loading it at start (ESWC: `CSystemArchiveNod::Save` call at 0x00425010), not while it runs, so changes made in memory are lost at exit. Status: verified (ESWC call site), inferred (no other writer seen).
- **Master volume:** none of the three games has one. The per-process Windows audio session (`IAudioSessionManager::GetSimpleAudioVolume(NULL, FALSE)` on the default render device) controls the game's whole output, since DirectSound plays in the process's default session. Status: verified (Windows API behaviour, tested in a test program).
