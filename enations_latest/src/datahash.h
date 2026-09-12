#ifndef DATAHASH_H
#define DATAHASH_H

//---------------------------------------------------------------------------
//
//  The gameplay data hash (015 area 3, phase 3).
//
//  One number over the files that decide simulation results - unit and
//  research tables, the version record, the AI's stdgta.dat, the race list and
//  the gameplay text lists. Sprites, bitmaps, music, SFX, fonts, cursors and
//  videos are NOT in it, so a player can replace art and sound and still join.
//
//  The bytes come from CDataFile, whatever it resolves them to (a loose file
//  or the container), so the number describes what this install will actually
//  load rather than what happens to sit on disk.
//
//---------------------------------------------------------------------------

#include "stdafx.h"

//  Computes the hash. Reads the data set, so call it once, after the data file
//  is initialised. Never throws: a resource it cannot read is folded in as a
//  miss, which keeps the value deterministic and different from a good set.
DWORD EnComputeGameplayDataHash( );

#endif  // DATAHASH_H
