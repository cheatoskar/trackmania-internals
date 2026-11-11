# multi_rounds.ps1 - like uber_rounds.ps1 with several start lines searched side by side, one lineage each. Input: replay, start CSVs. MIT licence, see LICENSE-MIT.
# Multi-line search: like uber_rounds.ps1, but with several start lines at once. Each start line (a CSV with
# the full inputs of a drive, or "replay" for the replay's own inputs) is its own lineage: worker w works on
# line w mod n, every round each line keeps its own verified best, so different ideas (my drives that go
# over the road border, into a dip, ...) are searched side by side instead of only the best one.
# Set $env:FVLAB_KICK_STALL (seconds) so a stuck worker jumps away from its line's best (see fvlab.cpp).
# All other objective switches come from the environment as for uber_rounds.ps1.
param([string]$Replay, [string]$Name, [int]$From, [int]$To, [int]$Horizon = 4000,
      [double]$X, [double]$Y, [double]$Z, [int]$Rounds = 2, [int]$Seconds = 600, [int]$Workers = 10,
      [string]$Starts = "replay", [int]$SeedBase = 0, [string]$OutDir = "out")
. (Join-Path $PSScriptRoot "fvlab_env.ps1")  # $FvlabExe, $Packs from FVLAB_BIN / TM_PACKS
$b = $FvlabExe
$packs = $Packs
$dir = Join-Path $OutDir $Name
New-Item -ItemType Directory -Force $dir | Out-Null
$dir = (Resolve-Path $dir).Path  # absolute: the workers get it as an argument
$inv = [Globalization.CultureInfo]::InvariantCulture
$lines = @($Starts -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ })
$n = $lines.Count
$Workers = [Math]::Min($Workers, [Environment]::ProcessorCount - 1)
function Score($csv) {
    # fresh process, only this score counts (the in-search score can differ from a reload)
    if ($csv -eq "replay") { Remove-Item Env:FVLAB_INPUTS -ErrorAction SilentlyContinue } else { $env:FVLAB_INPUTS = $csv }
    $line = & $b $packs $Replay uber 1 $From $To 0 "$dir\verify.csv" $Horizon $X.ToString($inv) $Y.ToString($inv) $Z.ToString($inv) |
        Where-Object { $_ -match "^base:" } | Select-Object -First 1
    if ($line -match "score (-?[0-9.]+)") { return @([double]::Parse($Matches[1], $inv), $line) }
    return @([double]::MaxValue, "")
}
for ($i = 0; $i -lt $n; $i++) {
    $s = Score $lines[$i]
    "line $i start $($lines[$i]): $($s[1])" | Tee-Object -Append "$dir\rounds.txt"
}
for ($round = 1; $round -le $Rounds; $round++) {
    $procs = @()
    for ($w = 0; $w -lt $Workers; $w++) {
        $line = $lines[$w % $n]
        $out = "$dir\r$($round)_w$w.csv"; $log = "$dir\r$($round)_w$w.log"
        $env:FVLAB_OPTIMIZED = "1"
        if ($line -eq "replay") { Remove-Item Env:FVLAB_INPUTS -ErrorAction SilentlyContinue } else { $env:FVLAB_INPUTS = $line }
        $procs += Start-Process -FilePath $b -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err" `
            -ArgumentList "`"$packs`"", "`"$Replay`"", "uber", ($SeedBase + $round * 1000 + $w), $From, $To, $Seconds, "`"$out`"", $Horizon,
                          $X.ToString($inv), $Y.ToString($inv), $Z.ToString($inv)
    }
    # below-normal priority and at most (cores - 1) workers keep the desktop (VS Code, the game) responsive
    $procs | ForEach-Object { try { $_.PriorityClass = 'BelowNormal' } catch {} }
    $procs | Wait-Process
    $overall = [double]::MaxValue; $overallCsv = ""
    for ($i = 0; $i -lt $n; $i++) {
        $best = $lines[$i]; $bs = Score $best; $bestScore = $bs[0]; $bestLine = $bs[1]
        for ($w = $i; $w -lt $Workers; $w += $n) {
            $csv = "$dir\r$($round)_w$w.csv"
            if (-not (Test-Path $csv)) { continue }
            $s = Score $csv
            $reported = Get-Content "$dir\r$($round)_w$w.log" | Where-Object { $_ -match "^done:" } | Select-Object -Last 1
            "r$($round)_w$w (line $i) verified: $($s[1]) | reported: $reported" | Add-Content "$dir\verify.txt"
            if ($s[0] -lt $bestScore) { $bestScore = $s[0]; $best = $csv; $bestLine = $s[1] }
        }
        if ($best -ne $lines[$i]) {
            Copy-Item $best "$dir\line$($i)_best.csv" -Force
            $lines[$i] = "$dir\line$($i)_best.csv"
        }
        "round $round line $i best score $bestScore ($best): $bestLine" | Tee-Object -Append "$dir\rounds.txt"
        if ($bestScore -lt $overall) { $overall = $bestScore; $overallCsv = $lines[$i] }
    }
    if ($overallCsv -and $overallCsv -ne "replay") { Copy-Item $overallCsv "$dir\best.csv" -Force }
    "round $round overall best score $overall ($overallCsv)" | Tee-Object -Append "$dir\rounds.txt"
}
