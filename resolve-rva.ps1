# resolve-rva.ps1 — resolve a module-relative fault offset (RVA) to function + file:line
# using the matching PDB. Usage:
#   ./resolve-rva.ps1 -Exe <path-to-exe> -Rva 0x1321d3
param(
    [Parameter(Mandatory)][string]$Exe,
    [Parameter(Mandatory)][string]$Rva
)
$rvaVal = [Convert]::ToUInt64($Rva, 16)

$src = @"
using System;
using System.IO;
using System.Runtime.InteropServices;
public static class SymRes {
    [DllImport("dbghelp.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern bool SymInitialize(IntPtr h, string path, bool invade);
    [DllImport("dbghelp.dll")] public static extern uint SymSetOptions(uint o);
    [DllImport("dbghelp.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern ulong SymLoadModuleEx(IntPtr h, IntPtr file, string img, string mod, ulong baseAddr, uint sz, IntPtr data, uint flags);
    [DllImport("dbghelp.dll", SetLastError=true)]
    public static extern bool SymGetSymFromAddr64(IntPtr h, ulong addr, out ulong disp, IntPtr sym);
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
    public struct IMAGEHLP_LINE64 { public uint SizeOfStruct; public IntPtr Key; public uint LineNumber; public IntPtr FileName; public ulong Address; }
    [DllImport("dbghelp.dll", SetLastError=true)]
    public static extern bool SymGetLineFromAddr64(IntPtr h, ulong addr, out uint disp, ref IMAGEHLP_LINE64 line);

    public static string Resolve(string exe, ulong rva) {
        IntPtr h = new IntPtr(0x1234);  // pseudo handle
        SymSetOptions(0x10 | 0x2);      // LOAD_LINES | UNDNAME
        SymInitialize(h, Path.GetDirectoryName(exe), false);
        ulong baseAddr = 0x140000000UL;
        ulong mod = SymLoadModuleEx(h, IntPtr.Zero, exe, null, baseAddr, 0, IntPtr.Zero, 0);
        if (mod == 0) return "SymLoadModuleEx failed: " + Marshal.GetLastWin32Error();
        ulong addr = baseAddr + rva;
        int structSize = 32, nameMax = 1024, total = structSize + nameMax;
        IntPtr buf = Marshal.AllocHGlobal(total);
        for (int i=0;i<total;i++) Marshal.WriteByte(buf,i,0);
        Marshal.WriteInt32(buf, 0, structSize);
        Marshal.WriteInt32(buf, 24, nameMax - 1);
        ulong disp;
        string s;
        if (SymGetSymFromAddr64(h, addr, out disp, buf)) {
            string name = Marshal.PtrToStringAnsi(IntPtr.Add(buf, structSize));
            s = string.Format("{0}+0x{1:X}", name, disp);
            IMAGEHLP_LINE64 line = new IMAGEHLP_LINE64();
            line.SizeOfStruct = (uint)Marshal.SizeOf(typeof(IMAGEHLP_LINE64));
            uint ld;
            if (SymGetLineFromAddr64(h, addr, out ld, ref line))
                s += string.Format("   ({0}:{1})", Marshal.PtrToStringAnsi(line.FileName), line.LineNumber);
        } else {
            s = "<no symbol, err=" + Marshal.GetLastWin32Error() + ">";
        }
        Marshal.FreeHGlobal(buf);
        return s;
    }
}
"@
Add-Type -TypeDefinition $src -Language CSharp
Write-Host ("RVA 0x{0:X} -> {1}" -f $rvaVal, [SymRes]::Resolve($Exe, $rvaVal))
