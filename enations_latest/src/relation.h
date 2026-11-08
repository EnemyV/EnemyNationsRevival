//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __RELATION_H__
#define __RELATION_H__


// relation.h : header file
//
// CDlgRelations removed (replaced by SDL2RelationsDialog).
// CDlgStats removed (CHEAT-only player-stats dialog).
// NewRelations is the free function that swaps a player's relation level
// and posts the network message; previously a static method on CDlgRelations.

class CPlayer;

void NewRelations( CPlayer* pPlyr, int iLevel );

#endif
