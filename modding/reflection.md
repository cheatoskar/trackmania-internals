# Engine reflection (CMwClassInfo / SMwParamInfo)

Every Nadeo game of this generation (TMN ESWC, TM Sunrise, TMNF/TMUF; very likely TM Original) ships a reflection table for its engine classes: class name, class id, parent, and per class a list of members (fields, sub-nods, methods) with names. Openplanet builds its scripting on the same idea for ManiaPlanet/TM2020. Status: verified statically for ESWC 1.80, Sunrise repack and TMNF; runtime walker not written yet.

## Layout (32-bit games)

CMwClassInfo (TMNF symbols, CMwClassInfo::FindFromClassName / MwGetNearestFather / __InitClassInfo_X):

| Offset | Field |
|---|---|
| +0x04 | class id (e.g. 0x2402e000) |
| +0x08 | parent CMwClassInfo* |
| +0x14 | class name (char*) |
| +0x18 | next class info (linked list, head = CMwEngineManager::First) |
| +0x20 | SMwParamInfo** params |
| +0x24 | param count |

`__InitClassInfo_<Class>(CMwClassInfo&)` writes +0x20/+0x24 (`mov [reg+20h], imm32`). TMNF writes the count as an immediate, ESWC/Sunrise load it from a variable.

SMwParamInfo (0x28 bytes seen for methods): +0x00 type, +0x04 member id (class id | index), +0x0c field offset (0xffffffff if none), +0x10 name, +0x14/+0x18 more name pointers (same string for methods), +0x20 flags (0x3f80 on methods), +0x24 function pointer for methods. Member lookup by id: `params[id & 0xfff]` (CMwClassInfo::GetMwParamFromName).

Type numbers differ between engine versions (TMNF types are shifted against ESWC/Sunrise), and so do class ids: in ESWC 0x03043000 is CGameReplay, while CGameCtnChallenge has 0x24003000. TMNF maps old ids with CMwDeprecated::WrapClassId. Porting therefore goes by class and member NAME, not by id or offset.

## Dumps

`data/reflection/<exe>_classes.json`, made with `scripts/re/classinfo.py <exe> <out.json>` (static, from the exe file):

| Exe | Classes (named) | Params |
|---|---|---|
| TMN ESWC 1.80 (sha 983bd127…) | 562 (555) | 6102 |
| TM Sunrise Win10 repack (sha e59dfc7e…) | 522 (515) | 5577 |
| TMNF (timestamp 4d431494) | 623 (616) | 7124 |

Each class: `name`, `init_va`, `params_va`, `params[]` with `type`, `id`, `offset`, `name`, raw words `w`. Class names come from the registration code next to the class id, so a few are missing.

Used for: finding menu handlers by name across games (e.g. `MenuPlayChallenge_Solo`, `MenuChooseChallenge_OnOk` / Sunrise `MenuPlayChallenge_OnOk`), see games/tmn-eswc and games/tm-sunrise. Next: a host-agnostic C++ runtime walker (CMwEngineManager::First list) for a node explorer and scripting.
