////////////////////////////////////////////////////////////////////////////
//
//  CHPRoute.hpp : CHPRoute object declaration
//                 The Last Planet - Human player materials distribution
//               
//  Last update:  10/31/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIMsg.hpp"
#include "CAIData.hpp"

//#include "CAIUnit.hpp"
//#include "CAIHex.hpp"

#ifndef __CHPROUTE_HPP__
#define __CHPROUTE_HPP__

//
// this class defines the human player's vehicle routing for
// unattended material distribution
//
#ifdef _DEBUG
class CHPRouter : public CObject
#else
class CHPRouter
#endif
{
	CAIUnitList *m_plUnits;
	CAIUnitList *m_plTrucksAvailable;
	CAIUnitList *m_plBldgsNeed;
	CAIUnitList *m_plShipsAvailable;

	int m_iPlayer;
	int m_iStore[CMaterialTypes::num_types];
	int m_iOutput[CMaterialTypes::num_types];
	int m_iNeeds[CMaterialTypes::num_types];
	int m_aiMatsNeeded[CMaterialTypes::num_types];

	WORD m_wSeaportCnt; // count of seaports

	WORD m_wLumberCnt;	// count number of special buildings that have
	WORD m_wCoalCnt;		// been completed construction which helps the
	WORD m_wIronCnt;	// special building status determination process
	WORD m_wSmelterCnt;

	BOOL m_bLastLumber; // flags indicate that the last of a special
	BOOL m_bLastCoal;	// type of building has been detected as being
	BOOL m_bLastIron;	// deleted, which enables reserve materials
	BOOL m_bLastSmelter;

	DWORD m_dwRocket;

public:
    
    // construct an instance of the object and then call Init()
    //
	CHPRouter( int iPlayer );
	// a FALSE indicates failed to initialize
	BOOL Init( void );
	
	// must call destructor on game end
	~CHPRouter();

	void		MsgNewBldg (CBuilding const * pBldg);
	void		MsgGiveBldg (CBuilding const * pBldg);
	void		MsgBuiltBldg (CBuilding const * pBldg);
	void		MsgOutMat (CBuilding const * pBldg);
	void		MsgDeleteUnit (CUnit const * pUnit);
	void		MsgTakeBldg (CBuilding const * pBldg);
	void		MsgTakeVeh (CVehicle const * pVeh);
	void		MsgVehNew (CVehicle const * pVeh, DWORD dwIDBldg);
	void		MsgGiveVeh (CVehicle const * pVeh);
	void		MsgArrived (CVehicle const * pVeh);
    void        MsgErrGoto (CVehicle const * pVeh);

protected:
	// the game's message interface to the router
	//
	// CNetCmd::bldg_new - for newly placed buildings
	// CNetCmd::bldg_stat - for just completed and out_mat conditions
	// CNetCmd::delete_unit - for KIA buildings or vehicles and
	//                        when HP takes control of vehicle
	// CNetCmd::veh_new - for newly produced vehicles (trucks/ships only) and
	//                    when HP returns control of a vehicle
	// CNetCmd::veh_dest - for when trucks arrive at source/assigned buildings
	//
	void RouteMessage( CNetCmd const *pMsg );
	
	// general 'do it all' manager cloned from AI's version
    void DoRouting( CAIMsg *pMsg );
	
    // utilities to replace CAIData, CAIMap and CAIMgr usage
	void VehicleErrorResponse( CAIMsg *pMsg );
	void DestinationResponse( CAIMsg *pMsg );
    void UpdateUnits( CAIMsg *pMsg );
	void GetCHexData( CAIHex *pHex );
	
	int GetMaterialCapacity( CAIUnit *paiUnit );
	void GetShipsAvailable( void );
	void GetTrucksAvailable( void );
	void GetVehicleCargo( DWORD dwID );
	BOOL HandleLoadedTruck( CAIUnit *pTruck );
	BOOL GetShipHex( DWORD dwID, CHexCoord& hex );
	BOOL GetVehicleHex( DWORD dwID, CHexCoord& hex );
	BOOL GetBldgExit( DWORD dwID, CHexCoord& hex );
	BOOL GetStagingHex( CAIUnit *pUnitToStage, 
		CAIUnit *pUnitNearby, CHexCoord& hexDest );
	
	// cloned members from CAIRouter
	void GetBuildingNeeding( void );
    void SetPriorities( void );

    void FillPriorities( void );
	void SecondaryStocking( int iMat, int iFromBldg, int iToBldg );
	int GetVehicleCount( CBuilding *pBldg );
	BOOL NeedTransports( void );
    BOOL FindTransport( CAIUnit *pCAIBldg );

	// run paths out to determine if water is crossed
	BOOL NeedsTransport( CAIUnit *pTruck, CHexCoord& hex );
	// this version gets a truck to a pick up point
	void ConsiderLandWater( CAIUnit *pTruck, CHexCoord& hex );
	// this version is for truck arriving at a building
	BOOL ConsiderLandWater( CAIUnit *pTruck, CAIHex *pHex );
	
	BOOL IsValidUnit( CAIUnit *pUnit );
	CAIUnit *GetNearestShip( CAIUnit *pCAITruck );
	CAIUnit *GetNearestTruck( CAIUnit *pCAIBldg );
	CAIUnit *GetNearestSource( CAIUnit *pTruck, int iMaterial, 
		CAIUnit *pBldgNeeding, int *piDistBack, int iX, int iY );

	BOOL TrucksAreEnroute( CAIUnit *pBldg );
	void UnAssignShip( CAIUnit *pShip );
	void UnassignTrucks( DWORD dwTruckID );
	void UnassignTrucks( CAIUnit *pBldg );

    BOOL NeedsCommodities( CAIUnit *pUnit );
	BOOL _NeedsCommodities( CAIUnit *pUnit );
	BOOL IsShip( DWORD dwID );
	BOOL IsTruck( DWORD dwID );
	BOOL IsVehicleCargo( DWORD dwID );
	void CheckWarehouses( void );
	BOOL HaveExcessMaterials( void );
	void SetExcessMaterials( CAIUnit *pWarehouseBldg );
	void SetNeeded( int iOffset, int iaNeeded[], int iOnHand );
	void SetUnitPriority( CAIUnit *pUnit );

	BOOL UnloadMaterials( CAIUnit *pTruck, CAIUnit *pBldg );
	void LoadMaterials( CAIUnit *pTruck, CAIHex *paiHex );
	BOOL BuildingNeedsAlways( CAIUnit *pBldg );

	BOOL GetNearRoad( CHexCoord& hexRoad );
	BOOL IsRoadNearBuilding( CHexCoord& hexRoad );
	int CountSpecialUnits( int iTypeUnit );

	void LoadUnit( DWORD dwCarrier, DWORD dwCargo );
	void UnloadCargo( DWORD dwCarrier );
	void SetDestination( DWORD dwID, CHexCoord& hexDest );

public:
	// must save and load separate from the AI
	// be sure the CFile::Flush() has been called before passing
	void Save( CFile *pFile );
	void Load( CFile *pFile );
};

#endif // __CHPROUTE_HPP__
