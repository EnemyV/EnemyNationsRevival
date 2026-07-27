param(
    [int]$Seconds = 25,
    [string]$Exe = 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe',
    [string]$WorkDir = 'd:\Enemy Nations'
)
$src = @"
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

public static class DebugRun {
    // Set once we learn the child's architecture (via IsWow64Process). A 64-bit
    // debugger sees a 32-bit child as WoW64; a native x64 child is not WoW64.
    static bool isChild64 = false;

    const uint DEBUG_PROCESS = 0x00000001;
    const uint DEBUG_ONLY_THIS_PROCESS = 0x00000002;

    const uint EXCEPTION_DEBUG_EVENT = 1;
    const uint EXIT_PROCESS_DEBUG_EVENT = 5;
    const uint LOAD_DLL_DEBUG_EVENT = 6;
    const uint OUTPUT_DEBUG_STRING_EVENT = 8;
    const uint CREATE_PROCESS_DEBUG_EVENT = 3;

    const uint DBG_CONTINUE = 0x00010002;
    const uint DBG_EXCEPTION_NOT_HANDLED = 0x80010001;

    const uint EXCEPTION_BREAKPOINT = 0x80000003;
    const uint STATUS_WX86_BREAKPOINT = 0x4000001F;
    const uint EXCEPTION_ACCESS_VIOLATION = 0xC0000005;

    const uint SYMOPT_LOAD_LINES = 0x10;
    const uint SYMOPT_UNDNAME = 0x2;
    const uint SYMOPT_DEFERRED_LOADS = 0x4;

    [StructLayout(LayoutKind.Sequential)]
    public struct STARTUPINFO { public uint cb; public IntPtr lpReserved, lpDesktop, lpTitle; public uint dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags; public ushort wShowWindow, cbReserved2; public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError; }
    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION { public IntPtr hProcess, hThread; public uint dwProcessId, dwThreadId; }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern bool CreateProcess(string lpApplicationName, string lpCommandLine, IntPtr lpProcessAttributes, IntPtr lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags, IntPtr lpEnvironment, string lpCurrentDirectory, ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);
    [DllImport("kernel32.dll", SetLastError = true, EntryPoint = "WaitForDebugEvent")]
    public static extern bool WaitForDebugEventRaw(IntPtr lpDebugEvent, uint dwMilliseconds);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool ContinueDebugEvent(uint dwProcessId, uint dwThreadId, uint dwContinueStatus);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool DebugSetProcessKillOnExit(bool KillOnExit);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool DebugActiveProcessStop(uint dwProcessId);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint dwSize, out uint lpNumberOfBytesRead);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool IsWow64Process(IntPtr hProcess, out bool Wow64Process);
    // x64 child: native GetThreadContext into a raw (16-byte-aligned) CONTEXT buffer.
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool GetThreadContext(IntPtr hThread, IntPtr lpContext);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenThread(uint dwDesiredAccess, bool bInheritHandle, uint dwThreadId);

    public static IntPtr OpenThreadHandle(uint tid) {
        const uint THREAD_GET_CONTEXT = 0x0008;
        const uint THREAD_QUERY_INFORMATION = 0x0040;
        const uint THREAD_SUSPEND_RESUME = 0x0002;
        return OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, false, tid);
    }

    // dbghelp - 64-bit signatures (we're a 64-bit process). The address space we're symbolicating is
    // the 32-bit child's, but dbghelp doesn't care: it tracks per-process module bases as ULONG64s
    // and resolves to the right PDB regardless.
    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    public static extern bool SymInitialize(IntPtr hProcess, string UserSearchPath, bool fInvadeProcess);
    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern uint SymSetOptions(uint SymOptions);
    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    public static extern ulong SymLoadModuleEx(IntPtr hProcess, IntPtr hFile, string ImageName, string ModuleName, ulong BaseOfDll, uint DllSize, IntPtr Data, uint Flags);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct IMAGEHLP_SYMBOL64 {
        public uint SizeOfStruct;
        public ulong Address;
        public uint Size;
        public uint Flags;
        public uint MaxNameLength;
        // Name follows; we'll allocate a fixed buffer instead and use unsafe access.
    }

    // SymGetSymFromAddr64 takes a pointer to a buffer containing IMAGEHLP_SYMBOL64 + name buffer.
    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool SymGetSymFromAddr64(IntPtr hProcess, ulong qwAddr, out ulong pdwDisplacement, IntPtr Symbol);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct IMAGEHLP_LINE64 {
        public uint SizeOfStruct;
        public IntPtr Key;
        public uint LineNumber;
        public IntPtr FileName;
        public ulong Address;
    }
    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool SymGetLineFromAddr64(IntPtr hProcess, ulong qwAddr, out uint pdwDisplacement, ref IMAGEHLP_LINE64 Line);

    // Stack walking
    const uint IMAGE_FILE_MACHINE_I386 = 0x014c;

    [StructLayout(LayoutKind.Sequential)]
    public struct ADDRESS64 { public ulong Offset; public ushort Segment; public uint Mode; }

    [StructLayout(LayoutKind.Sequential)]
    public struct STACKFRAME64 {
        public ADDRESS64 AddrPC;
        public ADDRESS64 AddrReturn;
        public ADDRESS64 AddrFrame;
        public ADDRESS64 AddrStack;
        public ADDRESS64 AddrBStore;
        public IntPtr FuncTableEntry;
        public ulong Params0, Params1, Params2, Params3;
        public int Far;
        public int Virtual;
        public ulong Reserved0, Reserved1, Reserved2;
        // KDHELP64 (4*8 + 6*8 + padding) — pad with extras
        public ulong KdHelp0, KdHelp1, KdHelp2, KdHelp3, KdHelp4, KdHelp5, KdHelp6, KdHelp7, KdHelp8, KdHelp9, KdHelp10, KdHelp11;
    }

    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool StackWalk64(uint MachineType, IntPtr hProcess, IntPtr hThread, ref STACKFRAME64 StackFrame, IntPtr ContextRecord, IntPtr ReadMemoryRoutine, IntPtr FunctionTableAccessRoutine, IntPtr GetModuleBaseRoutine, IntPtr TranslateAddress);

    [DllImport("dbghelp.dll")]
    public static extern IntPtr SymFunctionTableAccess64(IntPtr hProcess, ulong AddrBase);
    [DllImport("dbghelp.dll")]
    public static extern ulong SymGetModuleBase64(IntPtr hProcess, ulong qwAddr);

    // Wow64 context for x86 child thread
    const uint CONTEXT_i386 = 0x00010000;
    const uint CONTEXT_FULL = CONTEXT_i386 | 0x07; // CONTROL | INTEGER | SEGMENTS

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FLOATING_SAVE_AREA {
        public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] RegisterArea;
        public uint Cr0NpxState;
    }
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct WOW64_CONTEXT {
        public uint ContextFlags;
        public uint Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public FLOATING_SAVE_AREA FloatSave;
        public uint SegGs, SegFs, SegEs, SegDs;
        public uint Edi, Esi, Ebx, Edx, Ecx, Eax;
        public uint Ebp, Eip, SegCs, EFlags, Esp, SegSs;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)] public byte[] ExtendedRegisters;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool Wow64GetThreadContext(IntPtr hThread, ref WOW64_CONTEXT lpContext);

    public delegate IntPtr PFUNC_TABLE_ACCESS(IntPtr hProc, ulong addr);
    public delegate ulong  PGET_MODULE_BASE(IntPtr hProc, ulong addr);

    public static void WalkStack(IntPtr hProc, IntPtr hThr) {
        var ctx = new WOW64_CONTEXT();
        ctx.ContextFlags = CONTEXT_FULL;
        ctx.FloatSave.RegisterArea = new byte[80];
        ctx.ExtendedRegisters = new byte[512];
        if (!Wow64GetThreadContext(hThr, ref ctx)) {
            Console.WriteLine("    Wow64GetThreadContext failed: " + Marshal.GetLastWin32Error());
            return;
        }
        IntPtr ctxPtr = Marshal.AllocHGlobal(Marshal.SizeOf(ctx));
        Marshal.StructureToPtr(ctx, ctxPtr, false);
        var frame = new STACKFRAME64();
        frame.AddrPC.Offset = ctx.Eip; frame.AddrPC.Mode = 3;
        frame.AddrFrame.Offset = ctx.Ebp; frame.AddrFrame.Mode = 3;
        frame.AddrStack.Offset = ctx.Esp; frame.AddrStack.Mode = 3;

        Console.WriteLine("    --- stack ---");
        PFUNC_TABLE_ACCESS dFn = SymFunctionTableAccess64;
        PGET_MODULE_BASE dMod = SymGetModuleBase64;
        IntPtr fnTbl = Marshal.GetFunctionPointerForDelegate(dFn);
        IntPtr modBase = Marshal.GetFunctionPointerForDelegate(dMod);
        GC.KeepAlive(dFn); GC.KeepAlive(dMod);
        for (int i = 0; i < 20; i++) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_I386, hProc, hThr, ref frame, ctxPtr, IntPtr.Zero, fnTbl, modBase, IntPtr.Zero)) break;
            if (frame.AddrPC.Offset == 0) break;
            string s = ResolveSymbolPub(hProc, frame.AddrPC.Offset);
            Console.WriteLine(string.Format("    [{0,2}] 0x{1:X8}  {2}", i, frame.AddrPC.Offset, s));
        }
        Marshal.FreeHGlobal(ctxPtr);
    }

    // x64 child stack walk. The AMD64 CONTEXT is ~1232 bytes and must be 16-byte
    // aligned; we over-allocate and read registers by offset rather than marshal
    // the whole struct. StackWalk64 with IMAGE_FILE_MACHINE_AMD64 uses the PDATA
    // unwind tables (via SymFunctionTableAccess64), so AddrFrame is advisory.
    const uint IMAGE_FILE_MACHINE_AMD64 = 0x8664;
    public static void WalkStackX64(IntPtr hProc, IntPtr hThr) {
        const int CTX_SIZE = 1232;
        const int OFF_FLAGS = 0x30, OFF_RSP = 0x98, OFF_RBP = 0xA0, OFF_RIP = 0xF8;
        const uint CONTEXT_AMD64_FULL = 0x00100000 | 0x1 | 0x2 | 0x8; // CONTROL|INTEGER|FLOATING_POINT
        IntPtr raw = Marshal.AllocHGlobal(CTX_SIZE + 16);
        try {
            long aligned = (raw.ToInt64() + 15) & ~15L;
            IntPtr ctx = new IntPtr(aligned);
            for (int i = 0; i < CTX_SIZE; i++) Marshal.WriteByte(ctx, i, 0);
            Marshal.WriteInt32(ctx, OFF_FLAGS, unchecked((int)CONTEXT_AMD64_FULL));
            if (!GetThreadContext(hThr, ctx)) {
                Console.WriteLine("    GetThreadContext failed: " + Marshal.GetLastWin32Error());
                return;
            }
            ulong rip = (ulong)Marshal.ReadInt64(ctx, OFF_RIP);
            ulong rsp = (ulong)Marshal.ReadInt64(ctx, OFF_RSP);
            ulong rbp = (ulong)Marshal.ReadInt64(ctx, OFF_RBP);
            var frame = new STACKFRAME64();
            frame.AddrPC.Offset = rip;    frame.AddrPC.Mode = 3;
            frame.AddrFrame.Offset = rbp; frame.AddrFrame.Mode = 3;
            frame.AddrStack.Offset = rsp; frame.AddrStack.Mode = 3;
            Console.WriteLine("    --- stack (x64) ---");
            PFUNC_TABLE_ACCESS dFn = SymFunctionTableAccess64;
            PGET_MODULE_BASE dMod = SymGetModuleBase64;
            IntPtr fnTbl = Marshal.GetFunctionPointerForDelegate(dFn);
            IntPtr modBase = Marshal.GetFunctionPointerForDelegate(dMod);
            GC.KeepAlive(dFn); GC.KeepAlive(dMod);
            for (int i = 0; i < 30; i++) {
                if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProc, hThr, ref frame, ctx, IntPtr.Zero, fnTbl, modBase, IntPtr.Zero)) break;
                if (frame.AddrPC.Offset == 0) break;
                string s = ResolveSymbol(hProc, frame.AddrPC.Offset);
                Console.WriteLine(string.Format("    [{0,2}] 0x{1:X16}  {2}", i, frame.AddrPC.Offset, s));
            }
        } finally { Marshal.FreeHGlobal(raw); }
    }

    // Open the faulting thread and walk it with the arch-appropriate walker.
    public static void WalkCurrent(IntPtr hProc, uint tid) {
        IntPtr hThr = OpenThreadHandle(tid);
        if (hThr == IntPtr.Zero) { Console.WriteLine("    OpenThread failed: " + Marshal.GetLastWin32Error()); return; }
        if (isChild64) WalkStackX64(hProc, hThr); else WalkStack(hProc, hThr);
        CloseHandle(hThr);
    }

    public static string ResolveSymbolPub(IntPtr hProcess, ulong addr) { return ResolveSymbol(hProcess, addr); }

    static string ResolveSymbol(IntPtr hProcess, ulong addr) {
        const int nameMax = 512;
        // IMAGEHLP_SYMBOL64 layout (sequential): DWORD(4)+ULONG64(8)+DWORD(4)+DWORD(4)+DWORD(4)=24 padded to 32, then Name[1] min.
        int structSize = 32;
        int total = structSize + nameMax;
        IntPtr buf = Marshal.AllocHGlobal(total);
        try {
            for (int i = 0; i < total; i++) Marshal.WriteByte(buf, i, 0);
            Marshal.WriteInt32(buf, 0, structSize);   // SizeOfStruct
            Marshal.WriteInt64(buf, 8, 0);            // Address (out)
            Marshal.WriteInt32(buf, 16, 0);           // Size (out)
            Marshal.WriteInt32(buf, 20, 0);           // Flags (out)
            Marshal.WriteInt32(buf, 24, nameMax - 1); // MaxNameLength
            ulong disp;
            if (SymGetSymFromAddr64(hProcess, addr, out disp, buf)) {
                IntPtr namePtr = IntPtr.Add(buf, structSize);
                string name = Marshal.PtrToStringAnsi(namePtr);
                string s = string.Format("{0}+0x{1:X}", name, disp);

                IMAGEHLP_LINE64 line = new IMAGEHLP_LINE64();
                line.SizeOfStruct = (uint)Marshal.SizeOf(typeof(IMAGEHLP_LINE64));
                uint lineDisp;
                if (SymGetLineFromAddr64(hProcess, addr, out lineDisp, ref line)) {
                    string file = Marshal.PtrToStringAnsi(line.FileName);
                    s += string.Format("  ({0}:{1})", file, line.LineNumber);
                }
                return s;
            } else {
                int err = Marshal.GetLastWin32Error();
                return string.Format("<no symbol, err={0}>", err);
            }
        } finally { Marshal.FreeHGlobal(buf); }
    }

    public static int Run(string exePath, string workDir, int maxSeconds) {
        var si = new STARTUPINFO();
        si.cb = (uint)Marshal.SizeOf(si);
        PROCESS_INFORMATION pi;
        bool ok = CreateProcess(null, exePath, IntPtr.Zero, IntPtr.Zero, false, DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, IntPtr.Zero, workDir, ref si, out pi);
        if (!ok) { Console.WriteLine("CreateProcess failed: " + Marshal.GetLastWin32Error()); return -1; }
        DebugSetProcessKillOnExit(false); // debugger exit must never kill the game
        Console.WriteLine("[debugger] started pid=" + pi.dwProcessId);

        // Don't defer — we want immediate symbol loading per module.
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        string symPath = Path.GetDirectoryName(exePath);
        if (!SymInitialize(pi.hProcess, symPath, false)) {
            Console.WriteLine("SymInitialize failed: " + Marshal.GetLastWin32Error());
        } else {
            Console.WriteLine("SymInitialize OK with path=" + symPath);
        }

        bool wow; if (IsWow64Process(pi.hProcess, out wow)) { isChild64 = !wow; }
        Console.WriteLine("[arch] child process is " + (isChild64 ? "x64 (native)" : "x86 (WoW64)"));

        int bufSize = 256;
        IntPtr buf = Marshal.AllocHGlobal(bufSize);
        try {
            var sw = Stopwatch.StartNew();
            int firstChanceBreakpointsSeen = 0;
            while (sw.Elapsed.TotalSeconds < maxSeconds) {
                for (int i = 0; i < bufSize; i++) Marshal.WriteByte(buf, i, 0);
                if (!WaitForDebugEventRaw(buf, 1000)) continue;

                uint code = (uint)Marshal.ReadInt32(buf, 0);
                uint pid = (uint)Marshal.ReadInt32(buf, 4);
                uint tid = (uint)Marshal.ReadInt32(buf, 8);
                uint cont = DBG_CONTINUE;

                try {
                switch (code) {
                    case OUTPUT_DEBUG_STRING_EVENT: {
                        IntPtr pStr = Marshal.ReadIntPtr(buf, 16);
                        ushort fUnicode = (ushort)Marshal.ReadInt16(buf, 24);
                        ushort len = (ushort)Marshal.ReadInt16(buf, 26);
                        if (len > 0 && len < 4096) {
                            byte[] sb = new byte[len];
                            uint read;
                            if (ReadProcessMemory(pi.hProcess, pStr, sb, (uint)sb.Length, out read)) {
                                string msg = (fUnicode != 0) ? Encoding.Unicode.GetString(sb, 0, (int)read) : Encoding.ASCII.GetString(sb, 0, (int)read);
                                msg = msg.TrimEnd('\0', '\n', '\r');
                                if (msg.Length > 0) Console.WriteLine("[ODS] " + msg);
                            }
                        }
                        break;
                    }
                    case CREATE_PROCESS_DEBUG_EVENT: {
                        // CREATE_PROCESS_DEBUG_INFO layout (64-bit). hFile is first field.
                        // hProcess, hThread, lpBaseOfImage, ... — but easier: just call SymLoadModuleEx for the exe itself once we get this event.
                        IntPtr hFile = Marshal.ReadIntPtr(buf, 16);
                        IntPtr hProc = Marshal.ReadIntPtr(buf, 24);
                        IntPtr hThr  = Marshal.ReadIntPtr(buf, 32);
                        ulong lpBase = (ulong)Marshal.ReadInt64(buf, 40);
                        ulong res = SymLoadModuleEx(pi.hProcess, hFile, exePath, null, lpBase, 0, IntPtr.Zero, 0);
                        Console.WriteLine(string.Format("[create-proc] exe base=0x{0:X16} loadRes={1}", lpBase, res));
                        break;
                    }
                    case LOAD_DLL_DEBUG_EVENT: {
                        // LOAD_DLL_DEBUG_INFO layout: hFile (PTR @0), lpBaseOfDll (PTR @8), ...
                        IntPtr hFile = Marshal.ReadIntPtr(buf, 16);
                        ulong lpBase = (ulong)Marshal.ReadInt64(buf, 24);
                        ulong res = SymLoadModuleEx(pi.hProcess, hFile, null, null, lpBase, 0, IntPtr.Zero, 0);
                        break;
                    }
                    case EXCEPTION_DEBUG_EVENT: {
                        uint excCode = (uint)Marshal.ReadInt32(buf, 16);
                        ulong excAddr = (ulong)Marshal.ReadInt64(buf, 16 + 16);
                        uint firstChance = (uint)Marshal.ReadInt32(buf, 16 + 152);
                        string sym = ResolveSymbol(pi.hProcess, excAddr);
                        Console.WriteLine(string.Format("[EXC] code=0x{0:X8} addr=0x{1:X16} firstChance={2}", excCode, excAddr, firstChance));
                        Console.WriteLine("      " + sym);

                        bool isBp = (excCode == EXCEPTION_BREAKPOINT || excCode == STATUS_WX86_BREAKPOINT);
                        bool isAv = (excCode == EXCEPTION_ACCESS_VIOLATION);
                        if (isBp) {
                            firstChanceBreakpointsSeen++;
                            if (firstChanceBreakpointsSeen == 1) Console.WriteLine("      (initial loader breakpoint)");
                            else {
                                Console.WriteLine("      *** ASSERTION-style breakpoint — walking stack ***");
                                WalkCurrent(pi.hProcess, tid);
                            }
                            cont = DBG_CONTINUE;
                        } else if (isAv) {
                            // The real crash. Walk the faulting thread, then let it
                            // propagate (DBG_EXCEPTION_NOT_HANDLED) so SEH/handlers
                            // still run — an unhandled AV will then exit the process.
                            Console.WriteLine("      *** ACCESS VIOLATION (firstChance=" + firstChance + ") — walking stack ***");
                            WalkCurrent(pi.hProcess, tid);
                            cont = DBG_EXCEPTION_NOT_HANDLED;
                        } else {
                            cont = DBG_EXCEPTION_NOT_HANDLED;
                        }
                        break;
                    }
                    case EXIT_PROCESS_DEBUG_EVENT: {
                        uint exitCode = (uint)Marshal.ReadInt32(buf, 16);
                        Console.WriteLine(string.Format("[exit] code=0x{0:X8}", exitCode));
                        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                        return (int)exitCode;
                    }
                }
                } catch (Exception _dbgEx) {
                    Console.WriteLine("[dbgcatch] event-handler exception (continuing): " + _dbgEx.Message);
                }
                ContinueDebugEvent(pid, tid, cont);
            }
            // timeout: DETACH so the game keeps running (abandoning the debug port
            // would suspend the next ODS-raising thread and orphan the debuggee)
            DebugActiveProcessStop(pi.dwProcessId);
            Console.WriteLine("[timeout] detached; game keeps running");
            return -1;
        } finally { Marshal.FreeHGlobal(buf); }
    }
}
"@
Add-Type -TypeDefinition $src -Language CSharp
Write-Host "[dbgcatch] exe=$Exe  workdir=$WorkDir  timeout=${Seconds}s"
[DebugRun]::Run($Exe, $WorkDir, $Seconds)
