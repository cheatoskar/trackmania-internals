# ManiaLinks on TMF dedicated servers

How a server controller shows its own UI (widgets, popups, clickable lists) to players on a TMF dedicated server (`TrackmaniaServer_2011-02-21`, TMNF and TMUF), and what broke while building one. Everything here comes from one controller that ran on public servers for three months: [100-TMX-Server](https://github.com/cheatoskar/100-TMX-Server) (C#, fork of BigBang1112/randomizer-anywhere, ManiaAPI.XmlRpc 2.9.5). The wire protocol and the other XML-RPC methods are in [xmlrpc-dedicated.md](xmlrpc-dedicated.md); running the server is in [operations.md](operations.md).

Source keys: `RG` = [RandomizerGame.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/RandomizerGame.cs), `RC` = [RemoteClient.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/RemoteClient.cs), `IDEAS` = [IDEAS.md](https://github.com/cheatoskar/100-TMX-Server/blob/main/IDEAS.md) (findings written for the upstream author), `c:<hash>` = a commit message in 100-TMX-Server (`https://github.com/cheatoskar/100-TMX-Server/commit/<hash>`), `own notes` = the author's operating notes. Status as in the [README](../README.md#confidence-levels); "live" means seen on a public server with players.

## Which servers have ManiaLinks

| Fact | Source | Status |
|---|---|---|
| The TMF dedicated server (TMNF and TMUF, same build) has `SendDisplayManialinkPage`, `SendDisplayManialinkPageToLogin`, `SendDisplayManialinkPageToId`, `SendHideManialinkPage` and the `TrackMania.PlayerManialinkPageAnswer` callback | RC; RG | verified (live) |
| TMNF and TMUF behave the same: the TMUF instance ran the same build with the same widgets | own notes (TMUF instance, 2026-11-07) | verified (live) |
| The 2006 server (TMN ESWC, Sunrise, Original) has none of them ("Method not defined"): no widgets at all; everything players must know has to go to chat, and `$l` chat links show as literal text there, so post plain URLs | [old-2006-server.md](old-2006-server.md); c:[05cbc5a](https://github.com/cheatoskar/100-TMX-Server/commit/05cbc5a), c:[d18990f](https://github.com/cheatoskar/100-TMX-Server/commit/d18990f) | verified (live) |
| Feature-detect instead of keying on the game: call `system.listMethods` once after connecting and turn every ManiaLink call into a no-op when `SendDisplayManialinkPage` is missing | RC `SupportsManialinks` | verified |

## Sending and hiding

| Method / technique | Fact | Source | Status |
|---|---|---|---|
| `SendDisplayManialinkPage(string xml, int timeoutMs, bool hideOnClick)` | shows the page to every player connected right now; `0, false` = stays until replaced | RC | verified |
| `SendDisplayManialinkPageToLogin(string login, string xml, int, bool)` | one player, by login. Throws "Login unknown" when the player has just left: catch it and carry on (a finish handler that aborted on this skipped the map switch) | RC; c:[72e7a53](https://github.com/cheatoskar/100-TMX-Server/commit/72e7a53) | verified (live) |
| Late joiners | a page broadcast before a player connected is NOT shown to them: resend the current widgets to that login on `TrackMania.PlayerConnect` | RG `SendMapWidgetsToLoginAsync` | verified (live) |
| Replace by id | a `<manialink id="x">` replaces the page with the same id on that client; pages with different ids stay side by side (an always-on top-10 panel next to a per-map checkpoint counter) | RG | verified (live) |
| Hide one page | send `<manialink id="x" version="1"></manialink>` (empty body) with the same id, to all or to one login | RG `HideManialinkAsync`; IDEAS | verified (live) |
| `SendHideManialinkPage()` | hides every page of every player, including always-on panels: unusable for "hide one widget" | RG EndRace handler comment | verified |
| `SendHideManialinkPageToId` | wants an INT; passing the page's string id throws "Value of type STRING supplied where type INT was expected". The int is most likely the player's UId (the `...ToId` methods address players, like `SendDisplayManialinkPageToId`), so it would hide all pages of one player, not one page | IDEAS; c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79) | verified (error), inferred (meaning; see [contradictions](../contradictions.md) #26) |
| Debug | a chat command that sends a big centred label to yourself ("TEST HUD WORKS") tells "ManiaLinks don't reach me" from "my layout is off-screen" | RG `TestHudAsync` | verified |

## Clicks: actions and answers

| Fact | Source | Status |
|---|---|---|
| A clickable element carries `action="<int>"` (used on `<quad>`); the click comes back as callback `TrackMania.PlayerManialinkPageAnswer(int playerUid, string login, int answer)` with that int | IDEAS; RG | verified (live) |
| There is no scoping: all pages share one int namespace and the controller dispatches on the number itself | IDEAS | verified |
| Leave wide gaps between id ranges of generated lists. Preset entries started at 10 and history entries at 20: the 11th preset (one server listed 14) was read as a history click. Fixed with bases 100 and 200 | RG action constants; c:[3b87f11](https://github.com/cheatoskar/100-TMX-Server/commit/3b87f11) | verified (live bug) |
| A list click carries only its index: keep the list as it was shown (here: the last list shown) to map the index back to an item | RG `lastShownPresetNames` | verified (code) |
| The answer arrives on the same single callback loop as everything else (see [xmlrpc-dedicated.md](xmlrpc-dedicated.md#limits-and-timing)): a click handler that waits on the network freezes chat commands, checkpoints and finishes for everyone | RG; c:[af9d412](https://github.com/cheatoskar/100-TMX-Server/commit/af9d412) | verified (live) |

## Layout

| Fact | Source | Status |
|---|---|---|
| Use `posn="x y z"` and `sizen="w h"` with `version="1"`; the visible area is roughly x -64..64, y -48..48 (y up). Values outside render off-screen without an error | IDEAS; RG | verified |
| `halign`/`valign` (`left`, `center`, `right`, `top`) anchor the element at `posn`; the third `posn` value orders layers (background quad 4, text 5, button text 6 in the controller) | RG | verified (live) |
| Colours: `bgcolor`/`textcolor` take 3 or 4 hex digits RGB(A), e.g. `000A` = black, partly transparent | IDEAS; RG | verified |
| `<label ... text="...">` renders TrackMania format codes like chat: colours `$f00`, `$o` bold, `$w` wide, `$n` narrow, `$s` shadow, `$z` reset, `$$` a literal dollar. Codes are case-insensitive (`$S` works) | RG; c:[f4268d1](https://github.com/cheatoskar/100-TMX-Server/commit/f4268d1) | verified |
| `<quad image="http://...png">` loads the image from a URL: the controller served its icons from its own web server, so that port must be reachable by players | RG `BuildTop10Manialink` | verified (live) |
| `<quad url="...">` opens a web link; TMF adds `http://` itself, so the controller passes the address without a scheme. `<quad manialink="tmtp://#addfavourite=<server login>">` was used for an "add to favourites" tile | RG comments | inferred (in use, not tested separately) |
| Text width: a flat character limit overflowed panels. Nicknames use wide lookalike glyphs (Cyrillic, Greek, symbols ~1.8 ASCII widths; CJK, Hangul, fullwidth forms ~3). Truncate by estimated width, never cut inside a `$` code, and count the `...` against the budget. Estimates used: `$o` x1.15, `$w` x1.4, `$n` x0.7 | RG `MeasureDisplayWidth`; c:[8870994](https://github.com/cheatoskar/100-TMX-Server/commit/8870994), c:[f68f0d2](https://github.com/cheatoskar/100-TMX-Server/commit/f68f0d2) | verified (live overflow), estimates inferred |
| Name and score in ONE right-aligned label: two separately anchored labels left a gap after a truncated name | RG | verified (live) |

## Encoding and escaping traps

| Trap | Fix | Source | Status |
|---|---|---|---|
| Numbers formatted with the host's locale: on a German machine `18.5` became `18,5` and broke `posn`/`sizen` | format with the invariant culture (`.` always) | c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79); RG `Inv()` | verified (live bug) |
| Player names, map names and preset names go into XML attributes | XML-escape every inserted string (`&`, `<`, `>`, `"`, `'`) | RG (`SecurityElement.Escape`) | verified |
| `$<`...`$>` (format push/pop) inside a label becomes `&lt;`/`&gt;` after escaping; the controller does not rely on the game decoding it back | reset with `$z` (and set the colour again) instead | RG top-10 comment | unverified (avoided, never tested) |
| Nicknames contain `$l`/`$h`/`$p` links and raw format codes | keep the raw nickname for ManiaLinks (renders like chat), strip codes and links for anything outside the game (status page, Discord) | RG `StripTmCodes`; c:[ec1787a](https://github.com/cheatoskar/100-TMX-Server/commit/ec1787a) | verified |
| Non-ASCII text (CJK, Hangul, lookalikes) | renders in labels; only the width estimate is a problem (above) | c:[f68f0d2](https://github.com/cheatoskar/100-TMX-Server/commit/f68f0d2) | verified (live) |

## Size, refresh, load

| Fact | Source | Status |
|---|---|---|
| The only hard limit seen is the request size: TMF drops the XML-RPC connection on any request over 512 KB. The controller's largest pages (a 14-entry list, a top-10 panel) are a few KB; no element-count limit was ever hit | [xmlrpc-dedicated.md](xmlrpc-dedicated.md#limits-and-timing); RG | verified (limit), no ManiaLink-specific limit measured |
| Per-player refresh is cheap enough: the checkpoint counter is resent to one login on every `PlayerCheckpoint`. At 9-11 players the dedicated used 4-10 % CPU, load about 0.1, no lag warnings | RG; own notes 2026-10-22 | verified (live) |
| Replacing a page by id was never reported to flicker; between `EndRace` and the next `BeginRace` the old counter stays visible for a moment because it is only overwritten, not hidden | RG EndRace comment | verified (live), flicker not measured |
| `BeginRace` can be missing for a map that finished loading before anyone connected: widgets that are only sent from `BeginRace` never appear. Fill state on demand from `GetCurrentChallengeInfo` | IDEAS; c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79) | verified (live) |
| Do not fetch remote data (TMX details for a map-info panel) inside a callback before sending a page: send it from a background job | RG `ShowMapInfoWidgetInBackground`; c:[aedfa79](https://github.com/cheatoskar/100-TMX-Server/commit/aedfa79) | verified (live) |

## What broke, and the fix

| Symptom on the live server | Cause | Fix | Source |
|---|---|---|---|
| All widgets, chat commands and checkpoint updates froze for 60 s after a map change (2026-10-01) | one `SendDisplayManialinkPage` reply was lost while every other call answered in 1-45 ms; the call ran inside the `BeginRace` handler, which holds the callback loop. Root cause: ManiaAPI.XmlRpc <= 2.9.2 registered the reply slot after sending, so a fast reply was dropped | bound every call to 10 s; update to ManiaAPI.XmlRpc 2.9.5 | RC `RpcTimeout` comment; c:[219bdcd](https://github.com/cheatoskar/100-TMX-Server/commit/219bdcd), c:[4da3a4c](https://github.com/cheatoskar/100-TMX-Server/commit/4da3a4c) |
| Hiding a widget threw a type error | `SendHideManialinkPageToId` takes an int, not the page id | empty page with the same id | c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79) |
| Popups and lists drawn in the wrong place on one machine | decimal comma from the host locale | invariant formatting | c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79) |
| No widgets (and no finish handling) on the first map after a restart | `BeginRace` never fired for it | `GetCurrentChallengeInfo` on demand, widgets resent on connect | c:[7af6e79](https://github.com/cheatoskar/100-TMX-Server/commit/7af6e79) |
| Top-10 names ran out of the panel | character count instead of rendered width | width estimate per glyph and per `$` code | c:[8870994](https://github.com/cheatoskar/100-TMX-Server/commit/8870994), c:[f68f0d2](https://github.com/cheatoskar/100-TMX-Server/commit/f68f0d2) |
| Clicking the 11th preset opened a history entry | action id ranges 10 apart | ranges 100 apart | c:[3b87f11](https://github.com/cheatoskar/100-TMX-Server/commit/3b87f11) |
| An exception in one callback handler stopped all later callbacks (widgets, chat, finishes) until restart | an unhandled exception ends the library's dispatch loop | wrap every handler in try/catch | c:[f383685](https://github.com/cheatoskar/100-TMX-Server/commit/f383685) |
| A checkpoint counter read "4/3" | `NbCheckpoints` and the checkpoint callback count the finish line | normalise once, clamp to the total, show nothing on maps without checkpoints | c:[c172b09](https://github.com/cheatoskar/100-TMX-Server/commit/c172b09) |

## Minimal example

```xml
<manialink id="cp_counter" version="1">
  <quad posn="-64 37 4" sizen="13 5" halign="left" valign="center" bgcolor="000A"/>
  <label posn="-63 37 5" halign="left" valign="center" textsize="2.5" textcolor="FFFF" text="CP 2 / 5"/>
</manialink>
```

A clickable button is a `<quad ... action="1"/>` with a `<label>` on top; the click arrives as `PlayerManialinkPageAnswer(uid, login, 1)`. Send with `SendDisplayManialinkPageToLogin(login, xml, 0, false)`; hide with the same id and an empty body.
