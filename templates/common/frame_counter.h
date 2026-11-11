// frame_counter.h - the example hook: counts presented frames and logs the frame rate every 10 s.
// MIT, see LICENSE-MIT.
//
// It hooks IDirect3DDevice9::Present through the device vtable (memory of d3d9.dll or of an overlay
// that wraps it, not the game's), and IDirect3D9::CreateDevice so the device the game creates after
// the install is covered. A throwaway probe device also covers an existing game device where all
// devices share one vtable (plain d3d9.dll); with overlays that give each device its own vtable, only
// devices created after the install are seen.
// It draws nothing, reads nothing from the game and changes nothing the game sees.
#pragma once

namespace tmplt {

// Creates a throwaway 16x16 device to find the vtables, then swaps the two slots. Call from a
// worker thread, never from DllMain. Safe to call once.
bool frameCounterInstall();

// Hooks Present for a device you got some other way (an IDirect3DDevice9*). No-op if it is hooked.
void frameCounterAttach(void* device);

// Frames seen so far (all hooked devices).
unsigned long frameCounterFrames();

} // namespace tmplt
