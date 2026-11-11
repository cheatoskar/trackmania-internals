# watch_opt.ps1 - live view of a running opt_rounds.ps1 search, new overall bests in green. Input: search name. MIT licence, see LICENSE-MIT.
# Live view of a running opt_rounds.ps1 search: prints every new improvement of every worker
# (green when it beats the best seen so far), like the TMInterface bruteforce log.
# usage: powershell -ExecutionPolicy Bypass -File scripts\tas\watch_opt.ps1 NAME [-All] [-OutDir out]
# Default: only new overall bests (green). -All also shows per-worker improvements and start/end lines.
param([Parameter(Mandatory = $true)][string]$Name, [switch]$All, [string]$OutDir = "out")
$dir = Join-Path $OutDir $Name
$seen = @{}
$best = [double]::MaxValue
Write-Host "watching $dir (Ctrl+C to stop)"
while ($true) {
    foreach ($log in Get-ChildItem $dir -Filter "*.log" -ErrorAction SilentlyContinue) {
        $lines = @(Get-Content $log.FullName -ErrorAction SilentlyContinue)
        $start = if ($seen.ContainsKey($log.Name)) { $seen[$log.Name] } else { 0 }
        for ($i = $start; $i -lt $lines.Count; $i++) {
            $l = $lines[$i]
            $tag = $log.BaseName
            if ($l -match ": ([0-9.]+) ms \(segment") {
                $ms = [double]$Matches[1]
                if ($ms -lt $best) { $best = $ms; Write-Host ("[{0}] {1}  NEW BEST {2:N6} ms" -f $tag, $l, $ms) -ForegroundColor Green }
                elseif ($All) { Write-Host ("[{0}] {1}" -f $tag, $l) -ForegroundColor DarkGray }
            } elseif ($All -and $l -match "^(base|done)") {
                Write-Host ("[{0}] {1}" -f $tag, $l) -ForegroundColor Cyan
            }
        }
        $seen[$log.Name] = $lines.Count
    }
    $rounds = Join-Path $dir "rounds.txt"
    Start-Sleep -Seconds 2
}
