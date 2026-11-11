// read-only decompile of given addresses; x:<addr> lists xrefs, r:<addr> data refs
// Decompile functions containing given addresses. Args: outfile addr1 addr2 ...
// prefix "x:" to also list xrefs to the function, "r:" to list refs to a data addr
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class Dec extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        PrintWriter pw = new PrintWriter(new OutputStreamWriter(new FileOutputStream(args[0]), "UTF-8"));
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        for (int i = 1; i < args.length; i++) {
            String a = args[i];
            boolean xr = false, refs = false;
            if (a.startsWith("x:")) { xr = true; a = a.substring(2); }
            if (a.startsWith("r:")) { refs = true; a = a.substring(2); }
            Address ad = toAddr(a);
            if (refs) {
                pw.println("==== refs to " + ad);
                for (Reference r : getReferencesTo(ad)) {
                    Function f = fm.getFunctionContaining(r.getFromAddress());
                    pw.println("  " + r.getFromAddress() + " " + r.getReferenceType() + " in " + (f == null ? "?" : f.getName() + "@" + f.getEntryPoint()));
                }
                continue;
            }
            Function f = fm.getFunctionContaining(ad);
            if (f == null) {
                createFunction(ad, null);
                f = fm.getFunctionContaining(ad);
            }
            if (f == null) { pw.println("==== no function at " + ad); continue; }
            pw.println("==== " + f.getName() + " @ " + f.getEntryPoint() + " (query " + ad + ")");
            if (xr) {
                for (Reference r : getReferencesTo(f.getEntryPoint())) {
                    Function g = fm.getFunctionContaining(r.getFromAddress());
                    pw.println("  xref " + r.getFromAddress() + " " + r.getReferenceType() + " in " + (g == null ? "?" : g.getName() + "@" + g.getEntryPoint()));
                }
            }
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            if (res.decompileCompleted()) pw.println(res.getDecompiledFunction().getC());
            else pw.println("decompile failed: " + res.getErrorMessage());
        }
        pw.close();
    }
}
