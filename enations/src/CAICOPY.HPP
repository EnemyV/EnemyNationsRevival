////////////////////////////////////////////////////////////////////////////
//
//  CAICopy.hpp :  CAICopy object declaration
//                 Divide and Conquer AI
//               
//  Last update:    11/27/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include <afxwin.h>
#include "cai.h"

#ifndef __CAICOPY_HPP__
#define __CAICOPY_HPP__

// provide support to store appropriate data to reflect:
//
// CUnitData
// CBuildMaterials
// CBuildMine
// CBuildFarm
// CBuildUnit
// CStructureData
// CTransportData
// CBuilding
// CVehicle
//

class CAICopy : public CObject
{
public:

	enum {
		CUnitData,
		CBuildMaterials,
		CBuildMine,
		CBuildFarm,
		CBuildUnit,
		CStructureData,
		CTransportData,
		CBuilding,
		CVehicle };

	CAICopy( int iType );

	int m_iType;	// value from CAICopy::enum to indicate
					// the type data being stored in this object

	int m_aiDataIn[CAI_DATA_SLOTS];
	int m_aiDataOut[CAI_DATA_SLOTS];
};

class CAICopyList : public CObList
{
public:

	CAICopyList() {};
	~CAICopyList();

	CAICopy *GetCopy( int iType );
	void RemoveCopy( int iType );
	void DeleteList( void );
};

#endif // __CAICOPY_HPP__
