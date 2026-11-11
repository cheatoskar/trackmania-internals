// find instructions using a given displacement, e.g. '+ 0x25c]'
// Lists instructions whose text contains a given displacement, e.g. "+ 0x25c]".
// args: OUTPUT_FILE DISP_HEX [onlyWrites=1]
// @category ESWC

import java.io.FileWriter;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class FindDisp extends GhidraScript {
	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		PrintWriter w = new PrintWriter(new FileWriter(args[0], true));
		String needle = "+ 0x" + args[1].toLowerCase() + "]";
		boolean onlyWrites = args.length > 2 && args[2].equals("1");
		w.println("=== disp " + args[1]);
		InstructionIterator it = currentProgram.getListing().getInstructions(true);
		while (it.hasNext()) {
			Instruction ins = it.next();
			String s = ins.toString().toLowerCase();
			if (!s.contains(needle)) continue;
			if (onlyWrites) {
				String m = ins.getMnemonicString().toLowerCase();
				String op0 = ins.getDefaultOperandRepresentation(0).toLowerCase();
				if (!(m.equals("mov") || m.equals("lea") || m.equals("add") || m.equals("push"))) continue;
				if (m.equals("mov") && !op0.contains(needle.substring(0, needle.length() - 1))) continue;
			}
			Function fn = getFunctionContaining(ins.getAddress());
			w.println(ins.getAddress() + "  " + ins + "   in " + (fn == null ? "?" : fn.getName() + "@" + fn.getEntryPoint()));
		}
		w.close();
	}
}
