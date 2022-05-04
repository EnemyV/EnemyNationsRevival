////////////////////////////////////////////////////////////////////////////
//
//  CAIHex.hpp :  CAIHex object declaration
//                Divide and Conquer AI
//               
//  Last update:    09/11/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include <afxwin.h>

#ifndef __CAIHEX_HPP__
#define __CAIHEX_HPP__

//
// this class provides an AI specific hex to receive
// data copied from the CHex from the game
//
class CAIHex : public CObject
{
public:
	int m_iX;			// location
	int m_iY;
	BYTE m_cTerrain;	// low nibble - terrain type
	int m_iUnit;		// unit type of CBuilding or CVehicle
	DWORD m_dwUnitID;	// id of unit in this hex

	CAIHex();
	CAIHex( int iX, int iY );

#if 0
	CAIHex()
	{
		m_iX = 0;
		m_iY = 0;
		m_iUnit = 0xFFFF;
		m_dwUnitID = (DWORD)0;
		m_cTerrain = (BYTE)0xFF;
	}

	CAIHex( int iX, int iY ) 
	{
		m_iX = iX;
		m_iY = iY;
		m_iUnit = 0xFFFF;
		m_dwUnitID = (DWORD)0;
		m_cTerrain = (BYTE)0xFF;
	}
#endif
};

/*
	enum {			// terrain types
		city,		// CWndWorld m_clrTerrains is dependent on this
		desert,
		forest,
		hill,
		mountain,
		ocean,
		plain,
		river,
		road,
		rough,
		swamp,
		num_types };
*/

#endif // __CAIHEX_HPP__
