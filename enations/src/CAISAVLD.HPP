////////////////////////////////////////////////////////////////////////////
//
//  CAISavLd.hpp : Declares class for the Save/Load class.
//
//  Last update:    09/28/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIGoal.hpp"
#include "CAITask.hpp"

#ifndef __CAISAVLD_HPP__
#define __CAISAVLD_HPP__

////////////////////////////////////////////////////////////////////////////
//
// The CAISavLd class provides the functions for save/loading the map
//
////////////////////////////////////////////////////////////////////////////

class CAISavLd : public CObject
{
    DECLARE_DYNAMIC( CAISavLd );

protected:
    CWnd *m_pParent;        // parent window pointer
public:
    CAISavLd( CWnd *pWnd );
    void Save( void );
    void DumpData( CFile *pFile, UINT uWhich );

	void SaveBinary( void );
	void DumpBinaryData( CFile *pFile );

    void Load( void );
    void LoadData( CFile *pFile, UINT uWhich );

	void LoadBinaryData( CFile *pFile );
	void LoadBinary( void );

    CWordArray *LoadRCIP( int iSmart );
	CWordArray *LoadIG( int iSmart );
	CWordArray *LoadArrays( int iSmart, CString& sHead );
    CWordArray *LoadData( CFile *pFile );
    void SaveRCIP( int iSmart, CWordArray *pwaData );
	void SaveIG( int iSmart, CWordArray *pwaData );
	void SaveArrays( int iSmart, CWordArray *pwaData, CString& sHead );
    void DumpData( CFile *pFile, CWordArray *pwaData );
    
    CFile* OpenForLoad( const CString& rFileName );
    CFile* OpenForDump( const CString& rFileName );
};

/*
// these structures support the binary i/o

//  CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks );
typedef struct goal_buffer {
	int iID;
	int iType;
	int iTasks[NUM_INITIAL_GOALS];
} GoalBuff;

//	CAITask( WORD wID, BYTE cType, BYTE cPriority, 
//		WORD wOrderID, CWordArray *pwaParams );

typedef struct task_buffer {
	int iID;
	int iType;
	int iPriority;
	int iOrderID;
	int iParams[MAX_TASKPARAMS];
} TaskBuff;
*/

#endif // __CAISAVLD_HPP__
