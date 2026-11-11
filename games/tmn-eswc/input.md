# TMN ESWC and Sunrise: DirectInput devices, bindings, device loss

How the input engine of TrackMania Nations ESWC (unofficial 1.80, SHA-256 983bd127..., timestamp 5ce7272f) finds game controllers, where it keeps them, how bindings point at them and what happens when one is unplugged. TrackMania Sunrise (Win10 repack, SHA-256 e59dfc7e..., timestamp 5cc8f0cf) has the same code (source path `CodeBaseTMORC3` instead of `CodeBaseTMN`, more log strings); its addresses are in the second column.

Established 2026-11-11 by static analysis only (Ghidra 12.1.2 headless projects plus capstone byte checks). Nothing here was run in the game yet. Used by a controller-hotplug module of the author's launcher, which checks every code byte it relies on at start.

Status: verified = seen in code bytes/decompiled code and cross-checked in both exes; inferred = reading of the code, not run.

## The short version

- The game enumerates game controllers **once**: `CInputPortDx8::Init` calls `IDirectInput8W::EnumDevices(DI8DEVCLASS_GAMECTRL, callback, port, DIEDFL_ATTACHEDONLY)`. Nothing calls EnumDevices again. A pad plugged in later is never seen (the reason for "restart the game").
- Devices live in the port's device list: mouse, keyboard, then the pads in enumeration order.
- Bindings are stored per device **id** = hash of the DirectInput instance GUID string `| 0x80000000`. At race start they are turned into binding tables that address devices by **index** in the device list.
- Unplug: the port marks the device lost and calls Acquire on it every update; the slot and the bindings stay.

## Functions

| What | ESWC | Sunrise | Status |
|---|---|---|---|
| input engine: create the port once, store the global (`mov [port global], edx`) | 0x00461060 (store at 0x004610d5) | 0x0043f100 (store at 0x0043f175) | verified |
| `CInputPortDx8::Init(CSystemWindow*)` (DirectInput8Create, mouse, keyboard, EnumDevices at the end) | 0x00464290 | 0x004cde30 | verified |
| EnumDevices callback `BOOL CALLBACK(const DIDEVICEINSTANCEW*, void* port)`: `mov eax,[esp+4]; mov ecx,[esp+8]; push eax; call add-pad; ret 8` | 0x004647d0 | 0x004ce7c0 | verified |
| add pad (thiscall port, inst; ret 4): CreateDevice(inst+4 = guidInstance), SetDataFormat(c_dfDIJoystick2), SetCooperativeLevel, SetProperty(BUFFERSIZE 256), new CInputDeviceDx8Pad(dev, &inst->guidProduct) (0x184 bytes), append to the device list; logs DirectInput errors to the game log and returns 1 | 0x00464070 | 0x004cdb60 | verified |
| CInputDeviceDx8Pad ctor | 0x00467090 | 0x0069ae00 | verified |
| device base init: GetCapabilities, GetDeviceInfo, name, id = hash(UuidToStringA(guidInstance)) \| 0x80000000 at +0x30 | 0x00465210 | - | verified (ESWC) |
| port update (vtable slot 0xb0): per device Poll + GetDeviceData; lost handling; binding lookup per (device index, object) | 0x004647f0 | 0x004ce7e0 | verified |
| reacquire (Acquire, loops on DIERR_INPUTLOST; device+0x14 = 0 ok / 1 failed) | 0x004653b0 (via 0x00468460) | - | verified (ESWC) |
| find device by id -> index, device | 0x004602b0 | - | verified (ESWC) |
| create binding table (port vtable +0x90; arg 1 = "all objects" table 0x00460790) | 0x00460300 | 0x004966d0 | verified |
| add binding to the last created table (port vtable +0x94; action, device index, object) | 0x004604e0 | 0x004968b0 | verified |
| set current table (port vtable +0x9c), delete table (+0xa0), delete all (+0xa4) | 0x00460640 / 0x004606d0 / 0x00460750 | 0x00496a10 / 0x00496aa0 / 0x00496b20 | verified |
| table lookup (table vtable slot 0): no bounds check on the device index | 0x00460670 | - | verified (ESWC) |
| Reserve of a `{count, ptr, cap}` array of 8-byte items (thiscall n; ret 4; game malloc/free) | 0x00551be0 | 0x0060a070 | verified |
| bindings config -> table: for each saved (action, device id, object) find the device by id, AddBinding with its index | 0x00466300 (wrapper 0x004662d0) | - | verified (ESWC) |
| race init of input (race vtables): config table -> race+0x144, defaults, "all objects" table -> race+0x148 | 0x006d5770 | - | inferred (ESWC) |
| input settings menu: lists devices, names pads " (Pad %d)" | 0x00781220 | - | inferred (ESWC) |

## Globals and data

| What | ESWC | Sunrise | Status |
|---|---|---|---|
| `CInputPortDx8*` global (written once after Init returned; read nowhere in the exe, the engine keeps its own pointer) | 0x00c95184 | 0x00b8bb88 | verified |
| CInputPortDx8 vtable (size 0xac) | 0x00acfe40 | 0x00a23d18 | verified |
| CInputDeviceDx8Pad vtable (slot 0x88 returns 2 = pad) | 0x00ad0038 | 0x00a40af8 | verified |
| pad data format (c_dfDIJoystick2 copy: 0x18, 0x10, DIDF_ABSAXIS, 0x110, 164 objects) | 0x00b53c5c | 0x00a6cc9c | verified |
| IID_IDirectInput8W (the game uses the W interfaces) | 0x00b54500 | 0x00a70d8c | verified |
| CMwEngineMain* -> +0x28 -> +0x30 -> +0x20 = game HWND (the port reads its window this way) | 0x00c96140 | 0x00b8b494 | verified |

## Struct members

| Object | Offset | Meaning | Status |
|---|---|---|---|
| CInputPortDx8 | +0x14 / +0x18 | binding tables the port owns: count / array | verified |
| CInputPortDx8 | +0x20 | current binding table (update does nothing while 0, except in capture mode) | verified |
| CInputPortDx8 | +0x24 | first device (the mouse) | verified |
| CInputPortDx8 | +0x2c / +0x30 | device list: count / array of CInputDevice* (mouse, keyboard, pads) | verified |
| CInputPortDx8 | +0x34 | mode, 2 = capture (input settings "press a key": raw events with device index) | inferred |
| CInputPortDx8 | +0x40 | event queue object | inferred |
| CInputPortDx8 | +0x94 / +0x98 / +0x9c | IDirectInput8W* / config (+0x84 != 0: exclusive) / HWND | verified |
| CInputDevice | +0x14 | lost: 1 = not acquired; the update calls reacquire every time | verified |
| CInputDevice | +0x2c | hash(guidProduct) \| 0x80000000 (pad ctor) | verified |
| CInputDevice | +0x30 | device id = hash(guidInstance) \| 0x80000000; bindings refer to this | verified |
| CInputDevice | +0x34 | IDirectInputDevice8W* (only reference: update, reacquire, dtor) | verified |
| binding table | +0x14 {count, ptr, cap} | one (start, count) range per device index into the bindings | verified |
| binding table | +0x20 {count, ptr, cap} | bindings (action, object), 8 bytes each, grouped by device | verified |
| race (0x006d5770's this) | +0x140 / +0x144 / +0x148 | input port / config table / "all objects" table | inferred, see contradictions #23 |

## Device loss (unplug)

In the port update, `Poll` or `GetDeviceData` failing with DIERR_INPUTLOST, DIERR_NOTACQUIRED or E_ACCESSDENIED (other app has priority) sets device+0x14 = 1 and calls reacquire. DIERR_UNPLUGGED (0x80040209) also sets +0x14 = 1 but is not logged. Every later update calls reacquire again for that device until Acquire works; then +0x14 = 0 and the port resets its queue and refreshes the analog states (0x004611e0, 0x0045fde0). The device object, its index and its bindings are never removed while the game runs. Whether DirectInput reopens a replugged HID pad on the old device object depends on Windows; if not, the slot stays lost forever (controller-hotplug then gives the slot a fresh device for the same GUID).

## Adding a device later (what controller-hotplug does)

1. Call the EnumDevices callback with a `DIDEVICEINSTANCEW` (guidInstance, guidProduct, names) and the port: the game's own path for start-up pads.
2. Every binding table of the port (+0x14/+0x18) still has one range per old device. The lookup in the update has no bounds check, so each table gets ranges up to the new device count through the game's Reserve; new ranges start at the table's binding count with count 0. That is exactly what 0x00460300 writes when it creates a table for more devices.
3. The pad's bindings (saved by id) are applied by the game at the next race init. Adding them to the running race's table is not possible cleanly: AddBinding always writes into the last created table (the "all objects" one), not the config table.

Do on the game's main thread only (the update walks the device list).
