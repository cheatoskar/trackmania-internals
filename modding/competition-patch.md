# Competition Patch 2.0 (TMNF/TMUF): what mods need to know

Summary of the public descriptions by its authors, plus what follows for mods. Nothing here was taken from the patch's code; status `documented` means "stated by the authors, not checked by me". Sources: the Competition Patch page on [donadigo.com/tmcp](https://donadigo.com/tmcp) (updated 2026-11-04) and the technical post on the ModLoader site by Tomashu.

## What it is

| fact | value | source | status |
|---|---|---|---|
| authors | 1.x by donadigo; 2.0 (a rewrite) by Tomashu, the ModLoader developer | both pages | documented |
| install | 2.0 only as a ModLoader mod (no standalone); game version >= 2.11.26; Steam players are told to use the standalone game (Steam Fix) | donadigo.com/tmcp | documented |
| purpose | validation of runs for important TMX categories (official campaigns, classic tracks): no replaying of pre-made runs (TAS), no slow motion | both pages | documented |
| how to tell it runs | a dialog on the first start; the window title names the patch | donadigo.com/tmcp | documented |
| Linux | works (cryptography, no kernel driver) | ModLoader post | documented |
| servers | from 2026-11-19 servers require Competition Patch 2.0 (exchange news of 2026-11) | TMNF-X / TMUF-X news | documented |

## How a run is validated

| step | value | source | status |
|---|---|---|---|
| race start | the game asks the patch server for a random seed; the seed drives a PRNG that changes the player's steer input | both pages | documented |
| input change | a few least significant bits of the steer value are set from the PRNG; full steer (±65536) and zero stay unchanged; the change never points the other way than the stick; small input changes give small alterations | ModLoader post | documented |
| keyboard | keyboard steer is converted to analog first and moves to the new value over several ticks (otherwise digital input would hide the change) | ModLoader post | documented |
| grace period | no change during the first 2 s of a race and 1 s after a respawn (press-forward maps keep working) | donadigo.com/tmcp | documented |
| race end | the game sends a hash of the inputs; the server signs seed, timestamp and hash (cryptographic timestamping) and keeps nothing; the signature block goes into the replay | both pages | documented |
| slow motion | the server's wall-clock timestamps must match the race time, so slowed play or fixing inputs after the fact shows up | donadigo.com/tmcp | documented |
| connection | only needed at start and end; at the end the game retries for a few seconds; no connection: the timer turns red and the replay is not validated | donadigo.com/tmcp | documented |
| extra data | encrypted with a key pair the game makes per replay plus the verifier's built-in public key (key exchange spread over download and upload); the original inputs are stored too and can be recovered by tools | ModLoader post | documented |
| where it is checked | on TMX when the replay is uploaded; replays still validate in the game, physics are unchanged | donadigo.com/tmcp | documented |

## What follows for mods

| rule | why | status |
|---|---|---|
| with the patch loaded, do nothing with input: no DirectInput or window-message filtering, no injected keys | the patch changes the steer input itself before the simulation; anything in between can break the input hash or the validation | inferred |
| no effect on game speed or the simulation clock | slow motion is detected by wall clock against race time | inferred |
| keep long frame stalls out of races (shader compiles, synchronous GPU readbacks, file I/O on the render thread) | whether the game's race clock and the server's wall clock can drift apart over long stalls is not known; avoiding stalls is the safe side | inferred (open question) |
| an input display reading the game's input may show the modified (analog, slightly changed) steer, not the raw stick | the change happens before the simulation; which value a mod reads depends on where it reads | inferred |
| detect the patch at run time, not only at start-up | it is a ModLoader mod and may load after another mod initialised; re-check its module while running | inferred |
| test with the patch loaded and never work around it | see [tmx-rules-for-mods.md](tmx-rules-for-mods.md) | rule |
