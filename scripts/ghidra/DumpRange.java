// listing dump of address ranges (creates functions)
// Dumps Ghidra's listing (instructions) for address ranges; creates functions where missing on request.
// args: OUTPUT_FILE start-end [start-end ...]   (hex, e.g. 0x006e9900-0x006e9a80)
// @category ESWC

import java.io.FileWriter;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class DumpRange extends GhidraScript {
	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		PrintWriter w = new PrintWriter(new FileWriter(args[0], true));
		for (int i = 1; i < args.length; i++) {
			String[] p = args[i].split("-");
			Address a = toAddr(Long.parseUnsignedLong(p[0].substring(2), 16));
			Address b = toAddr(Long.parseUnsignedLong(p[1].substring(2), 16));
			w.println("=== " + args[i]);
			InstructionIterator it = currentProgram.getListing().getInstructions(new AddressSet(a, b), true);
			while (it.hasNext()) {
				Instruction ins = it.next();
				StringBuilder sb = new StringBuilder();
				sb.append(ins.getAddress()).append("  ").append(ins.toString());
				if (ins.getFlowType().isCall()) {
					Address[] fl = ins.getFlows();
					if (fl.length > 0) {
						Function f = getFunctionAt(fl[0]);
						if (f != null) sb.append("   ; ").append(f.getName());
					}
				}
				w.println(sb);
			}
		}
		w.close();
	}
}
