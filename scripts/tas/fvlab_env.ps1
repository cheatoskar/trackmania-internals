# fvlab_env.ps1 - dot-sourced by the round scripts: sets $FvlabExe (env FVLAB_BIN) and $Packs (env TM_PACKS). MIT licence, see LICENSE-MIT.
$FvlabExe = $env:FVLAB_BIN
if (-not $FvlabExe -or -not (Test-Path $FvlabExe)) { throw "fvlab.exe not found: set FVLAB_BIN to its full path (build: tools/fvlab/README.md)" }
$Packs = if ($env:TM_PACKS) { $env:TM_PACKS } else { "C:\Program Files (x86)\TmUnitedForever\Packs" }
if (-not (Test-Path $Packs)) { throw "Packs folder not found: $Packs (set TM_PACKS to <TMUF install>\Packs)" }
