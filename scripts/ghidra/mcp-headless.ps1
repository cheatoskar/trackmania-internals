# start the GhidraMCP headless REST server (port 8089) on the TmForever project
# Starts the GhidraMCP headless server (no GUI) on 127.0.0.1:8089 with the TmForever project.
# The MCP bridge `bridge-mcp-ghidra` talks to it like to the GUI plugin.
# Usage: powershell -File mcp-headless.ps1 [-Program /TMNF/TmForever.exe]
param([string]$Program = "/TMNF/TmForever.exe")

$ghidra = "$env:USERPROFILE\tools\ghidra_12.1.2_PUBLIC"
$project = "$env:USERPROFILE\tools\tm-re\project\TmForever.gpr"
$log = "$env:USERPROFILE\tools\tm-re\mcp-headless.log"

& "$ghidra\support\launch.bat" fg jdk GhidraMCP-Headless 8G '""' com.xebyte.headless.GhidraMCPHeadlessServer `
    --port 8089 --project $project --program $Program *> $log
