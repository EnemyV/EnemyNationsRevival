////////////////////////////////////////////////////////////////////////////
//
//  CAIMsg.hpp :  CAIMsg object
//                Divide and Conquer AI
//               
//  Last update:    10/28/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "netapi.h"
#include "netcmd.h"

#ifndef __CAIMSG_HPP__
#define __CAIMSG_HPP__

//
// provides a AI specific message that can support all CNetOrder
// derived messages from the game
//
class CAIMsg : public CObject
{
	DECLARE_SERIAL( CAIMsg );
	
protected:
public:

	int	m_iMsg;			// id of message
	int m_iPriority;	// priority of this message
	unsigned m_uFlags;	// used for status and flags
	DWORD m_dwID;		// id of CUnit if applicable
	int	m_iX;			// used for location
	int	m_iY;
	int	m_ieX;			// used for destination
	int	m_ieY;
	int m_idata1;		// used for m_iType, m_iStatus, 
						// m_iBldg, m_iVeh, m_iDamage
	int m_idata2;		// used for m_iNum, m_bOK
	int m_idata3;
	DWORD m_dwID2;		// id of 2nd CUnit if needed

	CAIMsg() {};
	~CAIMsg();
	CAIMsg( CNetCmd const *pMsg );
	CAIMsg( CAIMsg *SrcMsg );

	virtual void Serialize( CArchive& archive );
};

#endif // __CAIMSG_HPP__
