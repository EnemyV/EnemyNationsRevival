# dbgstack.ps1 — on-demand call-stack snapshot of the RUNNING Enemy Nations.
#
# A sampling debugger: it suspends the target thread(s), walks the stack with
# dbghelp + the build's PDB, symbolizes (func + file:line), then resumes. It does
# NOT attach as a debugger and never kills the game — safe to run any time.
#
# Usage:
#   ./dbgstack.ps1                    # UI thread (the window-owning thread), 24 frames
#   ./dbgstack.ps1 -Thread all        # every thread in the process
#   ./dbgstack.ps1 -Thread 12345      # a specific TID
#   ./dbgstack.ps1 -Frames 40
#   ./dbgstack.ps1 -OnlyGame          # only show frames that resolve to enations.exe symbols
#   ./dbgstack.ps1 -Pid 1234 -SymPath "d:\...\Debug"
#
# Target bitness is auto-detected. The current 32-bit (WOW64) build is fully
# supported; for a future 64-bit build it reports that the 64-bit walker isn't
# wired yet (the WOW64 context path won't apply).

[CmdletBinding()]
param(
    [int]$ProcId,
    [string]$ProcessName,
    [string]$Thread = 'main',     # 'main' (UI thread) | 'all' | <tid>
    [int]$Frames = 24,
    [switch]$OnlyGame,
    [string]$SymPath
)

$ErrorActionPreference = 'Stop'

if (-not ('DbgSample' -as [type])) {
Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class DbgSample {
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint a, bool inh, uint pid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenThread(uint a, bool inh, uint tid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool IsWow64Process(IntPtr h, out bool wow);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool Wow64GetThreadContext(IntPtr h, ref WOW64_CONTEXT c);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool GetThreadContext(IntPtr h, IntPtr ctx);

    [DllImport("dbghelp.dll", SetLastError=true, CharSet=CharSet.Ansi)] public static extern bool SymInitialize(IntPtr h, string path, bool invade);
    [DllImport("dbghelp.dll")] public static extern uint SymSetOptions(uint o);
    [DllImport("dbghelp.dll")] public static extern bool SymCleanup(IntPtr h);
    [DllImport("dbghelp.dll", SetLastError=true)] public static extern bool SymGetSymFromAddr64(IntPtr h, ulong addr, out ulong disp, IntPtr sym);
    [DllImport("dbghelp.dll", SetLastError=true)] public static extern bool SymGetLineFromAddr64(IntPtr h, ulong addr, out uint disp, ref IMAGEHLP_LINE64 line);
    [DllImport("dbghelp.dll")] public static extern bool StackWalk64(uint mt, IntPtr hP, IntPtr hT, ref STACKFRAME64 f, IntPtr ctx, IntPtr rd, IntPtr fta, IntPtr gmb, IntPtr ta);
    [DllImport("dbghelp.dll")] public static extern IntPtr SymFunctionTableAccess64(IntPtr h, ulong b);
    [DllImport("dbghelp.dll")] public static extern ulong SymGetModuleBase64(IntPtr h, ulong a);

    const uint IMAGE_FILE_MACHINE_I386 = 0x014c;
    const uint IMAGE_FILE_MACHINE_AMD64 = 0x8664;
    const uint CONTEXT_i386 = 0x00010000;
    const uint CONTEXT_FULL = CONTEXT_i386 | 0x07;
    const uint PROC_ACCESS = 0x0410;   // QUERY_INFORMATION | VM_READ
    const uint THREAD_ACCESS = 0x004A; // GET_CONTEXT | SUSPEND_RESUME | QUERY_INFORMATION
    const uint SYMOPT = 0x10 | 0x2 | 0x4; // LOAD_LINES | UNDNAME | DEFERRED_LOADS

    [StructLayout(LayoutKind.Sequential)] public struct ADDRESS64 { public ulong Offset; public ushort Segment; public uint Mode; }
    [StructLayout(LayoutKind.Sequential)] public struct STACKFRAME64 {
        public ADDRESS64 AddrPC, AddrReturn, AddrFrame, AddrStack, AddrBStore;
        public IntPtr FuncTableEntry;
        public ulong P0,P1,P2,P3; public int Far; public int Virtual; public ulong R0,R1,R2;
        public ulong K0,K1,K2,K3,K4,K5,K6,K7,K8,K9,K10,K11;
    }
    [StructLayout(LayoutKind.Sequential, Pack=4)] public struct FLOATING_SAVE_AREA {
        public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)] public byte[] RegisterArea; public uint Cr0NpxState;
    }
    [StructLayout(LayoutKind.Sequential, Pack=4)] public struct WOW64_CONTEXT {
        public uint ContextFlags; public uint Dr0,Dr1,Dr2,Dr3,Dr6,Dr7; public FLOATING_SAVE_AREA FloatSave;
        public uint SegGs,SegFs,SegEs,SegDs; public uint Edi,Esi,Ebx,Edx,Ecx,Eax; public uint Ebp,Eip,SegCs,EFlags,Esp,SegSs;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=512)] public byte[] ExtendedRegisters;
    }
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)] public struct IMAGEHLP_LINE64 {
        public uint SizeOfStruct; public IntPtr Key; public uint LineNumber; public IntPtr FileName; public ulong Address;
    }
    public delegate IntPtr PFTA(IntPtr h, ulong a);
    public delegate ulong PGMB(IntPtr h, ulong a);

    static IntPtr hProc = IntPtr.Zero;
    public static bool TargetIsWow64 = false;

    public static string Init(uint pid, string symPath) {
        hProc = OpenProcess(PROC_ACCESS, false, pid);
        if (hProc == IntPtr.Zero) return "OpenProcess failed: " + Marshal.GetLastWin32Error();
        bool wow; if (!IsWow64Process(hProc, out wow)) return "IsWow64Process failed: " + Marshal.GetLastWin32Error();
        TargetIsWow64 = wow;
        SymSetOptions(SYMOPT);
        if (!SymInitialize(hProc, symPath, true)) return "SymInitialize failed: " + Marshal.GetLastWin32Error();
        return null;
    }
    public static void Cleanup() { if (hProc != IntPtr.Zero) { SymCleanup(hProc); CloseHandle(hProc); hProc = IntPtr.Zero; } }

    // IMAGEHLP_SYMBOL64 (8-byte aligned): SizeOfStruct@0, Address@8, Size@16,
    // Flags@20, MaxNameLength@24, Name[]@28.
    const int SYM_NAME_OFFSET = 28;
    static string Resolve(ulong addr, bool onlyGame, out bool isGame) {
        isGame = false;
        int structSize = 32, nameMax = 512, total = SYM_NAME_OFFSET + nameMax;
        IntPtr buf = Marshal.AllocHGlobal(total);
        try {
            for (int i=0;i<total;i++) Marshal.WriteByte(buf,i,0);
            Marshal.WriteInt32(buf,0,structSize);
            Marshal.WriteInt32(buf,24,nameMax-1);
            ulong disp;
            if (SymGetSymFromAddr64(hProc, addr, out disp, buf)) {
                string name = Marshal.PtrToStringAnsi(IntPtr.Add(buf, SYM_NAME_OFFSET));
                if (name != null && name.StartsWith("enations", StringComparison.OrdinalIgnoreCase)) isGame = true;
                string s = string.Format("{0}+0x{1:X}", name, disp);
                IMAGEHLP_LINE64 line = new IMAGEHLP_LINE64();
                line.SizeOfStruct = (uint)Marshal.SizeOf(typeof(IMAGEHLP_LINE64));
                uint ld;
                if (SymGetLineFromAddr64(hProc, addr, out ld, ref line)) {
                    string file = Marshal.PtrToStringAnsi(line.FileName);
                    if (file != null) { int k = file.LastIndexOf('\\'); if (k>=0) file = file.Substring(k+1); }
                    s += string.Format("  ({0}:{1})", file, line.LineNumber);
                    isGame = true; // has source line => our code
                }
                return s;
            }
            return string.Format("0x{0:X8} <no symbol>", addr);
        } finally { Marshal.FreeHGlobal(buf); }
    }

    // Prints frames directly to stdout (Console.WriteLine) — same pattern as dbgcatch,
    // which avoids returning collections to PowerShell. Returns # of frames printed.
    public static int SampleThread(uint tid, int maxFrames, bool onlyGame) {
        IntPtr hThr = OpenThread(THREAD_ACCESS, false, tid);
        if (hThr == IntPtr.Zero) { Console.WriteLine("  OpenThread failed: " + Marshal.GetLastWin32Error()); return 0; }
        SuspendThread(hThr);
        try {
            return TargetIsWow64 ? WalkWow64(hThr, maxFrames, onlyGame)
                                 : WalkX64(hThr, maxFrames, onlyGame);
        } finally { ResumeThread(hThr); CloseHandle(hThr); }
    }

    // Native x64 walker: GetThreadContext into a 16-byte-aligned raw CONTEXT
    // buffer (over-allocate + read registers by offset), then StackWalk64 with
    // IMAGE_FILE_MACHINE_AMD64 (uses PDATA unwind via SymFunctionTableAccess64).
    static int WalkX64(IntPtr hThr, int maxFrames, bool onlyGame) {
        const int CTX_SIZE = 1232;
        const int OFF_FLAGS = 0x30, OFF_RSP = 0x98, OFF_RBP = 0xA0, OFF_RIP = 0xF8;
        const uint CONTEXT_AMD64_FULL = 0x00100000 | 0x1 | 0x2 | 0x8;
        int shown = 0;
        IntPtr raw = Marshal.AllocHGlobal(CTX_SIZE + 16);
        try {
            long aligned = (raw.ToInt64() + 15) & ~15L;
            IntPtr ctx = new IntPtr(aligned);
            for (int i = 0; i < CTX_SIZE; i++) Marshal.WriteByte(ctx, i, 0);
            Marshal.WriteInt32(ctx, OFF_FLAGS, unchecked((int)CONTEXT_AMD64_FULL));
            if (!GetThreadContext(hThr, ctx)) { Console.WriteLine("  GetThreadContext failed: " + Marshal.GetLastWin32Error()); return 0; }
            var fr = new STACKFRAME64();
            fr.AddrPC.Offset    = (ulong)Marshal.ReadInt64(ctx, OFF_RIP); fr.AddrPC.Mode = 3;
            fr.AddrFrame.Offset = (ulong)Marshal.ReadInt64(ctx, OFF_RBP); fr.AddrFrame.Mode = 3;
            fr.AddrStack.Offset = (ulong)Marshal.ReadInt64(ctx, OFF_RSP); fr.AddrStack.Mode = 3;
            PFTA dF = SymFunctionTableAccess64; PGMB dM = SymGetModuleBase64;
            IntPtr pF = Marshal.GetFunctionPointerForDelegate(dF), pM = Marshal.GetFunctionPointerForDelegate(dM);
            GC.KeepAlive(dF); GC.KeepAlive(dM);
            for (int i=0; i<maxFrames+40 && shown<maxFrames; i++) {
                if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProc, hThr, ref fr, ctx, IntPtr.Zero, pF, pM, IntPtr.Zero)) break;
                if (fr.AddrPC.Offset == 0) break;
                bool isGame; string sym = Resolve(fr.AddrPC.Offset, onlyGame, out isGame);
                if (onlyGame && !isGame) continue;
                Console.WriteLine(string.Format("  [{0,2}] 0x{1:X16}  {2}", shown, fr.AddrPC.Offset, sym));
                shown++;
            }
        } finally { Marshal.FreeHGlobal(raw); }
        return shown;
    }

    static int WalkWow64(IntPtr hThr, int maxFrames, bool onlyGame) {
        int shown = 0;
        {
            var ctx = new WOW64_CONTEXT();
            ctx.ContextFlags = CONTEXT_FULL;
            ctx.FloatSave = new FLOATING_SAVE_AREA(); ctx.FloatSave.RegisterArea = new byte[80];
            ctx.ExtendedRegisters = new byte[512];
            if (!Wow64GetThreadContext(hThr, ref ctx)) { Console.WriteLine("  Wow64GetThreadContext failed: " + Marshal.GetLastWin32Error()); return 0; }
            IntPtr ctxPtr = Marshal.AllocHGlobal(Marshal.SizeOf(ctx));
            Marshal.StructureToPtr(ctx, ctxPtr, false);
            var fr = new STACKFRAME64();
            fr.AddrPC.Offset = ctx.Eip; fr.AddrPC.Mode = 3;
            fr.AddrFrame.Offset = ctx.Ebp; fr.AddrFrame.Mode = 3;
            fr.AddrStack.Offset = ctx.Esp; fr.AddrStack.Mode = 3;
            PFTA dF = SymFunctionTableAccess64; PGMB dM = SymGetModuleBase64;
            IntPtr pF = Marshal.GetFunctionPointerForDelegate(dF), pM = Marshal.GetFunctionPointerForDelegate(dM);
            GC.KeepAlive(dF); GC.KeepAlive(dM);
            for (int i=0; i<maxFrames+40 && shown<maxFrames; i++) {
                if (!StackWalk64(IMAGE_FILE_MACHINE_I386, hProc, hThr, ref fr, ctxPtr, IntPtr.Zero, pF, pM, IntPtr.Zero)) break;
                if (fr.AddrPC.Offset == 0) break;
                bool isGame; string sym = Resolve(fr.AddrPC.Offset, onlyGame, out isGame);
                if (onlyGame && !isGame) continue;
                Console.WriteLine(string.Format("  [{0,2}] 0x{1:X8}  {2}", shown, fr.AddrPC.Offset, sym));
                shown++;
            }
            Marshal.FreeHGlobal(ctxPtr);
        }
        return shown;
    }
}
'@
}

# --- resolve target process ---
$proc = $null
if ($ProcId -gt 0) { $proc = Get-Process -Id $ProcId -ErrorAction SilentlyContinue }
else {
    $name = if ($ProcessName) { @($ProcessName) } else { @('enations','enations_gate') }
    foreach ($n in $name) { $proc = Get-Process $n -ErrorAction SilentlyContinue | Select-Object -First 1; if ($proc) { break } }
}
if (-not $proc) { Write-Error "game process not found"; exit 1 }

# symbol search path: prefer the explicit one, else the loaded module's dir (PDB sits next to it)
if (-not $SymPath) {
    try { $SymPath = Split-Path -Parent $proc.MainModule.FileName } catch { $SymPath = '' }
    if (-not $SymPath -or -not (Test-Path (Join-Path $SymPath 'enations.pdb'))) {
        $dbg = "D:\Enemy Nations\src\cmakeBuild\enations_latest\src\Debug"
        if (Test-Path (Join-Path $dbg 'enations.pdb')) { $SymPath = $dbg }
    }
}

$err = [DbgSample]::Init([uint32]$proc.Id, $SymPath)
if ($err) { Write-Error $err; exit 2 }

try {
    Write-Output ("pid {0} ({1})  wow64={2}  symPath={3}" -f $proc.Id, $proc.ProcessName, [DbgSample]::TargetIsWow64, $SymPath)

    # pick threads
    $tids = @()
    if ($Thread -eq 'all') {
        $tids = $proc.Threads | ForEach-Object { [uint32]$_.Id }
    } elseif ($Thread -eq 'main') {
        # UI thread = the one owning the largest visible window of this process
        Add-Type @'
using System; using System.Runtime.InteropServices; using System.Collections.Generic;
public static class WinThr {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
    public static uint BestUiThread(uint target) {
        uint best=0; int bestArea=-1;
        EnumWindows(delegate(IntPtr h, IntPtr p){
            uint wp; uint t = GetWindowThreadProcessId(h, out wp);
            if (wp==target && IsWindowVisible(h)) { RECT r; GetClientRect(h, out r); int a=(r.R-r.L)*(r.B-r.T);
                if (a>bestArea){bestArea=a;best=t;} }
            return true;
        }, IntPtr.Zero);
        return best;
    }
}
'@ -ErrorAction SilentlyContinue
        $ui = [WinThr]::BestUiThread([uint32]$proc.Id)
        if ($ui -ne 0) { $tids = @([uint32]$ui) } else { $tids = @([uint32]($proc.Threads | Sort-Object Id | Select-Object -First 1).Id) }
    } else {
        $tids = @([uint32]$Thread)
    }

    foreach ($tid in $tids) {
        $tag = if ($Thread -eq 'main') { ' (UI)' } else { '' }
        Write-Output ("=== thread {0}{1} ===" -f $tid, $tag)
        $n = [DbgSample]::SampleThread([uint32]$tid, [int]$Frames, [bool]$OnlyGame)
        if ($n -eq 0) { Write-Output "  (no frames)" }
    }
}
finally { [DbgSample]::Cleanup() }
exit 0
