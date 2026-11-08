//---------------------------------------------------------------------------
// cpufeatures.h — runtime CPU capability detection for ISA dispatch.
//
// WHY THIS EXISTS: the Windows Release build used to be compiled with
// /arch:AVX2, which let MSVC emit AVX2 anywhere in the binary. That made the
// shipped game refuse to start on any CPU older than Intel Haswell (2013) /
// AMD Excavator (2015) — an illegal-instruction fault during init, with no
// dialog and no log entry. Users saw only a brief "Loading Second Chance..."
// splash and then nothing (GH #8). Compiling from source did not help, because
// the flag was in CMakeLists.
//
// The binary is now built at the baseline x86-64 level (SSE2, guaranteed on
// every x64 CPU), so it starts everywhere. Anything wider must be selected HERE
// at runtime, per-function, after asking the CPU what it actually supports.
//
// RULE: never add /arch: back to the enations target. Compile a wide variant in
// its OWN translation unit with its own /arch: flag, and reach it only through a
// pointer chosen by these queries. A wide instruction on a path the baseline can
// reach is a crash on someone's machine — silent, and unreproducible in-house
// because every dev box here has AVX2.
//---------------------------------------------------------------------------
#ifndef ENATIONS_CPUFEATURES_H
#define ENATIONS_CPUFEATURES_H

namespace EnCpu
{

// Feature queries. Cheap: CPUID runs once, results cached in statics.
bool HasSSE42( );
bool HasAVX( );    // 256-bit float; ALSO requires OS XSAVE support (checked)
bool HasAVX2( );   // 256-bit integer; implies HasAVX
bool HasFMA( );

// Human-readable summary, e.g. "baseline(SSE2)" or "SSE4.2 AVX AVX2 FMA".
// Logged once at startup so a crash report tells us what the machine had.
const char* Summary( );

// Log the detected feature set. Call once during init, BEFORE any dispatch.
void LogFeatures( );

}  // namespace EnCpu

#endif  // ENATIONS_CPUFEATURES_H
