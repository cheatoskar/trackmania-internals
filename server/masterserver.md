# Master servers (TMF and ESWC)

Scope note: this page deliberately does **not** document the master-server login, authentication, session or request protocol of either game, and nothing about acting as a game client towards Nadeo's services. Only facts that matter for running the unmodified game or a dedicated server are listed.

## TMF (TMNF/TMUF, TmForever.exe 2.11.26)

| Item | Value | Status |
|---|---|---|
| Environment pack-mask bits (server list filter, dedicated `<packmask>`) | Snow 0x01, Desert 0x02, Rally 0x04, Island 0x08, Coast 0x10, Bay 0x20, Stadium 0x40; TMNF uses `40` (Stadium only), TMUF `7f`. Table at 0x00CF24B8 (TMNF 4d431494) | verified |
| Join URLs | `tmtp://#join=<login>[:<password>]`, `#spectate=`, `#joinasreferee=`; command line `/url=tmtp://#join=<login>` (`<login>` = the server's login) | verified (exe strings) |
| Polite use | if you build anything that queries public server lists, poll slowly (minutes, not seconds) and back off on errors | recommendation |

In-game server-browser functions and the `CGameNetServerInfo` layout: [../games/tmnf/master-server-functions.md](../games/tmnf/master-server-functions.md).

## ESWC online services (TmNationsESWC.exe 0.1.7.9)

| Item | Fact | Status |
|---|---|---|
| Host | `game.trackmanianations.com`, prefix `/online_game`, HTTPS 443 via WinINet; set up at 0x00401150 | verified |
| Server side | TLS 1.0 only (cipher AES256-SHA); certificate expired 2020-11-14; revocation check fails (WinINet 12057) | verified (probe) |
| Why modern Windows fails to go online | WinINet `SecureProtocols` (HKCU) defaults to TLS 1.2/1.3 only (0x2800) -> error 12157; `INTERNET_OPTION_SECURE_PROTOCOLS` is rejected (12009/12018) | verified |
| In-process compatibility fix (used by the eswc-autosave mod) | hook `kernelbase!RegQueryValueExW` to OR 0x80 (TLS 1.0) into `SecureProtocols` for the game process only; IAT-hook `HttpOpenRequestA` (slot 0x00ACA340) to add `SECURITY_FLAG_IGNORE_REVOCATION` on HTTPS. Never log request URLs (they can carry credentials) | verified (no fix: 12157; with fix: HTTP 200) |
| Game's own retry | opener 0x004E8710 retries on 12045/12037/12038 with SECURITY_FLAGS \|= 0xF380 (0x004E7C6F) | verified |
| Joining an ESWC server | no `tmtp://` handler; only via the in-game network menu | verified |
