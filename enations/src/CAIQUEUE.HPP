////////////////////////////////////////////////////////////////////////////
//
//  CAIQueue.hpp : CAIQueue object declaration
//                 Divide and Conquer AI
//               
//  Last update:    09/07/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "netapi.h"

#ifndef __CAIQUEUE_HPP__
#define __CAIQUEUE_HPP__

class CAIQueue : public CObList
{
	DECLARE_SERIAL( CAIQueue );

	int m_iPlayer;	// this player
	POSITION m_Pos;

public:
	CAIQueue() {};
	CAIQueue( int iPlayer );
	~CAIQueue();

	void DeleteList( void );

	virtual void Serialize( CArchive& archive );
};

#endif // __CAIQUEUE_HPP__
