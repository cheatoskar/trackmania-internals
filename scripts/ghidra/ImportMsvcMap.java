// Ghidra pre-script: import MSVC .map publics (TmForever.map) as labels/functions
// Imports the symbols of an MSVC linker map (TmForever.map) as labels and functions.
// Names stay mangled; Ghidra's Microsoft demangler analyzer turns them into
// namespaces, classes and signatures during auto-analysis, so run this as a pre-script.
// Usage (headless): -preScript ImportMsvcMap.java <path to .map>
// @category TMVS
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.SymbolTable;

public class ImportMsvcMap extends GhidraScript {
    // " 0001:0028fd10       ?SetIsActive@CGameControlCameraFree@@UAEXH@Z 00690d10 f   gameengine:GameControlCameraFree.obj"
    private static final Pattern LINE =
        Pattern.compile("^\\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\\s+(\\S+)\\s+([0-9a-fA-F]{8})\\s+(f?)\\s*(i?)\\s+(.*)$");

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File map = args.length > 0 ? new File(args[0]) : askFile("MSVC map file", "Import");
        SymbolTable st = currentProgram.getSymbolTable();
        int labels = 0, functions = 0, skipped = 0;
        boolean inSymbols = false;
        try (BufferedReader r = new BufferedReader(new FileReader(map))) {
            String line;
            while ((line = r.readLine()) != null) {
                if (line.contains("Publics by Value") || line.contains("Static symbols")) { inSymbols = true; continue; }
                if (!inSymbols) continue;
                Matcher m = LINE.matcher(line);
                if (!m.matches() || m.group(1).equals("0000")) continue;
                long va = Long.parseLong(m.group(4), 16);
                if (va == 0) continue;
                Address a = toAddr(va);
                MemoryBlock b = currentProgram.getMemory().getBlock(a);
                if (b == null) { skipped++; continue; }
                String name = m.group(3);
                try {
                    st.createLabel(a, name, SourceType.IMPORTED).setPrimary();
                    labels++;
                } catch (Exception e) { skipped++; continue; }
                // Only code in executable blocks becomes a function; 'f' is also set on thunks to imports.
                if (!m.group(5).isEmpty() && b.isExecute()) {
                    Function f = getFunctionAt(a);
                    if (f == null) {
                        disassemble(a);
                        f = createFunction(a, name);
                    }
                    if (f != null) functions++;
                }
                if ((labels & 0xfff) == 0) monitor.setMessage("map: " + labels + " symbols");
                monitor.checkCancelled();
            }
        }
        println(String.format("ImportMsvcMap: %d labels, %d functions, %d skipped", labels, functions, skipped));
    }
}
