# Sunrise online login: the master server still runs

Nadeo's Sunrise master server still answers. Only its names are gone. Checked on 2026-11-14.

| What | Finding | Source | Status |
|---|---|---|---|
| names the game uses | `www.trackmaniasunrise.com` is the only master host string in `TmSunrise.exe`; `game.trackmaniasunrise.com` comes up too (most likely from the master's own answers, as on ESWC) | exe strings | verified / inferred |
| DNS today | `game.trackmaniasunrise.com` does not resolve; `www.trackmaniasunrise.com` resolves to an OVH parking server (213.186.33.5) that resets the connection, so the game shows HTTP error 12031 | nslookup, game | verified |
| the master box | `91.121.54.245`, the same box as `game.trackmanianations.com` (ESWC master). Apache with PHP 5.2.6, HTTPS with TLS 1.0 only, legacy renegotiation, expired certificate | direct requests | verified |
| site by Host header | `Host: game.trackmaniasunrise.com` -> `/home/scripts/game_Host_v1/...`, `Host: www.trackmaniasunrise.com` -> `/home/scripts/game_Host_v1_web/...` (`/online_game/getservers.php` without `ver` answers "Bad ver"); `/` gives a 403 naming the vhost. No SNI needed | direct requests | verified |
| second error | with the names pointed at the box (hosts file) the game shows 12157 (`ERROR_INTERNET_SECURITY_CHANNEL_ERROR`): Windows 10/11 WinINet has TLS 1.0 off, as for ESWC | game | verified |
| WinINet imports | `InternetCloseHandle` 0x00a1a2d4, `InternetConnectA` 0x00a1a2d8, `HttpOpenRequestA` 0x00a1a2dc (IAT slots, base 0x400000); each called straight through its slot | pefile, capstone | verified |
| accounts | creating an online account still goes through the master with the game key, as in 2005 | game (asks for the player key) | verified |

## Two ways to get online again

1. **System-wide (manual):** hosts file `91.121.54.245 game.trackmaniasunrise.com` and `91.121.54.245 www.trackmaniasunrise.com`, plus Internet Options "Use TLS 1.0" on and "Check for server certificate revocation" off. This changes the settings for every WinINet program.
2. **In the game process only** (Horizon Client / Legacy Essentials login fix): `InternetConnectA` to one of the two names connects to `91.121.54.245` instead, every request on that connection gets `Host: <name>` back (`HttpAddRequestHeadersA`, ADD | REPLACE), and the ESWC login fix adds TLS 1.0 to `SecureProtocols` for the process and sets `SECURITY_FLAG_IGNORE_REVOCATION` on HTTPS requests. The game already sets `IGNORE_CERT_CN_INVALID`, so connecting by IP does not trip the certificate check. Tested in the game on 2026-11-14: the master is reached without a hosts entry or system setting.

The address is fixed in the code; if Nadeo moves the box, a hosts entry pointing the names at the new address keeps working with the fix.
