////////////////////////////////////////////////////////////////////////////
//
//  CAIStart.hpp :  CAIStart object declaration
//                  Divide and Conquer AI
//               
//  Last update:  09/04/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIData.hpp"

#ifndef __CAISTART_HPP__
#define __CAISTART_HPP__

class CAIStart
{
public:
	CAIHex *m_pgrpHexs;	// pointer to work area for group of hexes
	int m_iGrpSize;		// count of hexes in group

	WORD *m_pwaSeen;	// pointer to hexes that can be seen
	int m_iMapSize;		// count of hexes on map

	CAIHex *m_pmnHexs;	// array of mineral hexes

	int m_iBlockX;	// default start hex, middle of block
	int m_iBlockY;
	int m_iBaseX;	// start hex of evaluation block
	int m_iBaseY;
	int m_iBlockSize; // width/height of block to check

	CAIStart( int iBlockX, int iBlockY );

	void PickStartHex( void );
	void RateMinerals( CHexCoord& hex );
	int RateMineralDist( CHexCoord& hexFrom );

	int RateForests( CHexCoord& hexBase );
	int GetBuildMultiplier( int iOffset );
	int GetGroupMult(void);
	int CountAround( CAIHex *paiHex );
	BOOL IsGroupBuildable( CAIHex *paiHex, int iPos );
	BOOL IsHexValidToBuild( CAIHex *paiHex );
	BOOL IsHexInStartArea( CHexCoord& hex );
	void GetGroupXY( CAIHex *paiHex, int iPos, int *piX, int *piY );
	int GetMapOffset( int iX, int iY );
	void OffsetToXY( int iOffset, int *piX, int *piY );
	void LoadGroupHexes( int isx, int isy );
	void ClearGroupHexes(void);

	~CAIStart();
};

#endif // __CAISTART_HPP__
