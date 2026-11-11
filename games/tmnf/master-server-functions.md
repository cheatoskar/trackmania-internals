# TMNF: server browser and join functions (TmForever.exe 2.11.26, 4d431494)

Static analysis (Ghidra), spot-checked against `TmForever.map`. Login, session and request functions of the master-server client are deliberately not listed (see [../../server/masterserver.md](../../server/masterserver.md)).

| Address | Name | Status |
|---|---|---|
| 0x00637C20 / 0x00637990 / 0x00638550 / 0x0063B170 | MenuInternetLeague_InitServers / InitServersSimple / ChangeServersType / MenuInternetLeague (pack mask hex) | verified |
| 0x00702BC0 / 0x00703F10 | CGameNetServerInfo / CGameCtnNetServerInfo::FillFromXML; param tables 0x00CF94B0 / 0x00CF96D0 | verified |
| 0x0059EBC0 / 0x0059EA20 | CGameApp::CmdLineUrlTmtp_Check / DoJoinParam (`tmtp://` parsing) | verified |
| 0x00CF24B8 | table of environment pack-mask bits | verified |
| server-info struct (CGameNetServerInfo) | +0xB8 ServerName, +0xC0/+0xC2 players/max, +0xC3/+0xC5 spectators/max, +0xC8 LadderMode, +0xF4 IsBuddy, +0x120 IsFavourite, +0x140 players avg level, +0x148/+0x14C ladder limit max/min, +0x150 text (zone?), +0x158 login, +0x160 PlayerName, +0x1C0 ServerLevel, +0x1E0 pack mask text, +0x1E8 pack mask | verified (offsets), inferred (some meanings) |
