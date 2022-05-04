////////////////////////////////////////////////////////////////////////////
//
//  CAIUnit.hpp :  CAIUnit object declaration
//                 Divide and Conquer AI
//               
//  Last update:    06/09/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "terrain.inl"
#include "netapi.h"
#include "player.h"

#include "CAICopy.hpp"

#ifndef __CAIUNIT_HPP__
#define __CAIUNIT_HPP__

//
// this class will contain an AI specific data for 
// instances of D&C vehicles and buildings
//
class CAIUnit : public CObject
{
protected:
	CHexCoord	m_hexLastDest;		// last place/time for SetDest (so don't repeat)
	DWORD			m_timeLastDest;

	DWORD m_dwID;
	int m_iOwner;
	int m_iType;		// value of CUnit::building
						// or CUnit::vehicle
	int m_iTypeUnit;	// CStructureData or CTransportData id

	BOOL m_bControl;	// indicates if unit is controled
						// by the AI, FALSE=HP unit
	WORD m_wGoal;		// current goal of this unit
	WORD m_wTask;		// current task of this unit
	DWORD m_dwData;		// possible id of building/opfor attacking

	WORD m_wStatus;		// a status flag to use generally

	WORD *m_pwaParams;	// an array of commodities needed
						// if this unit is a building
						// or carried if unit is a vehicle

	DWORD *m_pdwaParams;	// a DWORD version of array

	CAICopyList *m_plCopyData;  // list of CAICopy objects that
								// provide CVehicle/CBuilding support
//
// maintain a flag to indicate what the data is in each
// copy data data object
//
// store the appropriate data to reflect:
//
// CUnitData
// CBuildMaterials
// CBuildMine
// CBuildFarm
// CBuildVehicle
// CStructureData
// CTransportData
// CBuilding
// CVehicle
//



public:
	CAIUnit( DWORD dwID, int iOwner, int iType, int iTypeUnit );
	~CAIUnit();
	CAIUnit() {};

	DWORD GetID( void );
	void SetID( DWORD );
	int GetOwner( void );
	void SetOwner( int );
	int GetType( void );
	void SetType( int iType );
	int GetTypeUnit( void );
	BOOL IsControl( void );
	void SetControl( BOOL );
	WORD GetTask( void );
	void SetTask( WORD wTask );
	WORD GetGoal( void );
	void SetGoal( WORD wGoal );
	DWORD GetDataDW( void );
	void SetDataDW( DWORD );
	WORD GetStatus( void );
	void SetStatus( WORD );

	WORD GetParam( WORD wOffset );
	void SetParam( WORD wOffset, WORD wValue );
	DWORD GetParamDW( WORD wOffset );
	void SetParamDW( WORD wOffset, DWORD dwValue );
	void ClearParamDW( void );
	void ClearParam( void );

	void Update( CUnitData const *pData );
	void Update( CBuildMaterials const *pData );
	void Update( CBuildMine const *pData );
	void Update( CBuildFarm const *pData );
	void Update( CBuildUnit const *pData );

	void Update( CStructureData const *pData );
	void Update( CTransportData const *pData );
	void Update( CBuilding *pData );
	void Update( CVehicle *pData );

	// critical section used
	CAICopy *GetCopyData( int iType );
	void RepairVehicle( void );
	void RepairBuilding( void );
	void SetDestination( CAIUnit *pCAIBldg );
	void SetDestination( CSubHex& subHexDest );
	void SetDestination( CHexCoord& m_hex );
	void UnloadCargo( void );
	void LoadUnit( DWORD dwCargo );
	void AttackUnit( DWORD dwTarget );
	int AttackCount( void );
	BOOL IsAttacked( void );
	void AttackedBy( DWORD dwAttacker );
	void MoveMaterial( int iMaterial, int iQty, 
		DWORD dwFromID, DWORD dwToID );
};

class CAIUnitList : public CObList
{
public:
	CAIUnitList() {};
	~CAIUnitList();

	CAIUnit *GetUnitNY( DWORD dwId );
	CAIUnit *GetUnit( DWORD dwID );
	void ClearTarget( DWORD dwTarget );
	CAIUnit *GetClosestRepair( int iPlayer, int iBldg, CHexCoord& hex );
	int GetRepairCount( int iPlayer, DWORD dwRepBldg );
	CAIUnit *GetClosest( int iPlayer, int iBldg, CHexCoord& hex );
	void AddUnit( DWORD dwID );
	void RemoveUnits( int iPlayer );
	void RemoveUnit( DWORD dwID, BOOL bObjectToo = TRUE );
	void DeleteList( void );

	// critical section used
	CAIUnit *GetOpForUnit( DWORD dwId );
	void Load( CFile *pFile );
	void Save( CFile *pFile );
};

#endif // __CAIUNIT_HPP__
