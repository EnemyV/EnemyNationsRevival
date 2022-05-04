////////////////////////////////////////////////////////////////////////////
//
//  CAIOpFor.hpp : CAIOpFor object declaration
//                 Divide and Conquer AI
//               
//  Last update:    05/10/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include <afxwin.h>

#include "CAIMsg.hpp"

#ifndef __CAIOPFOR_HPP__
#define __CAIOPFOR_HPP__

// attitude settings for CAIOpFor
#define ALLIANCE	(BYTE)200	// >= 200
#define PEACE		(BYTE)160	// >= 160 && < 200
#define NEUTRAL		(BYTE)100	// < 160 && > 70
#define HOSTILE		(BYTE)70	// <= 70 && > 50
#define WAR			(BYTE)50	// <= 50

// for every THREATHOLD number of message received, w/o attack
// increase attitude towards peace
#define THREATHOLD	100

//
// contains data relative to opposing player forces which are encountered
// by the AI during play
//
class CAIOpFor : public CObject
{
protected:

	int m_iPlayer;			// game generated id (called owner sometimes)
	CString m_sName;		// name of the OpFor
	DWORD m_dwRocket;		// id of the rocket building

							// the OpFor units we know about
	WORD *m_pwaUnits;		// array of counts of known OpFor units
	int m_iNumUnits;		// in vehicle type order by offset
	WORD *m_pwaBldgs;		// array of counts of known OpFor buildings
	int m_iNumBldgs;		// in building type order by offset

	WORD *m_pwaRAs;		// array of racial attributes known
	int m_iNumRAs;		// about this OPFOR in offset order with
						// 0 == Unknown characteristic

								// history of OpFor attacks on us
	WORD *m_pwaAttackedUnits;	// counts of attacks by vehicle
								// uses m_iNumUnits
	WORD *m_pwaAttackedBldgs;	// counts of attacks by building
								// uses m_iNumBldgs

	int m_iMsgCount;	// counter used to track number of msgs w/o attack
	int m_iAttitude;
	BYTE m_cRelations;	// 200+ alliance 160-199 peace 100-159 neutral
						// 70-99 hostile 0-69 war
	
	BOOL m_bAtWar;	// TRUE indicates all out war underway
	BOOL m_bIsAI;	// TRUE indicates that this CAIOpFor is controlled by my AI
	BOOL m_bKnown;  // TRUE indicates the opfor is known to the AI

public:

	CAIOpFor() {};
	~CAIOpFor();
	CAIOpFor( int iPlayerID, const char *pzName );

	int GetPlayerID( void );
	
	CString& GetName( void );

	int GetStrengths( void );
	int GetIndustrial( void );
	int GetWarfare( void );
	int GetCombat( void );
	DWORD GetRocket( void );

	void UpdateCounts( void );

	BYTE GetRelations( void );
	void SetRelations( void );
	void SetRelations( BYTE );
	void SetRocket( DWORD dwRocket );

	BOOL AtWar( void );
	void SetWar( BOOL bWar );
	BOOL IsAI( void );
	void SetIsAI( BOOL bAI );
	BOOL IsKnown( void );
	void SetKnown( BOOL bKnown );
	int GetMsgCount( void );
	void SetMsgCount( int );
	int GetAttitude( void );
	void SetAttitude( int );

	void AdjustThreat( void );
	void UpdateCounts( CAIMsg *pMsg );
	WORD GetVehicle( WORD wOffset );
	void SetVehicle( WORD wOffset, WORD wCnt );
	WORD GetBuilding( WORD wOffset );
	void SetBuilding( WORD wOffset, WORD wCnt );
	WORD GetAttribute( WORD wOffset );
	void SetAttribute( WORD wOffset, WORD wValue );
	WORD GetTotalAttacks( void );
	void AddUnitAttack( int iVeh );
	WORD GetUnitAttack( int iVeh );
	void AddBldgAttack( int iBldg );
	WORD GetBldgAttack( int iBldg );
	
	void Load( CFile *pFile );
	void Save( CFile *pFile );
};

class CAIOpForList : public CObList
{
public:

	CAIOpForList() {};
	~CAIOpForList();
	
	CAIOpFor *GetOpFor( int iPlayer );
	CAIOpFor *AddOpFor( int iPlayer );
	
	void RemoveOpFor( int iPlayer );
	void DeleteList( void );

	WORD GetVehicleCnt( int iVeh, BOOL bAve=FALSE );
	WORD GetBuildingCnt( int iBldg );
	WORD GetAttackCnt( int iType, int iTypeUnit );

	CAIOpFor *GetNearest( CHexCoord& hexFrom, BOOL bKnown );
	CAIOpFor *GetWeakest( BOOL bKnown );
	BOOL AllKnown( void );

	void Save( CFile *pFile );
	void Load( CFile *pFile );
};

#endif // __CAIOPFOR_HPP__
