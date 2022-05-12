//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// rand.cpp - random numbers
//

#include "stdafx.h"
#include "_windwrd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


int RandNum (int iMax)
{

	ASSERT (iMax >= 0);
	if (iMax <= 0)
		return (0);

	// can be BIG
	if (iMax >= RAND_MAX)
		{
		if (iMax < INT_MAX/RAND_MAX)
			{
			int iRtn = (MyRand () * iMax) / RAND_MAX;
			ASSERT (iRtn <= iMax);
			return (iRtn);
			}
		int iRtn = MyRand () * (iMax / RAND_MAX);
		ASSERT (iRtn <= iMax);
		return (iRtn);
		}

	int iRtn = MyRand () / (RAND_MAX / (iMax + 1));
	if (iRtn > iMax)
		return (iMax);
	return (iRtn);
}


class xRand
{
public:
	xRand () { MySrand (0); }
};

static xRand needTheCtor;
static int aRnd [98];
static int iRtn = 0;

DWORD MySeed ()
{

	SYSTEMTIME _st;
	GetSystemTime (&_st);
	unsigned uRand = ((unsigned) _st.wDay << 28) | ((unsigned) (_st.wMinute & 31) << 23) | 
							((unsigned) (_st.wSecond & 31) << 18) | ((unsigned) (_st.wMonth & 3) << 16) | 
							(timeGetTime () & 0xFFFF);

	return (uRand);
}

void MySrand (DWORD dwSeed)
{

	srand (dwSeed);

	for (int iOn=0; iOn<98; iOn++)
		aRnd[iOn] = rand ();
	iRtn = rand ();
}

int MyRand ()
{

	int iInd = (iRtn * 97) / RAND_MAX;
	ASSERT ((0 <= iInd) && (iInd < 98));

	iRtn = aRnd [iInd];
	aRnd [iInd] = rand ();

	return (iRtn);
}
