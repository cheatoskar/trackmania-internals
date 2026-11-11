# uber_rounds.ps1 - island rounds of fvlab `uber` towards a target point, every worker result re-scored in a fresh process. Input: replay. MIT licence, see LICENSE-MIT.
# Island search for an uber towards a target point with fvlab "uber" mode: each round runs $Workers
# processes from the current best inputs, then the best score (lowest) seeds the next round.
param([string]$Replay, [string]$Name, [int]$From, [int]$To, [int]$Horizon = 4000,
      [double]$X, [double]$Y, [double]$Z, [int]$Rounds = 2, [int]$Seconds = 600, [int]$Workers = 10,
      [string]$Start = "", [int]$SeedBase = 0, [string]$OutDir = "out")
# -SeedBase: one value per machine (see opt_rounds.ps1)
. (Join-Path $PSScriptRoot "fvlab_env.ps1")  # $FvlabExe, $Packs from FVLAB_BIN / TM_PACKS
$b = $FvlabExe
$packs = $Packs
$dir = Join-Path $OutDir $Name
New-Item -ItemType Directory -Force $dir | Out-Null
$dir = (Resolve-Path $dir).Path  # absolute: the workers get it as an argument
$inv = [Globalization.CultureInfo]::InvariantCulture
$current = if ($Start) { $Start } else { $null }
$Workers = [Math]::Min($Workers, [Environment]::ProcessorCount - 1)
for ($round = 1; $round -le $Rounds; $round++) {
    $procs = @()
    for ($w = 0; $w -lt $Workers; $w++) {
        $out = "$dir\r$($round)_w$w.csv"; $log = "$dir\r$($round)_w$w.log"
        $env:FVLAB_OPTIMIZED = "1"
        if ($current) { $env:FVLAB_INPUTS = $current } else { Remove-Item Env:FVLAB_INPUTS -ErrorAction SilentlyContinue }
        $procs += Start-Process -FilePath $b -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err" `
            -ArgumentList "`"$packs`"", "`"$Replay`"", "uber", ($SeedBase + $round * 1000 + $w), $From, $To, $Seconds, "`"$out`"", $Horizon,
                          $X.ToString($inv), $Y.ToString($inv), $Z.ToString($inv)
    }
    # below-normal priority and at most (cores - 1) workers keep the desktop (VS Code, the game) responsive
    $procs | ForEach-Object { try { $_.PriorityClass = 'BelowNormal' } catch {} }
    $procs | Wait-Process
    # The score a worker reports can differ from a fresh run of its CSV (seen on a nose search: -30 reported,
    # +25 on reload), so every worker CSV is re-scored in a new process and only that score counts.
    $best = $null; $bestScore = [double]::MaxValue; $bestLine = ""
    for ($w = 0; $w -lt $Workers; $w++) {
        $csv = "$dir\r$($round)_w$w.csv"
        if (-not (Test-Path $csv)) { continue }
        $env:FVLAB_INPUTS = $csv
        $check = & $b $packs $Replay uber 1 $From $To 0 "$dir\verify.csv" $Horizon $X.ToString($inv) $Y.ToString($inv) $Z.ToString($inv) |
            Where-Object { $_ -match "^base:" } | Select-Object -First 1
        $reported = Get-Content "$dir\r$($round)_w$w.log" | Where-Object { $_ -match "^done:" } | Select-Object -Last 1
        if ($check -match "score (-?[0-9.]+)") {
            $s = [double]::Parse($Matches[1], $inv)
            "r$($round)_w$w verified: $check | reported: $reported" | Add-Content "$dir\verify.txt"
            if ($s -lt $bestScore) { $bestScore = $s; $best = $csv; $bestLine = $check }
        }
    }
    if (-not $best) { "round $($round): no worker result" | Tee-Object -Append "$dir\rounds.txt"; continue }
    Copy-Item $best "$dir\best.csv" -Force
    $current = "$dir\best.csv"
    "round $round best score $bestScore ($best): $bestLine" | Tee-Object -Append "$dir\rounds.txt"
}
