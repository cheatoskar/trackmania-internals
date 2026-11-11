# headless import+analysis of TMNF (with map) and TMUF into one Ghidra project
# Imports both TmForever.exe builds into the Ghidra project ~/tools/tm-re/project/TmForever
# and runs auto-analysis. TMNF gets the symbols of TmForever.map before analysis.
# No 'Stop' preference: PowerShell 5.1 turns Ghidra's stderr lines into terminating errors.
$ghidra  = "$env:USERPROFILE\tools\ghidra_12.1.2_PUBLIC"
$re      = "$env:USERPROFILE\tools\tm-re"
$proj    = "$re\project"
$scripts = "$re\scripts"
$env:GHIDRA_HEADLESS_MAXMEM = '8G'
New-Item -ItemType Directory -Force $proj | Out-Null

# Copies in bin\: cmd.exe (analyzeHeadless.bat) can't take the "(x86)" in the game paths.
& "$ghidra\support\analyzeHeadless.bat" $proj "TmForever/TMNF" `
    -import "$re\bin\tmnf\TmForever.exe" `
    -scriptPath $scripts `
    -preScript ImportMsvcMap.java "$re\bin\tmnf\TmForever.map" `
    -log "$re\analyze_tmnf.log" -scriptlog "$re\analyze_tmnf_script.log"
Write-Output "TMNF exit $LASTEXITCODE"

& "$ghidra\support\analyzeHeadless.bat" $proj "TmForever/TMUF" `
    -import "$re\bin\tmuf\TmForever.exe" `
    -log "$re\analyze_tmuf.log"
Write-Output "TMUF exit $LASTEXITCODE"
