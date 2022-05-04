////////////////////////////////////////////////////////////////////////////
//
//  CAIInit.hpp :  CAIInitPos object implementation
//                 Divide and Conquer AI
//               
//  Last update:  07/03/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIMap.hpp"
#include "CAIUnit.hpp"

#ifndef __CAIINIT_HPP__
#define __CAIINIT_HPP__

class CAIInitPos
{
	int m_iPlayer;			// AI Player
	CAIUnitList *m_plUnits;	// list of all CAUnits for this manager
	//CAIMapList *m_plMaps;	// list of all CAIMaps for this manager
	CAIMap *m_pMap;	// CAIMap for this manager
	CHexCoord m_hexRocket;	// where the rocket lands
	CHexCoord m_hexStart;	// start and end hexes of the block
	CHexCoord m_hexEnd;

public:
	CAIInitPos( int iPlayer, CAIMap *pMap, CAIUnitList *plUnits );
		//CAIMapList *plMaps, CAIUnitList *plUnits );

	void DoIt( void );

	//void PlaceBuilding( CAIMap *pMap, CAIUnit *pUnit, CHexCoord& hex, int iBldg);
	void PlaceBuilding( CAIUnit *pUnit, CHexCoord& hex, int iBldg);
	//void PlaceVehicle( CAIMap *pMap, CAIUnit *pUnit, int iVeh );
	void PlaceVehicle( CAIUnit *pUnit, int iVeh );
	//void PlaceMaterial( int iMaterial, int iBldg, int iQty );
	//void PlaceRocket( CAIMap *pMap );
	void PlaceRocket( void );

	//void GetVehInitPosHex( CAIMap *pMap, int iVeh, CHexCoord& hexPlace );
	void GetVehInitPosHex( int iVeh, CHexCoord& hexPlace );
};

#endif // __CAIINIT_HPP__

