////////////////////////////////////////////////////////////////////////////
//
//  CAIHex.cpp :  CAIHex object implementation
//                Divide and Conquer AI
//               
//  Last update:    10/02/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIHex.hpp"

#define new DEBUG_NEW

CAIHex::CAIHex()
{
	m_iX = 0;
	m_iY = 0;
	m_iUnit = 0xFFFF;
	m_dwUnitID = (DWORD)0;
	m_cTerrain = (BYTE)0xFF;
}

CAIHex::CAIHex( int iX, int iY ) 
{
	m_iX = iX;
	m_iY = iY;
	m_iUnit = 0xFFFF;
	m_dwUnitID = (DWORD)0;
	m_cTerrain = (BYTE)0xFF;
}

// end of CAIHex.cpp
