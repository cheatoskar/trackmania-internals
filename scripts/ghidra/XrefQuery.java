// xrefs for strings (s:/w:), addresses (a:) and data values (d:)
// Prints references for strings or addresses.
// args: OUTPUT_FILE then items. Item "s:Text" = exact ASCII string (null terminated),
// "w:Text" = UTF-16LE string, "a:0x00401000" = address (refs to it, incl. function callers),
// "d:0x00401000" = 32-bit data occurrences of that value anywhere in memory (vtables, tables).
// @category ESWC

import java.io.FileWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;

public class XrefQuery extends GhidraScript {
	PrintWriter w;

	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		w = new PrintWriter(new FileWriter(args[0], true));
		for (int i = 1; i < args.length; i++) {
			String it = args[i];
			String kind = it.substring(0, 2);
			String val = it.substring(2);
			w.println("=== " + it);
			if (kind.equals("s:") || kind.equals("w:")) {
				byte[] b;
				if (kind.equals("s:")) {
					byte[] s = val.getBytes(StandardCharsets.US_ASCII);
					b = new byte[s.length + 1];
					System.arraycopy(s, 0, b, 0, s.length);
				} else {
					byte[] s = val.getBytes(StandardCharsets.UTF_16LE);
					b = new byte[s.length + 2];
					System.arraycopy(s, 0, b, 0, s.length);
				}
				Memory mem = currentProgram.getMemory();
				Address start = mem.getMinAddress();
				while (true) {
					Address f = mem.findBytes(start, b, null, true, monitor);
					if (f == null) break;
					// require preceding byte to be 0 (start of string) for ascii
					w.println(" str @ " + f);
					refsTo(f);
					findData(f.getOffset());
					start = f.add(1);
				}
			} else if (kind.equals("a:")) {
				Address a = toAddr(Long.parseUnsignedLong(val.substring(2), 16));
				refsTo(a);
			} else if (kind.equals("d:")) {
				findData(Long.parseUnsignedLong(val.substring(2), 16));
			}
		}
		w.close();
	}

	void refsTo(Address a) {
		for (Reference r : getReferencesTo(a)) {
			Address from = r.getFromAddress();
			Function fn = getFunctionContaining(from);
			w.println("   ref " + r.getReferenceType() + " from " + from + " in "
				+ (fn == null ? "?" : fn.getName() + "@" + fn.getEntryPoint()));
		}
	}

	void findData(long v) throws Exception {
		byte[] b = new byte[] { (byte) v, (byte) (v >> 8), (byte) (v >> 16), (byte) (v >> 24) };
		Memory mem = currentProgram.getMemory();
		Address start = mem.getMinAddress();
		int n = 0;
		while (n < 50) {
			Address f = mem.findBytes(start, b, null, true, monitor);
			if (f == null) break;
			Function fn = getFunctionContaining(f);
			w.println("   dword at " + f + (fn == null ? "" : " in " + fn.getName() + "@" + fn.getEntryPoint()));
			start = f.add(1);
			n++;
		}
	}
}
