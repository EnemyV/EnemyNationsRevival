#include "stdafx.h"
#include "event.h"
#include "lastplnt.h"
#include "area.h"
#include "player.h"
#include "sprite.h"
#include "bridge.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"

CBridgeMap theBridgeMap;
CBridgeHex theBridgeHex;



/////////////////////////////////////////////////////////////////////////////
// CBridge - a complete bridge (end to end)

CBridge::CBridge (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt)
{


	m_hexStart = hexStart;
	m_hexEnd = hexEnd;
	m_dwID = dwID;
	m_iAlt = iAlt;
	m_iConstTotal = CTerrainData::GetBuildRoadTime () * 40 * 4;
	m_iPerBuilt = 0;

	// direction of bridge
	m_iDir = (hexStart.X () != hexEnd.X ()) ? 1 : 0;

	// middle of bridge
	m_mlMiddle.x = hexStart.X () * MAX_HEX_HT + 
										( CHexCoord::Diff (hexEnd.X () - hexStart.X ()) * MAX_HEX_HT ) / 2;
	m_mlMiddle.y = hexStart.Y () * MAX_HEX_HT + 
										( CHexCoord::Diff (hexEnd.Y () - hexStart.Y ()) * MAX_HEX_HT ) / 2;
	m_mlMiddle.Wrap ();
}

CBridge * CBridge::Create (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt)
{
#ifdef _LOGOUT
	logPrintf (LOG_PRI_CRITICAL, LOG_VEH_MOVE, 
		"Vehicle %ld bridge building from %d,%d to %d,%d ", 
			dwID, hexStart.X(), hexStart.Y(), hexEnd.X(), hexEnd.Y() );
#endif

	// make an instance of a building
	CBridge * pBrdg = new CBridge (hexStart, hexEnd, dwID, iAlt);

	// put up the pieces
	CHexCoord _hexOn (hexStart);
	int xAdd = CHexCoord::Diff (hexEnd.X () - _hexOn.X ());
	int yAdd = CHexCoord::Diff (hexEnd.Y () - _hexOn.Y ());

	int iSpan = abs (xAdd) + abs (yAdd) + 1;
	pBrdg->m_iConstTotal = CTerrainData::GetBuildRoadTime () * 40 * iSpan;

	xAdd = __minmax (-1, 1, xAdd);
	yAdd = __minmax (-1, 1, yAdd);

	hexStart.Flatten( iAlt );
	hexEnd  .Flatten( iAlt );

	int iLen = 0;
	while (iLen <= MAX_SPAN+1)
		{
		CBridgeUnit * pBu = new CBridgeUnit (_hexOn, pBrdg);
		pBu->m_hex.SetInvalidated ();
		pBrdg->m_lstUnits.AddTail (pBu);

		// handle start
		if (_hexOn == hexStart)
			{
			pBu->m_iExit = (xAdd != 0) ? (xAdd + 2) : ((yAdd + 3) & 0x03);

			/* GG
			theMap.GetHex (hexStart.X () + (xAdd == -1 ? 1 : 0),
																hexStart.Y () + (yAdd == 1 ? -1 : 0))->SetAlt (iAlt);
			theMap.GetHex (hexStart.X () + (xAdd == 1 ? 0 : 1),
																hexStart.Y () + (yAdd == -1 ? 0 : -1))->SetAlt (iAlt);
			*/
			}
		else
			// end
			if (_hexOn == hexEnd)
				{
				pBu->m_iExit = (xAdd != 0) ? (xAdd & 0x03) : yAdd + 1;

				/* GG
				theMap.GetHex (hexEnd.X () + (xAdd == 1 ? 1 : 0),
																hexEnd.Y () + (yAdd == -1 ? -1 : 0))->SetAlt (iAlt);
				theMap.GetHex (hexEnd.X () + (xAdd == -1 ? 0 : 1),
																hexEnd.Y () + (yAdd == 1 ? 0 : -1))->SetAlt (iAlt);
				*/
		      pBu->AssignSprite();
				break;
				}

#ifdef _LOGOUT
		logPrintf (LOG_PRI_CRITICAL, LOG_VEH_MOVE, 
			"bridge built at %d,%d ", _hexOn.X(), _hexOn.Y() );
#endif

		_hexOn.X () += xAdd;
		_hexOn.Y () += yAdd;
		_hexOn.Wrap ();

      pBu->AssignSprite();
		}

	theBridgeMap.Add (pBrdg);

	// invalidate ALL windows
	// GG: Shouldn't need this now: CWndAnim::InvalidateAllWindows ();

	// if visible start the sound
	CWndArea * pAreaWnd = theAreaList.GetTop ();
	if (pAreaWnd != NULL)
		{
		CRect rect;
		pAreaWnd->GetClientRect (&rect);
		CPoint ptBldg = pAreaWnd->GetAA().WrapWorldToWindow ( CMapLoc3D ( pBrdg->m_mlMiddle ));
		if (rect.PtInRect (ptBldg))
			pAreaWnd->InvalidateSound ();
		}

	ASSERT_VALID (pBrdg);

	return (pBrdg);
}

CBridge::~CBridge ()
{

	while (m_lstUnits.GetCount () > 0)
		{
		CBridgeUnit * pBu = m_lstUnits.RemoveHead ();
		delete pBu;
		}
}

void CBridge::__SetPer (int iPer)
{

	m_iPerBuilt = iPer;

	POSITION pos = m_lstUnits.GetHeadPosition ();
	while (pos != NULL)
		{
		CBridgeUnit * pBu = m_lstUnits.GetNext (pos);
		pBu->m_iPerBuilt = iPer;
		pBu->m_hex.SetInvalidated ();
		}
}

//---------------------------------------------------------------------------
// CBridge::GetSurfaceAlt
//---------------------------------------------------------------------------
Fixed
CBridgeUnit::GetSurfaceAlt(
	CMapLoc const & maploc ) const
{
	Fixed	fix;

	fix.Value(( GetAlt() << 16 ) + 0x00060000 );	// GGTODO: Figure out height on sloped endpoints

	return fix;
}

#ifdef _DEBUG

void CBridge::AssertValid () const
{
	// assert base object
	CObject::AssertValid();

	ASSERT ((m_iDir == 0) || (m_iDir == 1));
	ASSERT ((0 <= m_iAlt) && (m_iAlt <= 127));
	ASSERT ((0 <= m_iPerBuilt) && (m_iPerBuilt <= 100));
}

#endif


/////////////////////////////////////////////////////////////////////////////
// CBridgeUnit - one hex of a bridge

CBridgeUnit::CBridgeUnit (CHexCoord const & hex, CBridge * pPar)
{

	m_hex = hex;
	m_iDir = pPar->GetDir ();
	m_iExit = -1;					// set to middle
	m_pParBridge = pPar;
	m_iAlt = pPar->GetAlt ();
	m_iPerBuilt = 0;

	theBridgeHex.GrabHex (this);
}

//---------------------------------------------------------------------------
// CBridgeUnit::AssignSprite
//---------------------------------------------------------------------------
void
CBridgeUnit::AssignSprite()
{
   int   iID = -1;

   switch ( m_iExit )
   {
      case -1: switch ( m_iDir )
               {
						case 0:  iID = CStructureData::bridge_0; break;
                  case 1:  iID = CStructureData::bridge_1; break;
               }

               break;

      case 0:  iID = CStructureData::bridge_end_2; break;
      case 1:  iID = CStructureData::bridge_end_3; break;
      case 2:  iID = CStructureData::bridge_end_0; break;
      case 3:  iID = CStructureData::bridge_end_1; break;
   }

  	m_psprite = theStructures.GetSprite( iID, 0 );
}

//---------------------------------------------------------------------------
// CBridgeUnit::IsTwoPiece
//---------------------------------------------------------------------------
BOOL
CBridgeUnit::IsTwoPiece() const
{
	CStructureSprite::STAGE_TYPE	eStage;

	if ( IsBuilt() )
		eStage = CStructureSprite::COMPLETED_STAGE;
	else 
		eStage = CStructureSprite::CONSTRUCTION_STAGE;

	int	iDamage = 0;

	return GetSprite()->IsTwoPiece( GetDir(), eStage, iDamage );
}

//---------------------------------------------------------------------------
// CBridgeUnit::GetDrawCorner - Return window client coords of LL corner of sprite
//---------------------------------------------------------------------------
CPoint
CBridgeUnit::GetDrawCorner(
	CHexCoord const & hexcoord ) const
{
	int			iX;
	int			iY;

	switch ( xiDir )
	{
		case 0:	iX = hexcoord.X();
					iY = hexcoord.Y();
					break;

		case 1:	iX = hexcoord.X() + 1;
					iY = hexcoord.Y();
					break;

		case 2:	iX = hexcoord.X() + 1;
					iY = hexcoord.Y() + 1;
					break;

		case 3:	iX = hexcoord.X();
					iY = hexcoord.Y() + 1;
					break;
	}

	CHexCoord	hexcoordCorner = CHexCoord( iX, iY );
	CMapLoc		maploc  ( hexcoordCorner );
	CMapLoc3D	maploc3d( maploc, GetAlt() );

	CPoint	pt = xpanimatr->WorldToWindow( maploc3d );

	pt.y += MAX_HEX_HT >> xiZoom + 1;

	const int BRIDGE_Y_CENTER_FUDGE  = 21;
	const int BRIDGE_Y_TOP_END_FUDGE = -4;

	if ( -1 == m_iExit )
		pt.y += BRIDGE_Y_CENTER_FUDGE  >> xiZoom;	// artwork misaligned
	else if ( m_iExit == xiDir || m_iExit == (( xiDir + 1 ) & 0x03 ))
		pt.y += BRIDGE_Y_TOP_END_FUDGE >> xiZoom;	// artwork misaligned

	return pt;
}

//---------------------------------------------------------------------------
// CBridgeUnit::Draw
//---------------------------------------------------------------------------
CRect
CBridgeUnit::Draw(
	const CHexCoord & hexcoord )
{
	ASSERT_STRICT_VALID (this);

	CDrawParms		drawparms( *this, CPoint( 0, 0 ));
	CPoint			ptCorner = GetDrawCorner( hexcoord );
	CPoint		 	ptOffset = ptCorner;
	int			 	iDir 		= xiDir;
	int				iDamage  = 0;
	CMapLoc		 	maploc( hexcoord );
	CSpriteView	 * pspriteviewBridge;
	CRect			 	rect;

	// if built then its a normal draw

	if ( IsBuilt() )
	{
		pspriteviewBridge =
			GetSprite()->
				GetView(	iDir,
							GetLayer(),
						  	CStructureSprite::COMPLETED_STAGE,
						  	iDamage );

		ptOffset.y = ptCorner.y - pspriteviewBridge->Height();

		drawparms.m_ptOffset = ptOffset;

		rect = pspriteviewBridge->Draw( drawparms );

		return rect;
	}

	// Draw partial construction

	pspriteviewBridge =
		GetSprite()->GetView( iDir,
									 GetLayer(),
 					  			    CStructureSprite::CONSTRUCTION_STAGE,
									 iDamage );

	ptOffset.y = ptCorner.y - pspriteviewBridge->Height();
	rect 		  = pspriteviewBridge->Rect() - pspriteviewBridge->Rect().TopLeft() + ptOffset;
	rect.top  += (( 100 - GetPer() ) * rect.Height() ) / 100;

	drawparms.m_ptOffset = ptOffset;

	rect = pspriteviewBridge->DrawClip( drawparms, &rect );

	xpanimatr->GetDirtyRects()->AddRect( &rect );

	return rect;
}

//---------------------------------------------------------------------------
// CBridgeUnit::IsHit
//---------------------------------------------------------------------------
BOOL
CBridgeUnit::IsHit(
	CHexCoord hexcoord,
	CPoint	 ptCursor ) const
{
	ASSERT_STRICT_VALID (this);

	CDrawParms		drawparms( *( CBridgeUnit * )this, CPoint( 0, 0 ));
	CPoint			ptCorner = GetDrawCorner( hexcoord );
	CPoint		 	ptOffset = ptCorner;
	int			 	iDir 		= xiDir;
	int				iDamage  = 0;
	CMapLoc		 	maploc( hexcoord );
	CSpriteView	 * pspriteviewBridge;
	CRect			 	rect;

	// if built then its a normal draw

	if ( IsBuilt() )
	{
		pspriteviewBridge =
			GetSprite()->
				GetView(	iDir,
							CStructureSprite::FOREGROUND_LAYER,
						  	CStructureSprite::COMPLETED_STAGE,
						  	iDamage );

		ptOffset.y = ptCorner.y - pspriteviewBridge->Height();

		drawparms.m_ptOffset = ptOffset;

		if ( pspriteviewBridge->IsHit( drawparms, ptCursor ))
			return TRUE;

		if ( GetSprite()->IsTwoPiece( iDir, CStructureSprite::COMPLETED_STAGE, iDamage ))
		{
			pspriteviewBridge =
				GetSprite()->
					GetView(	iDir,
								CStructureSprite::BACKGROUND_LAYER,
							  	CStructureSprite::COMPLETED_STAGE,
							  	iDamage );

			ptOffset.y = ptCorner.y - pspriteviewBridge->Height();

			drawparms.m_ptOffset = ptOffset;

			if ( pspriteviewBridge->IsHit( drawparms, ptCursor ))
				return TRUE;
		}

		return FALSE;
	}

	// Draw partial construction

	pspriteviewBridge =
		GetSprite()->GetView( iDir,
									 CStructureSprite::FOREGROUND_LAYER,
 					  			    CStructureSprite::CONSTRUCTION_STAGE,
									 iDamage );

	ptOffset.y = ptCorner.y - pspriteviewBridge->Height();
	rect 		  = pspriteviewBridge->Rect() - pspriteviewBridge->Rect().TopLeft() + ptOffset;
	rect.top  += (( 100 - GetPer() ) * rect.Height() ) / 100;

	drawparms.m_ptOffset = ptOffset;

	if ( pspriteviewBridge->IsHitClip( drawparms, ptCursor, &rect ))
		return TRUE;

	if ( IsTwoPiece() )
	{
		pspriteviewBridge =
			GetSprite()->GetView( iDir,
										 CStructureSprite::BACKGROUND_LAYER,
 						  			    CStructureSprite::CONSTRUCTION_STAGE,
										 iDamage );

		ptOffset.y = ptCorner.y - pspriteviewBridge->Height();
		rect 		  = pspriteviewBridge->Rect() - pspriteviewBridge->Rect().TopLeft() + ptOffset;
		rect.top  += (( 100 - GetPer() ) * rect.Height() ) / 100;

		drawparms.m_ptOffset = ptOffset;

		if ( pspriteviewBridge->IsHitClip( drawparms, ptCursor, &rect ))
			return TRUE;
	}

	return FALSE;
}

#ifdef _DEBUG

void CBridgeUnit::AssertValid () const
{
	// assert base object
	CUnitTile::AssertValid();

	ASSERT ((m_iDir == 0) || (m_iDir == 1));
	#ifndef _GG
	ASSERT ((0 <= m_iExit) && (m_iExit <= 3));
	#endif
	ASSERT ((0 <= m_iAlt) && (m_iAlt <= 127));
	ASSERT ((0 <= m_iPerBuilt) && (m_iPerBuilt <= 100));
}

#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeMap - a map of all the bridges

void CBridgeMap::DeleteAll ()
{

	POSITION pos = GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBridge * pBrdg;
		GetNextAssoc (pos, dwID, pBrdg);
		ASSERT_STRICT_VALID (pBrdg);
		delete pBrdg;
		}

	RemoveAll ();
}


void CBridgeMap::Serialize( CArchive & ar )
{

	if (ar.IsStoring ())
		{
		ASSERT_VALID (this);

		ar << (WORD) GetCount ();
		POSITION pos = GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBridge * pBrdg;
			GetNextAssoc (pos, dwID, pBrdg);
			ASSERT_VALID (pBrdg);
			ar << pBrdg->m_dwID << pBrdg->m_hexStart << pBrdg->m_hexEnd << pBrdg->m_iAlt;
			}
		}
	else
	  {
		WORD wCount;
		ar >> wCount;
		while (wCount--)
			{
			DWORD dwID; CHexCoord hexStart, hexEnd; int iAlt;
			ar >> dwID >> hexStart >> hexEnd >> iAlt;
			CBridge * pBrdg = CBridge::Create (hexStart, hexEnd, dwID, iAlt);
			pBrdg->BridgeBuilt ();
			}
		}
}


/////////////////////////////////////////////////////////////////////////////
// CBridgeHex - hexes the bridges are on

void CBridgeHex::GrabHex (CBridgeUnit * pBrdg)
{

	SetAt (ToArg (pBrdg->GetHex().X(), pBrdg->GetHex().Y()), pBrdg);
	CHex * pHex = theMap._GetHex ( pBrdg->GetHex () );
	pHex->OrUnits ( CHex::bridge );

	// no forest under
	if ( pHex->GetType () == CHex::forest )
		{
		pHex->SetType ( CHex::plain );
		pHex->SetVisibleType ( CHex::plain );
		}
}

