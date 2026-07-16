# dmpexc.ps1 - print a minidump's exception (code, address, module offset, symbol)
param([Parameter(Mandatory=$true)][string]$Dump,
      [string]$ExePdbDir = "$PSScriptRoot\cmakeBuild-x64\enations_latest\src\Debug")

if (-not ('DmpExc' -as [type])) {
Add-Type @'
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
public static class DmpExc {
    [DllImport("dbghelp.dll", SetLastError=true)]
    public static extern bool MiniDumpReadDumpStream(IntPtr BaseOfDump, uint StreamNumber,
        out IntPtr Dir, out IntPtr StreamPointer, out uint StreamSize);
    [DllImport("dbghelp.dll", SetLastError=true)]
    public static extern bool SymInitialize(IntPtr hProcess, string SearchPath, bool fInvade);
    [DllImport("dbghelp.dll", SetLastError=true)]
    public static extern ulong SymLoadModuleEx(IntPtr hProcess, IntPtr hFile, string ImageName,
        string ModuleName, ulong BaseOfDll, uint DllSize, IntPtr Data, uint Flags);
    [DllImport("dbghelp.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern bool SymFromAddr(IntPtr hProcess, ulong Address, out ulong Displacement, IntPtr Symbol);
    [DllImport("dbghelp.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern bool SymGetLineFromAddr64(IntPtr hProcess, ulong Address, out uint Displacement, IntPtr Line);
}
'@
}

# memory-map the dump (full dumps exceed the 2GB ReadAllBytes ceiling)
$mmf  = [IO.MemoryMappedFiles.MemoryMappedFile]::CreateFromFile($Dump, [IO.FileMode]::Open, "dmpexc_$PID", 0, [IO.MemoryMappedFiles.MemoryMappedFileAccess]::Read)
$view = $mmf.CreateViewAccessor(0, 0, [IO.MemoryMappedFiles.MemoryMappedFileAccess]::Read)
try {
  $base = $view.SafeMemoryMappedViewHandle.DangerousGetHandle()
  $dir=[IntPtr]::Zero; $ptr=[IntPtr]::Zero; $size=[uint32]0
  # 6 = ExceptionStream
  if (-not [DmpExc]::MiniDumpReadDumpStream($base, 6, [ref]$dir, [ref]$ptr, [ref]$size)) { throw "no exception stream" }
  $tid  = [Runtime.InteropServices.Marshal]::ReadInt32($ptr, 0)
  # MINIDUMP_EXCEPTION starts at offset 8: code(0), flags(4), record(8), address(16)
  $code = [Runtime.InteropServices.Marshal]::ReadInt32($ptr, 8)
  $addr = [Runtime.InteropServices.Marshal]::ReadInt64($ptr, 24)
  "thread {0}  code 0x{1:X8}  address 0x{2:X}" -f $tid, $code, $addr

  # 4 = ModuleListStream: first module = exe; base at offset 4 (after count:uint64? layout: NumberOfModules ULONG32 then modules)
  if ([DmpExc]::MiniDumpReadDumpStream($base, 4, [ref]$dir, [ref]$ptr, [ref]$size)) {
    $n = [Runtime.InteropServices.Marshal]::ReadInt32($ptr, 0)
    $modBase = [Runtime.InteropServices.Marshal]::ReadInt64($ptr, 4)   # first MINIDUMP_MODULE.BaseOfImage
    "exe base 0x{0:X}  (offset of crash = 0x{1:X})" -f $modBase, ($addr - $modBase)

    # resolve via PDB
    $hp = [IntPtr]::new(4660)
    [DmpExc]::SymInitialize($hp, $ExePdbDir, $false) | Out-Null
    $exe = Join-Path $ExePdbDir "enations.exe"
    $loaded = [DmpExc]::SymLoadModuleEx($hp, [IntPtr]::Zero, $exe, "enations", [uint64]$modBase, 0, [IntPtr]::Zero, 0)
    if ($loaded -ne 0) {
      $symBuf = [Runtime.InteropServices.Marshal]::AllocHGlobal(88 + 512)
      [Runtime.InteropServices.Marshal]::WriteInt32($symBuf, 0, 88)      # SizeOfStruct
      [Runtime.InteropServices.Marshal]::WriteInt32($symBuf, 80, 512)    # MaxNameLen
      $disp=[uint64]0
      if ([DmpExc]::SymFromAddr($hp, [uint64]$addr, [ref]$disp, $symBuf)) {
        $name = [Runtime.InteropServices.Marshal]::PtrToStringAnsi([IntPtr]::Add($symBuf, 84))
        "symbol: $name + 0x{0:X}" -f $disp
      } else { "symbol: (SymFromAddr failed $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))" }
      $lineBuf = [Runtime.InteropServices.Marshal]::AllocHGlobal(48)
      [Runtime.InteropServices.Marshal]::WriteInt32($lineBuf, 0, 48)
      $ld=[uint32]0
      if ([DmpExc]::SymGetLineFromAddr64($hp, [uint64]$addr, [ref]$ld, $lineBuf)) {
        $fn = [Runtime.InteropServices.Marshal]::PtrToStringAnsi([Runtime.InteropServices.Marshal]::ReadIntPtr($lineBuf, 16))
        $ln = [Runtime.InteropServices.Marshal]::ReadInt32($lineBuf, 24)
        "line: ${fn}:${ln}"
      }
    } else { "SymLoadModuleEx failed" }
  }
} finally { $view.Dispose(); $mmf.Dispose() }
