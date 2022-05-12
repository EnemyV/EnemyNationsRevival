#ifndef __RAND_H__
#define __RAND_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


DWORD MySeed ();
int MyRand ();
void MySrand (DWORD dwSeed);
int RandNum (int iMax);


#endif
