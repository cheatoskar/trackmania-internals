# Sunrise and Original reset every input binding at start

TrackMania Sunrise (Win10 repack, `5cc8f0cf`) and TrackMania Original (Online Patch v2, `5d337477`) throw away all saved input bindings (keyboard and pads) every time they load the profiles at start. The bindings are saved correctly; the next start replaces them with the defaults, and the next profile save writes those defaults to the file. Nations ESWC does not do this.

Status: found statically in both exes; the fix below was tested in the game on Sunrise on 2026-11-14 (keyboard and pad bindings stay after a restart, also with the pad plugged in after start). Original: static analysis and an exe check only.

## Bindings config

`CInputBindingsConfig` (class 0x13006000, 0x50 bytes):

| Offset | What |
|---|---|
| +0x14 / +0x18 | entries {action index, device id, object}, 12 bytes each |
| +0x20 / +0x24 | actions |
| +0x2c / +0x30 | ids of the devices that have bindings |
| +0x38 / +0x3c | action names from the file, 16 bytes each |
| +0x44 | string ("Race") |
| +0x4c | input port |

In the profile file (`GameData\Profiles\<name>.Profile.Gbx`, body LZO-compressed):
- chunk 0x13006000: a string, then per entry the object (u32), the device id as a MwId, the action type (u32), a u32 and the action name (string);
- chunk 0x13006001: the id list.

Device id: a MwId with the 0x80000000 mark. Its first occurrence is followed by the DirectInput instance GUID as text (`UuidToStringA`, lower case), later ones are back references. Read through `0x43E480(string) | 0x80000000`, the same function the device init uses (`0x4CF2A0`), so the ids match. The instance GUID itself comes from `HKCU\System\CurrentControlSet\Control\MediaProperties\PrivateProperties\DirectInput\VID_xxxx&PID_xxxx\Calibration\0` and stays the same between starts (verified with an Xbox One pad, VID 045E PID 02FF).

## What the profile load does

Sunrise: CTrackMania start `0x816240` -> `0x8211F0` (Original `0x81F7xx`), per `CConstructionPlayerProfile` (class 0x2404A000, bindings config at +0x68):

1. register the actions (`0x821C30`), resolve the action names (`0x4D0570`);
2. if profile+0x128 < 1: set +0x128 = 1 and call `0x821D20`, which calls `0x4CFF80(config, id -1)` (removes every entry and every id) and then the defaults pass `0x821D50` for the devices that are there;
3. otherwise only the defaults pass.

profile+0x128 is set only in the constructor (to 0) and in that check. Saving and loading the profile (Sunrise `0x79EA00`, Original `0x79D850`, with everything they call) never touch +0x128. So step 2 runs at every start and every binding is gone.

ESWC (`0x716790`...): register the actions, resolve the names, defaults pass. No removal; bindings stay.

## The fix used in the Horizon Client and Legacy Essentials

Redirect that one call so it runs the defaults pass alone, which is what ESWC does:

| Exe | Call (rel32) | Original target | New target |
|---|---|---|---|
| Sunrise | `0x00821335` | `0x821D20` | `0x821D50` |
| Original | `0x0081F815` | `0x820200` | `0x820230` |

Both exes hold the same 15 bytes there: `8b442410 8b4068 50 8bce e8 e6090000`. Same calling form (thiscall, one argument, ret 4). The patch has to be in place before the profiles are loaded: the three exes import `dinput8.dll` statically, so a dinput8 loader runs early enough (checked: the Horizon host starts before the game calls `Direct3DCreate9`). The code refuses if app+0x178 (loaded profiles) is already > 0.

Source: static analysis with Ghidra and capstone on both exes; profile file decompressed and read; in-game test on Sunrise (Horizon Client branch `sunrise-login-bindings`).
