# dumpstacks.ps1 — attach to a running PID, suspend all threads, walk every
# thread's call stack via StackWalk64 + dbghelp Sym* APIs, then resume.
# Used to figure out where a hung 32-bit process (enations.exe) is stuck.
#
# Usage:
#   ./dumpstacks.ps1 -Pid 1234
#
# Notes:
# - Targets x86 (32-bit) processes. Uses Wow64GetThreadContext + StackWalk64
#   with IMAGE_FILE_MACHINE_I386 because PowerShell here runs x64.
# - Symbol path defaults to the exe's directory (so enations.pdb resolves).

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][Alias('Pid')][int]$ProcessId,
    [string]$SymPath = 'd:\Enemy Nations\src\cmakeBuild\enations_latest\src\Release'
)

$ErrorActionPreference = 'Stop'

$src = @"
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

public static class StackDumper {

    // --- handle / thread enumeration ---
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(uint access, bool inh, uint pid);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")]
    public static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")]
    public static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")]
    public static extern uint Wow64SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")]
    public static extern uint ResumeThread(IntPtr h);

    public const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
    public const uint THREAD_ALL_ACCESS  = 0x1FFFFF;
    // Minimum needed: query basic info, read memory, suspend/resume threads
    public const uint PROCESS_QUERY_INFORMATION = 0x0400;
    public const uint PROCESS_VM_READ           = 0x0010;
    public const uint PROCESS_QUERY_LIMITED     = 0x1000;

    // CreateToolhelp32Snapshot for thread iteration
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll")]
    public static extern bool Thread32First(IntPtr snap, ref THREADENTRY32 te);
    [DllImport("kernel32.dll")]
    public static extern bool Thread32Next(IntPtr snap, ref THREADENTRY32 te);
    public const uint TH32CS_SNAPTHREAD = 0x00000004;

    [StructLayout(LayoutKind.Sequential)]
    public struct THREADENTRY32 {
        public uint dwSize;
        public uint cntUsage;
        public uint th32ThreadID;
        public uint th32OwnerProcessID;
        public int  tpBasePri;
        public int  tpDeltaPri;
        public uint dwFlags;
    }

    // --- Wow64 (32-bit-on-64-bit) context structures ---
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct WOW64_FLOATING_SAVE_AREA {
        public uint   ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] RegisterArea;
        public uint   Cr0NpxState;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct WOW64_CONTEXT {
        public uint ContextFlags;
        public uint Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public WOW64_FLOATING_SAVE_AREA FloatSave;
        public uint SegGs, SegFs, SegEs, SegDs;
        public uint Edi, Esi, Ebx, Edx, Ecx, Eax;
        public uint Ebp, Eip, SegCs, EFlags, Esp, SegSs;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)] public byte[] ExtendedRegisters;
    }

    public const uint WOW64_CONTEXT_i386      = 0x10000;
    public const uint WOW64_CONTEXT_CONTROL   = WOW64_CONTEXT_i386 | 0x1;
    public const uint WOW64_CONTEXT_INTEGER   = WOW64_CONTEXT_i386 | 0x2;
    public const uint WOW64_CONTEXT_SEGMENTS  = WOW64_CONTEXT_i386 | 0x4;
    public const uint WOW64_CONTEXT_FULL      = WOW64_CONTEXT_CONTROL | WOW64_CONTEXT_INTEGER | WOW64_CONTEXT_SEGMENTS;

    [DllImport("kernel32.dll")]
    public static extern bool Wow64GetThreadContext(IntPtr h, ref WOW64_CONTEXT ctx);

    // --- StackWalk64 / Sym* in dbghelp ---
    [StructLayout(LayoutKind.Sequential, Pack = 16)]
    public struct ADDRESS64 { public ulong Offset; public ushort Segment; public uint Mode; }

    [StructLayout(LayoutKind.Sequential)]
    public struct STACKFRAME64 {
        public ADDRESS64 AddrPC;
        public ADDRESS64 AddrReturn;
        public ADDRESS64 AddrFrame;
        public ADDRESS64 AddrStack;
        public ADDRESS64 AddrBStore;
        public IntPtr    FuncTableEntry;
        public ulong     Params0, Params1, Params2, Params3;
        public uint      Far;
        public uint      Virtual;
        public ulong     Reserved0, Reserved1, Reserved2;
        // KDHELP64 — 11 fields (DWORD64 except first which is DWORD64)
        public ulong     KdHelp_Thread;
        public ulong     KdHelp_ThCallbackStack;
        public ulong     KdHelp_ThCallbackBStore;
        public ulong     KdHelp_NextCallback;
        public ulong     KdHelp_FramePointer;
        public ulong     KdHelp_KiCallUserMode;
        public ulong     KdHelp_KeUserCallbackDispatcher;
        public ulong     KdHelp_SystemRangeStart;
        public ulong     KdHelp_KiUserExceptionDispatcher;
        public ulong     KdHelp_StackBase;
        public ulong     KdHelp_StackLimit;
        public ulong     KdHelp_Reserved0;
        public ulong     KdHelp_Reserved1;
        public ulong     KdHelp_Reserved2;
        public ulong     KdHelp_Reserved3;
        public ulong     KdHelp_Reserved4;
    }

    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool StackWalk64(uint MachineType, IntPtr hProc, IntPtr hThread,
                                          ref STACKFRAME64 frame, IntPtr ctxPtr,
                                          IntPtr readMem, IntPtr funcTbl, IntPtr getModBase, IntPtr translateAddress);

    [DllImport("dbghelp.dll")]
    public static extern bool SymInitialize(IntPtr hProc, string symPath, bool fInvadeProcess);
    [DllImport("dbghelp.dll")]
    public static extern bool SymCleanup(IntPtr hProc);
    [DllImport("dbghelp.dll")]
    public static extern uint SymSetOptions(uint opts);

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGEHLP_SYMBOL64 {
        public uint SizeOfStruct;
        public ulong Address;
        public uint Size;
        public uint Flags;
        public uint MaxNameLength;
        // followed by Name buffer
    }

    [DllImport("dbghelp.dll", CharSet = CharSet.Ansi)]
    public static extern bool SymGetSymFromAddr64(IntPtr hProc, ulong addr, out ulong displacement, IntPtr symbol);

    [DllImport("dbghelp.dll", CharSet = CharSet.Ansi)]
    public static extern bool SymGetLineFromAddr64(IntPtr hProc, ulong addr, out uint displacement, ref IMAGEHLP_LINE64 line);

    [StructLayout(LayoutKind.Sequential)]
    public struct IMAGEHLP_LINE64 {
        public uint SizeOfStruct;
        public IntPtr Key;
        public uint LineNumber;
        public IntPtr FileName;
        public ulong Address;
    }

    public const uint IMAGE_FILE_MACHINE_I386 = 0x014c;

    public static string ResolveSym(IntPtr hProc, ulong addr, out string srcLoc) {
        srcLoc = "";
        // IMAGEHLP_SYMBOL64 + 256-char buffer
        const int nameLen = 256;
        int totalLen = Marshal.SizeOf(typeof(IMAGEHLP_SYMBOL64)) + nameLen;
        IntPtr buf = Marshal.AllocHGlobal(totalLen);
        try {
            var sym = new IMAGEHLP_SYMBOL64 {
                SizeOfStruct = (uint)Marshal.SizeOf(typeof(IMAGEHLP_SYMBOL64)),
                MaxNameLength = (uint)nameLen
            };
            Marshal.StructureToPtr(sym, buf, false);
            ulong disp = 0;
            string name = "?";
            if (SymGetSymFromAddr64(hProc, addr, out disp, buf)) {
                IntPtr namePtr = IntPtr.Add(buf, Marshal.SizeOf(typeof(IMAGEHLP_SYMBOL64)));
                name = Marshal.PtrToStringAnsi(namePtr);
            }
            var line = new IMAGEHLP_LINE64 { SizeOfStruct = (uint)Marshal.SizeOf(typeof(IMAGEHLP_LINE64)) };
            uint lineDisp;
            if (SymGetLineFromAddr64(hProc, addr, out lineDisp, ref line)) {
                string fileName = Marshal.PtrToStringAnsi(line.FileName);
                srcLoc = string.Format("({0}:{1})", fileName, line.LineNumber);
            }
            return name;
        } finally {
            Marshal.FreeHGlobal(buf);
        }
    }

    public static void DumpAllStacks(uint pid, string symPath) {
        // Try ALL_ACCESS first; fall back to limited if denied.
        IntPtr hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        int err1 = Marshal.GetLastWin32Error();
        if (hProc == IntPtr.Zero) {
            hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
            int err2 = Marshal.GetLastWin32Error();
            if (hProc == IntPtr.Zero) {
                Console.WriteLine(string.Format("OpenProcess failed: ALL_ACCESS={0}, QUERY|VM_READ={1}", err1, err2));
                return;
            }
            Console.WriteLine("OpenProcess: using QUERY|VM_READ (no all-access)");
        }

        SymSetOptions(0x10 | 0x2 | 0x4);
        if (!SymInitialize(hProc, symPath, true)) {
            Console.WriteLine("SymInitialize failed: " + Marshal.GetLastWin32Error());
        }

        // Enumerate threads
        IntPtr snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == IntPtr.Zero) { Console.WriteLine("snap failed"); return; }

        var threadIds = new System.Collections.Generic.List<uint>();
        var te = new THREADENTRY32 { dwSize = (uint)Marshal.SizeOf(typeof(THREADENTRY32)) };
        if (Thread32First(snap, ref te)) {
            do {
                if (te.th32OwnerProcessID == pid) threadIds.Add(te.th32ThreadID);
            } while (Thread32Next(snap, ref te));
        }
        CloseHandle(snap);
        Console.WriteLine(string.Format("Found {0} threads in PID {1}", threadIds.Count, pid));

        // Open handles to all threads first, then suspend them all together
        var threadHandles = new System.Collections.Generic.Dictionary<uint, IntPtr>();
        foreach (var tid in threadIds) {
            IntPtr ht = OpenThread(THREAD_ALL_ACCESS, false, tid);
            if (ht != IntPtr.Zero) threadHandles[tid] = ht;
        }
        // Suspend all (Wow64 since the target is 32-bit)
        foreach (var kvp in threadHandles) Wow64SuspendThread(kvp.Value);

        try {
            int idx = 0;
            foreach (var kvp in threadHandles) {
                uint tid = kvp.Key;
                IntPtr ht = kvp.Value;
                Console.WriteLine(string.Format("\n--- thread {0} (TID {1}) ---", idx++, tid));

                var ctx = new WOW64_CONTEXT { ContextFlags = WOW64_CONTEXT_FULL };
                if (!Wow64GetThreadContext(ht, ref ctx)) {
                    Console.WriteLine(string.Format("  Wow64GetThreadContext failed: {0}", Marshal.GetLastWin32Error()));
                    continue;
                }

                var frame = new STACKFRAME64();
                frame.AddrPC.Offset    = ctx.Eip;  frame.AddrPC.Mode    = 3; // AddrModeFlat
                frame.AddrFrame.Offset = ctx.Ebp;  frame.AddrFrame.Mode = 3;
                frame.AddrStack.Offset = ctx.Esp;  frame.AddrStack.Mode = 3;

                // StackWalk64 expects a pointer to the context in 32-bit form
                int ctxSize = Marshal.SizeOf(typeof(WOW64_CONTEXT));
                IntPtr ctxPtr = Marshal.AllocHGlobal(ctxSize);
                Marshal.StructureToPtr(ctx, ctxPtr, false);
                try {
                    int depth = 0;
                    while (depth < 40) {
                        bool ok = StackWalk64(IMAGE_FILE_MACHINE_I386, hProc, ht,
                                              ref frame, ctxPtr,
                                              IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
                        if (!ok || frame.AddrPC.Offset == 0) break;
                        string srcLoc;
                        string name = ResolveSym(hProc, frame.AddrPC.Offset, out srcLoc);
                        Console.WriteLine(string.Format("  [{0,2}] 0x{1:X8}  {2} {3}", depth, frame.AddrPC.Offset, name, srcLoc));
                        depth++;
                    }
                } finally {
                    Marshal.FreeHGlobal(ctxPtr);
                }
            }
        } finally {
            // Resume + close
            foreach (var kvp in threadHandles) {
                ResumeThread(kvp.Value);
                CloseHandle(kvp.Value);
            }
            SymCleanup(hProc);
            CloseHandle(hProc);
        }
    }
}
"@

if (-not ('StackDumper' -as [type])) {
    Add-Type -TypeDefinition $src -Language CSharp
}

[StackDumper]::DumpAllStacks([uint32]$ProcessId, $SymPath)
