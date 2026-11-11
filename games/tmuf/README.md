# TMUF (TrackMania United Forever)

Exe `TmForever.exe` (same file name as TMNF), PE timestamp 4b9e8add, 10,493,952 bytes, SHA-256 4b6a7b31... No `.map` symbols exist; every TMUF address here was ported from TMNF.

- Engine hook addresses (TMUF column): [../tmnf/engine-hooks.md](../tmnf/engine-hooks.md).
- Ported symbols with confidence tiers for the functions these notes use (120 rows, 103 unique): [../../data/symbols_tmuf.csv](../../data/symbols_tmuf.csv). The full port covered 2,144 symbols (1,255 unique). Use only `confidence=unique`; re-check others by masked byte compare (`scripts/re/port.py`).
- TMNF->TMUF address delta is NOT constant (viewport slots -0x280, CHmsDyna -0x130, others +-0x10..0x1f0); a "-0x500" rule of thumb in one physics note is unverified.
- Telling TMNF and TMUF apart: PE timestamp; or `Nadeo.ini` (`WindowTitle=trackmania united` -> TMUF); folder names are unreliable.

Source keys (short names for own project sources, not part of this repo): `S` = server controller of a TMNF/TMUF randomizer server project; `W` = its website; `P` = 100 TMX Bingo plugin (own TMLoader mod); `H` = TM Horizon launcher (own); `A`/`T` = ESWC autosave tool; `R` = own RE tools (ESWC); `N` = a GBX tool fork; `M` = own RE notes. "Twinkie" = third-party MIT TMNF mod whose published offsets the plugin reused.

| Fact | Value | Source | Status |
|---|---|---|---|
| exe | `TmForever.exe`, PE timestamp 0x4b9e8add, no map file; Ghidra project has it as /TMUF/TmForever.exe | own Ghidra notes | verified |
| TMUF addresses | none established in the earliest sources (stale: superseded by the ported symbols above); the plan was to port TMNF map names via Ghidra Version Tracking and check every address with a masked byte compare before use | own Ghidra notes | unverified (stale) |
| Plugin offsets on TMUF | same baseProfile (app 0x972EB8 etc.) is tried against any TmForever.exe; unknown builds validated by UID shape; TMUF vs TMNF only by folder name | P\src\game.cpp:1605 | inferred |
| Environment filter | TMX tmuf.exchange `environment` filter is 0-based: 0 Snow,1 Desert,2 Rally,3 Island,4 Coast,5 Bay,6 Stadium; the API field `Environment` in results is 1-based | S\Presets\100tmx_tmuf_snow.toml; H\packages\api\src\sources\tmx.ts (1 Snow..7 Stadium) | verified 2026-11-07 |
| Pack mask bits (server list `g`) | Snow(Alpine) 0x01, Desert(Speed) 0x02, Rally 0x04, Island 0x08, Coast 0x10, Bay 0x20, Stadium 0x40. TMF dedicated cfg `<packmask>`: "nations" for TMNF, empty for United (only TMUF owners can join) | own notes; S\ServerSetup.cs:149 | verified/inferred |
| MediaTracker/Unlimiter | TMUF/TMNF maps may use Unlimiter blocks; the TMX `inunlimiter=0` filter only excludes maps declared as such: `UnlimiterVersion` is uploader-declared metadata, so undeclared Unlimiter maps can slip through | W\docs\gbx-detection-plan.md | verified (measurement) |

## Layouts from the VR mod (RVAs, add module base)

Source keys (short names for own project sources, not part of this repo): `tmvs` = TM Vibrant Shaders mod source; `tmvs-mp` = its ManiaPlanet port; `tmvs[X]` = feature branch X of it; `tmvs-notes` = its notes; `phys` = own physics RE notes; `tmnfc` = TMNF-C (third-party MIT reimplementation, github.com/adonis-singh/TMNF-C); `dz` = own DeltaZero project; `xr` = TrackManiaForeverOpenXR (upstream jiink/TrackManiaForeverOpenXR); `tm-re` = own RE tools. Status: verified = seen in code/data or measured; inferred; unverified = prose only.

| fact | value | source | status |
|---|---|---|---|
| exe | see exe-ids; no .map; symbols exist only by porting | own Ghidra notes | verified |
| address porting method | take 48-64 bytes of TMNF function, mask bytes covered by TMNF .reloc entries (absolute addresses) and the rel32 after E8/E9, search +-64 KB around the TMNF address in TMUF, must be unique, confirm over 256 bytes; vtable slots: port the function in the slot then find the TMUF pointer to it | scripts/re/port.py; tmvs port_address.py; phys port_tmuf.py | verified |
| phys symbol port result | full port: 2144 rows (header `va_tmnf,class,method,va_tmuf,confidence,n_bytes,tier,sim256,size_tmnf`): confidence unique 1255, ambiguous 723, none 164, unique-relaxed 2; tier exact 2120, relaxed 24. TMNF side 2651 symbols, 2500 distinct (class,method). The published CSVs are reduced to the symbols the notes use (see [../../data/README.md](../../data/README.md)) | phys | verified |
| TMUF offset vs TMNF | not constant: viewport slots -0x280 (e.g. 009a0990->009a0710), CHmsDyna -0x130 (00532d40->00532c10), many TMUF engine funcs +-0x10..0x1f0. Claim "about -0x500" for the bugslide functions | measured by me from tables; bugslide.md claim | verified (deltas) / unverified (-0x500) |
| TMUF extra | 7 environments (Stadium, Island, Coast, Bay, Desert, Rally, Snow); water blocks are Stadium only; environment read at map load from decoration identifier (+0xc4/+0xc8 of CGameCtnDecoration) | tmvs\engine.cpp; docs | verified |
| Steam TMUF with TMLoader | `kSteamUnitedLayout` "Steam TMUF 2.11.26": CTrackMania vtable slot RVA 0x0073E0F8 -> function RVA 0x000859C0 (VehicleSetVisibility); ComputeClippingPlanes RVA 0x0012C940; call-site return RVAs {0x0012F705,0x0012F7A7,0x0058E7DA,0x0058E967,0x00590D74,0x00592727,0x00595126,0x00595C36,0x005975E5,0x005ABB9C} (index 3 = world clip plane); RenderTree visibility branch RVA 0x001301F6, visible continuation 0x00130201, rejected continuation 0x00130C6F. `kTmLoaderLayout` "TrackMania ModLoader 2.12.0/2.12.0-compat": slot 0x0073E100, fn 0x00085B90, ComputeClippingPlanes 0x0012CAC0, call sites {0x0012F835,0x0012F8D7,0x0058EA3A,0x0058EBC7,0x00590FD4,0x00592987,0x00595386,0x00595E96,0x00597845,0x005ABEAC}, branch 0x00130326/0x00130331/0x00130D9F. RVAs (not VAs: add module base) | xr\src\d3d9_proxy.cpp:728-760 | verified (code); how obtained not stated |
| TMUF Packs | `<TMUF install dir>\Packs` (ForeverValidator / fvlab read these) | phys notes | verified |
