////////////////////////////////////////////////////////////////////////////
//
//  CAIRoute.hpp : CAIRouter object declaration
//                 The Last Planet - AI
//               
//  Last update:  08/15/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIMsg.hpp"
#include "CAIMap.hpp"

#ifndef __CAIROUTE_HPP__
#define __CAIROUTE_HPP__

//
// this class provides the highest level of management for the AI
//
class CAIRouter : public CObject
{
	CAIMap *m_pMap;
	CAIUnitList *m_plUnits;
	CAIUnitList *m_plTrucksAvailable;
	CAIUnitList *m_plBldgsNeed;

	int m_iPlayer;		// id of the player
	DWORD m_dwRocket;	// ID of the rocket

public:

	BOOL m_bNeedGas;	// external access for goalmgr to update gas status

	CAIRouter( CAIMap *pMap, CAIUnitList *plUnits, int iPlayer );
	~CAIRouter();

    void DoRouting( CAIMsg *pMsg );
	
	void GetBuildingNeeding( void );
    void SetPriorities( void );

    void FillPriorities( void );
	BOOL NeedTransports( void );
    BOOL FindTransport( CAIUnit *pCAIBldg );
	void IdleTruckTask( int iMat, int iFromBldg, int iToBldg );

	void GetTrucksAvailable( void );
	CAIUnit *GetNearestTruck( CAIUnit *pCAIBldg );
	CAIUnit *GetNearestSource( int iMaterial, int iQtyNeeded, 
		int *piDistBack, int iX, int iY );

	BOOL IsValidUnit( CAIUnit *pUnit );
	BOOL IsValidUnit( DWORD dwID );

	BOOL TrucksAreEnroute( CAIUnit *pBldg );
	void UnassignTruck( DWORD dwTruckID );
	void UnassignTrucks( DWORD dwTruckID );
	void UnassignTrucks( CAIUnit *pBldg );

    BOOL NeedsCommodities( CAIUnit *pUnit );
	void SetNeeded( int iOffset, int iaNeeded[], int iOnHand );
	void SetUnitPriority( CAIUnit *pUnit );

	BOOL UnloadMaterials( CAIUnit *pTruck, CAIUnit *pBldg );
	void LoadMaterials( CAIUnit *pTruck, CAIHex *paiHex );
	BOOL BuildingNeedsAlways( CAIUnit *pBldg );

	void Save( CFile *pFile );
	void Load( CFile *pFile, CAIUnitList *plUnits );
};

#endif // __CAIROUTE_HPP__
