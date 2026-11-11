# opt_rounds.ps1 - island rounds of fvlab `opt` (earliest finish or box entry), best of each round seeds the next. Input: replay. MIT licence, see LICENSE-MIT.
# Island search for the earliest finish: each round runs $Workers fvlab "opt" processes from the current
# best inputs (different seeds and segment counts), then the best result seeds the next round.
param([string]$Replay, [string]$Name, [int]$From = 100, [int]$To = 0, [int]$Rounds = 3,
      [int]$Seconds = 600, [int]$Workers = 10, [string]$Start = "", [int]$SeedBase = 0, [string]$OutDir = "out")
# -SeedBase: give every machine its own value (e.g. 0 on one, 100000 on another), otherwise two machines with
# the same start inputs try exactly the same candidates
. (Join-Path $PSScriptRoot "fvlab_env.ps1")  # $FvlabExe, $Packs from FVLAB_BIN / TM_PACKS
$b = $FvlabExe
$packs = $Packs
$dir = Join-Path $OutDir $Name
New-Item -ItemType Directory -Force $dir | Out-Null
$dir = (Resolve-Path $dir).Path  # absolute: the workers get it as an argument
$segs = @(4, 8, 8, 16, 16, 32, 8, 16, 4, 32, 8, 16)
$current = if ($Start) { $Start } else { $null }
$Workers = [Math]::Min($Workers, [Environment]::ProcessorCount - 1)
for ($round = 1; $round -le $Rounds; $round++) {
    $procs = @()
    for ($w = 0; $w -lt $Workers; $w++) {
        $out = "$dir\r$($round)_w$w.csv"; $log = "$dir\r$($round)_w$w.log"
        $env:FVLAB_OPTIMIZED = "1"
        if ($current) { $env:FVLAB_INPUTS = $current } else { Remove-Item Env:FVLAB_INPUTS -ErrorAction SilentlyContinue }
        $seed = $SeedBase + $round * 1000 + $w
        $procs += Start-Process -FilePath $b -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err" `
            -ArgumentList "`"$packs`"", "`"$Replay`"", "opt", $seed, $From, $To, $segs[$w % $segs.Count], $Seconds, "`"$out`""
    }
    # below-normal priority and at most (cores - 1) workers keep the desktop (VS Code, the game) responsive
    $procs | ForEach-Object { try { $_.PriorityClass = 'BelowNormal' } catch {} }
    $procs | Wait-Process
    $best = $null; $bestMs = [double]::MaxValue
    for ($w = 0; $w -lt $Workers; $w++) {
        $last = Get-Content "$dir\r$($round)_w$w.log" | Where-Object { $_ -match "^done:" } | Select-Object -Last 1
        if ($last -match "best ([0-9.]+) ms") { $ms = [double]$Matches[1]; if ($ms -lt $bestMs) { $bestMs = $ms; $best = "$dir\r$($round)_w$w.csv" } }
    }
    Copy-Item $best "$dir\best.csv" -Force
    $current = "$dir\best.csv"
    "round $round best $bestMs ms ($best)" | Tee-Object -Append "$dir\rounds.txt"
}
