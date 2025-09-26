//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// projbase.cpp : projectiles
//

#include "stdafx.h"
#include "base.h"
#include "lastplnt.h"
#include "world.h"
#include "sfx.h"
#include "event.h"
#include "area.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CExplGrp theExplGrp;
CProjMap theProjMap;
CInitProjMem theInitProjMem;
memory_pool<mempool_std_heap<188, 64>> CProjBase::m_memPool;


const int MAX_NUM_PROJECTILES = 24;
const int MAX_NUM_EXPLOSIONS = 48;


CExplData::CExplData ()
{

    m_pProjSprite = m_pExpSprite = NULL;
    m_iProjDelay = m_iExplSound = 0;
}


//---------------------------------------------------------------------------
// CProjBase

CProjBase::CProjBase (BYTE bTyp) : m_fixAlt( 0 ) 
{ 

    m_bType = bTyp;
    m_pNext = NULL;
}

//---------------------------------------------------------------------------
// CProjBase::Draw
//---------------------------------------------------------------------------
CRect CProjBase::Draw( const CHexCoord &hexcoord )
{
    ASSERT_STRICT_VALID (this);

    CMapLoc		maploc  ( hexcoord );
    CMapLoc3D	maploc3d( GetMapLoc().x, GetMapLoc().y, GetAlt() );
    CPoint		ptOffset( xpanimatr->WorldToWindow( xpanimatr->WorldToCenterWorld( maploc3d )));

    // Assumes projectiles are 1-piece

    CSpriteView *pspriteview = GetView();

    ptOffset.x -= pspriteview->Width()  >> 1;
    ptOffset.y -= pspriteview->Height() >> 1;

    CDrawParms	drawparms( *this, ptOffset );

    return pspriteview->Draw( drawparms );
}

CRect CProjectile::Draw( const CHexCoord &hexcoord )
{
    ASSERT_STRICT_VALID (this);

    CRect	rectBound = CProjBase::Draw( hexcoord );

    xpanimatr->GetDirtyRects()->AddRect( &rectBound, CDirtyRects::RECT_LIST::LIST_PAINT_BOTH );

    return rectBound;
}

CProjectile::CProjectile (CUnit const * pUnit, CMapLoc const & mlEnd, DWORD dwTarget, int iNumShots) : CProjBase (projectile)
{

    m_dwIDTarget = dwTarget;
    m_dwIDShooter = pUnit->GetID ();
    m_iNumShots = iNumShots;

    // attach the sprite
    if (pUnit->GetData()->GetProjectile () == 0)
        {
        TRAP ();
        m_pEd = NULL;
        m_iProjDelay = 0;
        }
    else
        {
        m_pEd = theExplGrp.GetData (pUnit->GetData()->GetProjectile ());
        m_iProjDelay = 0;
//BUGBUG - can't hide it		m_iProjDelay = m_pEd->m_iProjDelay;
        }
    if (m_pEd != NULL)
        {
        // if no projectile set it to fire the explosion
        if ( (m_psprite = m_pEd->m_pProjSprite) == NULL)
            {
            m_mlEnd = mlEnd;
            m_mlEnd.Wrap ();
            m_maploc = m_mlEnd;
            m_xAdd = m_yAdd = m_iSteps = 0;
            m_iStepMod = AVG_SPEED_MUL;
            return;
            }
        }

    m_maploc = pUnit->GetMapLoc ();
    m_maploc.Wrap ();
    m_mlEnd = mlEnd;
    m_mlEnd.Wrap ();

    m_fixAlt = m_maploc.CalcAltitude() + 1;

    int x = CMapLoc::Diff (mlEnd.x - m_maploc.x);
    int y = CMapLoc::Diff (mlEnd.y - m_maploc.y);

    if (abs (x) >= abs (y))
        {
        // could happen
        if (x == 0)
            {
            m_xAdd = m_yAdd = m_iSteps = 0;
            m_iStepMod = AVG_SPEED_MUL;
            return;
            }

        m_xAdd = STEPS_HEX;
        m_yAdd = (abs (y) * STEPS_HEX) / abs (x);
        m_iSteps = (abs (x) + STEPS_HEX - 1) / STEPS_HEX;
        }
    else
        {
        m_yAdd = STEPS_HEX;
        m_xAdd = (abs (x) * STEPS_HEX) / abs (y);
        m_iSteps = (abs (y) + STEPS_HEX - 1) / STEPS_HEX;
        }

    if (x < 0)
        m_xAdd = - m_xAdd;
    if (y < 0)
        m_yAdd = - m_yAdd;
    m_iStepMod = 0;

    // set the direction
    if ( (m_pEd != NULL) && (m_pEd->m_iFlags && CExplData::has_dir) )
        {
        PauseAnimations ( TRUE );
        SetFrame ( CSpriteView::ANIM_FRONT_1, ( ( FastATan ( x, y ) + 8 ) / 16 - 1 ) & 0x07 );
        }

    // move it out of the unit
    if (pUnit->GetUnitType() == CUnit::vehicle)
        {
        CSubHex subOn (m_maploc);
        while ((m_iSteps > 0) && ((subOn == ((CVehicle*)pUnit)->GetPtNext ()) ||
                                    (subOn == ((CVehicle*)pUnit)->GetPtHead ()) ||
                                    (subOn == ((CVehicle*)pUnit)->GetPtTail ())))
            {
            CSubHex subTmp (subOn);
            while ((subOn == subTmp) && (m_iSteps > 0))
                {
                m_maploc.x += m_xAdd;
                m_maploc.y += m_yAdd;
                m_maploc.Wrap ();
                m_iSteps--;
                subOn = m_maploc;
                }
            }
        }
    else
        if (pUnit->GetUnitType() == CUnit::building)
            {
            CHexCoord hexOn (m_maploc);
            int xDif = CHexCoord::Diff (hexOn.X() - ((CBuilding*)pUnit)->GetHex().X());
            int yDif = CHexCoord::Diff (hexOn.Y() - ((CBuilding*)pUnit)->GetHex().Y());
            while ((m_iSteps > 0) && (0 <= xDif) && (xDif < ((CBuilding*)pUnit)->GetCX ()) &&
                                        (0 <= yDif) && (yDif < ((CBuilding*)pUnit)->GetCY ()))
                {
                CHexCoord hexTmp (hexOn);
                while ((hexOn == hexTmp) && (m_iSteps > 0))
                    {
                    m_maploc.x += m_xAdd;
                    m_maploc.y += m_yAdd;
                    m_maploc.Wrap ();
                    m_iSteps--;
                    hexOn = m_maploc;
                    }
                xDif = CHexCoord::Diff (hexOn.X() - ((CBuilding*)pUnit)->GetHex().X());
                yDif = CHexCoord::Diff (hexOn.Y() - ((CBuilding*)pUnit)->GetHex().Y());
                }
            }

    if (m_iSteps <= 0)
        {
        m_iSteps = 0;
        m_iProjDelay = 0;
        m_faAdd = 0;
        m_iStepMod = AVG_SPEED_MUL;
        }
    else
        // handle the altitude
        {
        Fixed fEndAlt = m_mlEnd.CalcAltitude () + 1;
        m_faAdd = fEndAlt - m_fixAlt;
        if (fEndAlt >= m_fixAlt)
            m_faAdd += 1;
        m_faAdd /= m_iSteps;
        }

    ASSERT ((0 <= m_iSteps) && (m_iSteps < 160));
    TRAP (! ((0 <= m_iSteps) && (m_iSteps < 320)));
}

CExplosion::CExplosion (CProjectile const * pProj, CUnit * pTarget) : CProjBase (explosion)
{

    SetOneShotAnimations();

    m_maploc = pProj->m_mlEnd;
    m_maploc.Wrap ();
    m_dwIDTarget = pProj->m_dwIDTarget;
    m_dwIDShooter = pProj->m_dwIDShooter;
    m_iKillFrame = 10000;

    m_fixAlt = m_maploc.CalcAltitude ();

    // attach the sprite
    m_psprite = pProj->m_pEd->m_pExpSprite;

    // if it's not killed - it's the projectile's explosion
    if ((pTarget == NULL) || (pTarget->GetDamagePoints () > 0))
        {
        // turn on sound
        if (theMusicPlayer.SoundsPlaying ())
            {
            CWndArea * pWndArea = theAreaList.GetTop ();
            if ( pWndArea )
                {
                int iPan, iVol;

                if ((pTarget != NULL) && (pTarget->GetOwner()->IsMe ()))
                    {
                    pWndArea->GetPanAndVol ( pTarget, iPan, iVol );
                    theMusicPlayer.PlayForegroundSound (SOUNDS::GetID (pProj->m_pEd->m_iExplSound), iPan, iVol);
                    }
                else
                    {
                    CUnit * pShooter = ::GetUnit (m_dwIDShooter);
                    if ((pShooter != NULL) && (pShooter->GetOwner()->IsMe ()))
                        {
                        pWndArea->GetPanAndVol ( pShooter, iPan, iVol );
                        theMusicPlayer.PlayForegroundSound (SOUNDS::GetID (pProj->m_pEd->m_iExplSound), iPan, iVol);
                        }
                    }
                }
            }
        return;
        }

    // it's dying
    TRAP ();
    CUnit * pShooter = ::GetUnit (m_dwIDShooter);
    if ((pShooter != NULL) && (pShooter->GetOwner()->IsMe ()) && (pShooter->GetTarget () == pTarget))
        if ((pTarget != NULL) && (pTarget->GetUnitType () == CUnit::building))
            {
            TRAP ();
            theGame.Event (EVENT_ATK_DESTROYED, EVENT_NOTIFY, pShooter);
            }

    // we killed it - the explosion comes from the target
    CExplData const * pEd = theExplGrp.GetData ( 5 );

    // attach the sprite
    m_psprite = pEd->m_pExpSprite;

    // only hide under if dest visible
    if (theMap._GetHex ( CHexCoord (m_maploc) )->GetVisible ())
        m_iKillFrame = AnimCount (CSpriteView::ANIM_FRONT_1) / 2;

    // turn on sound
    if (theMusicPlayer.SoundsPlaying ())
        {
        CWndArea * pWndArea = theAreaList.GetTop ();
        if ( pWndArea )
            {
            int iPan, iVol;

            TRAP ();
            pWndArea->GetPanAndVol ( pTarget, iPan, iVol );
            theMusicPlayer.PlayForegroundSound (SOUNDS::GetID (SOUNDS::explosion_4), iPan, iVol);
            }
        }
}

CExplosion::CExplosion (CUnit const * pTarget, CMapLoc const & ml, DWORD dwIDShooter) : CProjBase (explosion)
{

    ASSERT_VALID (pTarget);

    SetOneShotAnimations();

    m_maploc = ml;
    m_maploc.Wrap ();
    m_dwIDTarget = pTarget->GetID ();
    m_dwIDShooter = dwIDShooter;

    m_fixAlt = m_maploc.CalcAltitude ();

    // we killed it - the explosion comes from the target
    CExplData const * pEd = theExplGrp.GetData ( 2 + RandNum (3) );

    // attach the sprite
    m_psprite = pEd->m_pExpSprite;

    // only hide under if dest visible
    if ( (pTarget->GetUnitType () != CUnit::building) || (pTarget->GetOwner()->IsMe ()) ||
                                                                                                        (((CBuilding*)pTarget)->IsLive ()) )
        m_iKillFrame = AnimCount (CSpriteView::ANIM_FRONT_1) / 2;
}

//---------------------------------------------------------------------------
// CExplosion::Draw - Puts bottom center of sprite at the explosion's location
//---------------------------------------------------------------------------
CRect
CExplosion::Draw(
    const CHexCoord &hexcoord )
{
    ASSERT_STRICT_VALID (this);

    CMapLoc		maploc  ( hexcoord );
    CMapLoc3D	maploc3d( GetMapLoc().x, GetMapLoc().y, GetAlt() );
    CPoint		ptOffset( xpanimatr->WorldToWindow( xpanimatr->WorldToCenterWorld( maploc3d )));

    // Assumes projectiles are 1-piece

    if ( !GetSprite() )
        return CRect( 0, 0, 0, 0 );

    CSpriteView *pspriteview = GetView();

    ptOffset.x -= pspriteview->Width()  >> 1;
    ptOffset.y -= pspriteview->Height();

    CDrawParms	drawparms( *this, ptOffset );

    return pspriteview->Draw( drawparms );
}

void CUnit::DecDamagePoints (int iDamage, DWORD dwKiller)
{

    // mark it for red
    if ( (iDamage > 0) && (GetOwner()->IsMe ()) )
        {
        m_iFrameHit = NUM_FRAMES_SHOW_HIT;
        if (GetUnitType () == CUnit::building)
            theApp.m_wndWorld.SetBldgHit ();
        else
            theApp.m_wndWorld.SetVehHit ();

        if ( GetUnitType () == CUnit::building )
            theAreaList.SetLastAttack ( ((CBuilding*) this)->GetHex () );
        }

    if (iDamage == 0)
        return;

    // if we are under construction it hurts more based on % done
    if ((GetUnitType () == CUnit::building) && (((CBuilding *)this)->m_iConstDone != -1))
        {
        int iTotalTime = ((CBuilding *)this)->m_iFoundTime + ((CBuilding *)this)->GetData()->GetTimeBuild ();
        int iDone = ((CBuilding *)this)->m_iConstDone;

        // do this to stop divide by zero and integer wrap - not cause I want to limit it
        if ((iTotalTime > iDone * 256) || (iDone == 0))
            iDamage *= 256;
        else
            iDamage += (iTotalTime * iDamage) / iDone;
        }

    m_iDamagePoints -= iDamage;
    if (m_iDamagePoints <= 0)
        {
        m_iDamagePoints = 0;
        PrepareToDie (dwKiller);
        }
    else
        if (m_iDamagePoints > GetData()->GetDamagePoints ())
            m_iDamagePoints = GetData()->GetDamagePoints ();

    int iOldPer = m_iDamagePer;
    m_iDamagePer = (m_iDamagePoints * 100) / GetData()->GetDamagePoints ();
    m_fDamageMult = (float) m_iDamagePoints / (float) GetData()->GetDamagePoints ();
    m_fDamPerfMult = 0.5f + m_fDamageMult / 2.0f;

    if ( (GetOwner()->IsMe ()) && (GetUnitType () == CUnit::building) &&
                                                                                            ( ! IsFlag (CUnit::destroying) ) )
        {
        if ( (iOldPer > 50) && (m_iDamagePer <= 50) )
            theGame.Event (EVENT_BLDG_HURTING, EVENT_NOTIFY, this);
        else
            if ( (iOldPer > 10) && (m_iDamagePer <= 10) )
                theGame.Event (EVENT_BLDG_DYING, EVENT_NOTIFY, this);
        }

    // we handle building fire rates in operate because of the power mul
//BUGBUG	if ( ( GetData()->_GetFireRate () > 0 ) && (GetUnitType () != CUnit::building) )
//BUGBUG		m_iFireRate = GetData()->_GetFireRate () + ((100 - m_iDamagePer) / 32) * GetData()->_GetFireRate ();

    // for non-buildings we update cause visibility says when we see them
    //   for buildings only if we see them
    if ( (GetUnitType () != CUnit::building) || (GetOwner()->IsMe ()) ||
                                                                                    ( ((CBuilding *)this)->IsLive () ) )
        UpdateDamageLevel ();
}

static int fnEnumUpdateBldg (CHex *, CHexCoord hex, void *)
{

    hex.SetInvalidated ();
    return (FALSE);
}

void CUnit::UpdateDamageLevel ()
{

    int iOldDamage = GetDamage ();

    // this tells the sprite engine which view to use
    if (m_iDamagePer > DAMAGE_1)
        SetDamage (0);
    else
        if (m_iDamagePer > DAMAGE_2)
            SetDamage (1);
        else
            if (m_iDamagePer > DAMAGE_3)
                SetDamage (2);
            else
                SetDamage (3);

    BOOL bRedraw = iOldDamage != GetDamage ();

    // this sets the smoke & fire level
    if ( m_iDamagePer != m_iLastShowDamagePer )
        {
        bRedraw = TRUE;
        m_iLastShowDamagePer = m_iDamagePer;
        CDamageDisplay * pDd = GetDamageDisplay ();
        if (pDd)
            pDd->SetDamage( min( 100, 100 - m_iDamagePer ));	// GG: Now takes a %
        }

//		{
//		if (m_iDamagePoints >= GetData()->GetDamagePoints ())
//			pDd->SetDamage( 0 );
//		else
//			pDd->SetDamage (15 - (16 * m_iDamagePoints) / GetData()->GetDamagePoints ());
//		}

    if (bRedraw)
        {
        if (GetUnitType () == CUnit::building)
            {
            CBuilding * pBldg = (CBuilding *) this;
            theMap.EnumHexes (pBldg->GetHex (), pBldg->GetCX (), pBldg->GetCY (), fnEnumUpdateBldg, NULL);
            }
        else
            if (GetUnitType () == CUnit::vehicle)
                {
                CVehicle * pVeh = (CVehicle *) this;
                CHexCoord _hex ( pVeh->GetPtHead () );
                _hex.SetInvalidated ();
                }
        MaterialChange ();
        }
}

void CProjectile::Operate ()
{
const int PROJ_OPER_MOV = ( 2 * AVG_SPEED_MUL ) / 3;

    m_iStepMod += theGame.GetOpersElapsed ();

    if (m_iStepMod < PROJ_OPER_MOV)
        return;
    div_t dtRate = div ( m_iStepMod, PROJ_OPER_MOV );
    int iNum = dtRate.quot;
    m_iStepMod = dtRate.rem;

    // if we're waiting - do nothing
    if ( m_iProjDelay > 0 )
        {
        if ( m_iProjDelay >= iNum )
            {
            m_iProjDelay -= iNum;
            return;
            }
        iNum -= m_iProjDelay;
        m_iProjDelay = 0;
        }

    CHexCoord _hex (GetMapLoc ());

    if (iNum < m_iSteps)
        {
        m_iSteps -= iNum;
        m_maploc.x += m_xAdd * iNum;
        m_maploc.y += m_yAdd * iNum;
        m_maploc.Wrap ();
        m_fixAlt += m_faAdd * iNum;
        theProjMap.Move (_hex, this);
        return;
        }

    // we're done
    theProjMap.Remove (_hex, this);

    // we've arrived - do we hit anything?
    CUnit * pShoot = GetUnit (m_dwIDShooter);
    if (pShoot != NULL)
        {
        CUnit * pTarget = GetUnit (m_dwIDTarget);
        TRAP ( theProjMap.GetCount () >= MAX_NUM_EXPLOSIONS );
        if ( ( m_pEd->m_pExpSprite != NULL ) && ( theProjMap.GetCount () < MAX_NUM_EXPLOSIONS ) )
            {
            CExplosion * pExpl = new CExplosion ( this, pTarget );
            theProjMap.Add (pExpl);
            }
        pShoot->AssessDamage (m_mlEnd, m_iNumShots, pTarget);
        }

    delete this;
}

// ok, we have to see what it hit where it landed
// we also subtract damage here if we are the shooter
void CUnit::AssessDamage ( CMapLoc const & ml, int iNumShots, CUnit * pTarget )
{

    // we assess damage on the shooters system
    if (! GetOwner()->IsLocal ())
        return;

    // walk all subs based on blast radius
    CSubHex _sub ( ml );
    int iRadius = GetData()->GetExpRadius ();

    // quad for diag check
    int x1, y2;
    if ( (ml.x & 31) < 16 )
        x1 = -1;
    else
        x1 = 1;
    if ( (ml.y & 31) < 16 )
        y2 = -1;
    else
        y2 = 1;

    for (int x=-iRadius; x<=iRadius; x++)
        for (int y=-iRadius; y<=iRadius; y++)
            {
            CSubHex _subTest ( _sub.x + x, _sub.y + y );
            _subTest.Wrap ();

            // find the target
            CUnit * pDamage;
            if ((pDamage = theBuildingHex._GetBuilding (_subTest)) == NULL)
                {
                pDamage = theVehicleHex._GetVehicle (_subTest);

                // special test for diaganol vehicles
                if (pDamage == NULL)
                    {
                    CSubHex _subDiag ( _subTest.x + x1, _subTest.y );
                    _subDiag.Wrap ();
                    pDamage = theVehicleHex._GetVehicle (_subDiag);
                    if ( pDamage == NULL )
                        continue;

                    // check above/below now for same
                    _subDiag = CSubHex ( _subTest.x, _subTest.y + y2 );
                    _subDiag.Wrap ();
                    if ( theVehicleHex._GetVehicle (_subDiag) != pDamage )
                        continue;
                    // we found a diaganol vehicle over us - hit it
                    }
                }

            // don't shoot ourselves
            if (pDamage == this)
                continue;

            // ok terrain/defense may eliminate the damage
            if (pDamage->GetUnitType () == CUnit::vehicle)
                {
                int iTyp = theMap._GetHex (_subTest)->GetType ();
                int iDefense = theTerrain.GetData (iTyp).GetDefenseMult (((CVehicle*)pDamage)->GetData()->GetWheelType ());
                if (iDefense > 1)
                    if (RandNum (iDefense) > 1)
                        continue;
                }

            // damage based on distance from center
            int iDamage = GetDamageInflicted (pDamage, TRUE);
            if (iDamage > 0)
                {
                int iDist = abs (x) + abs (y);
                // special case - infantry close in get an advantage
                if ( (iDist > 1) || (GetUnitType() != CUnit::vehicle) ||
                                                        ( ! ((CVehicle*)this)->GetData()->IsPeople () ) )
                    iDamage >>= iDist;
                else
                    if ( (iDist == 0) && (GetUnitType() == CUnit::vehicle) &&
                                                        ( ((CVehicle*)this)->GetData()->IsPeople () ) )
                        iDamage += iDamage / 4;
                }

            // post to all the damage we did
            if (iDamage > 0)
                pDamage->AddDamageThisTurn ( this, iDamage * iNumShots );
            }

}

int fnEnumHex2 (CHex *pHex, CHexCoord _hex, void *);

void CExplosion::Operate ()
{
    ASSERT_VALID (this);

    // VTBUGBUG
    // this shouldnt be nessasary:
    // 
    // --- Safety checks to avoid dereferencing NULL sprite/view/ambient ---
    // Why was this added? Some explosions were crashing the game.
    CSprite* pSprite = GetSprite( );
    if ( pSprite == NULL )
    {
        // No sprite attached - remove and destroy this explosion to avoid repeated crashes
        theProjMap.Remove( this );
        delete this;
        return;
    }

    CSpriteView* pView = GetView( );
    if ( pView == NULL )
    {
        // No view available - remove and destroy
        theProjMap.Remove( this );
        delete this;
        return;
    }


    int iOn = GetFrame( CSpriteView::ANIM_FRONT_1 );
    BOOL	bFinished = GetAmbient( CSpriteView::ANIM_FRONT_1 )->IsOneShotFinished( GetView() );

    // step 1 - if over the half way mark we hide the underlying unit
    if ( (iOn >= m_iKillFrame) || (bFinished && (m_iKillFrame < 256)) )
        {
        CUnit * pTarget = ::_GetUnit (m_dwIDTarget);
        if ( (pTarget != NULL) && (pTarget->IsFlag (CUnit::dying)) )
            {
            int iVis = pTarget->GetVisible ();
            if (pTarget->GetUnitType () == CUnit::vehicle)
                ((CVehicle *)pTarget)->ReleaseOwnership ();
            else
                if (pTarget->GetUnitType () == CUnit::building)
                    if (iVis == 0)
                        {
                        ((CBuilding *)pTarget)->MakeBldgVisible ();
                        iVis = pTarget->GetVisible ();
                        }

            // take the unit out & put destroyed city in
            pTarget->IncVisible ( - iVis);
            if (pTarget->GetUnitType () == CUnit::building)
                theMap.EnumHexes (((CBuilding *)pTarget)->GetHex (), ((CBuilding *)pTarget)->GetCX (),
                                                                                            ((CBuilding *)pTarget)->GetCY (), fnEnumHex2, NULL);

            CUnit * pShooter = ::GetUnit (m_dwIDShooter);
            if ((pShooter != NULL) && (pShooter->GetOwner()->IsMe ()) && (pShooter->GetTarget () == pTarget))
                if ((pTarget != NULL) && (pTarget->GetUnitType () == CUnit::building))
                    {
                    TRAP ();
                    theGame.Event (EVENT_ATK_DESTROYED, EVENT_NOTIFY, pShooter);
                    }
            }
        }

    // step 2 - delete if on the last frame
// GG: Ambient rewrite
//	int iTotal = AnimCount (CSpriteView::ANIM_FRONT_1);
//	if (iOn >= iTotal-1)
    if ( bFinished )
        {
        theProjMap.Remove (this);
        delete this;
        }

    // NOTE - this is not a one-shot because we have to alloc it to run
}

void CVehicle::HandleCombat ()
{

    ASSERT_STRICT (GetOwner()->IsLocal ());
    ASSERT_STRICT ((m_iEvent == attack) || (m_pUnitOppo != NULL));
    ASSERT_STRICT_VALID_OR_NULL (m_pUnitOppo);

    // we handle this on each local system
    // must be able to shoot
    if ( (! GetOwner()->IsLocal ()) || (GetFireRate () == 0) )
        return;

    // turn off temp_target if it starts moving or we got an oppo
    if ( m_bFlags & temp_target )
        {
        if ( (m_pUnitOppo != NULL) && (m_pUnitOppo != m_pUnitTarget) )
            {
            m_bFlags &= ~ temp_target;
            m_pUnitTarget = NULL;
            }
        else
            if ( (m_pUnitTarget != NULL) && (m_pUnitTarget->GetUnitType () == CUnit::vehicle) &&
                                                                    (((CVehicle *)m_pUnitTarget)->GetRouteMode () != stop) )
                {
                m_bFlags &= ~ temp_target;
                m_pUnitTarget = NULL;
                SetOppo ( NULL );
                }
        }

    // must have a target and not be in a building/carrier or we can't shoot
    // but we can reload...
    if ( ( (m_pUnitTarget == NULL) && (m_pUnitOppo == NULL) ) || (! GetHexOwnership () ) )
        {
#ifdef _LOGOUT
        logPrintf (LOG_PRI_VERBOSE, LOG_ATTACK, "Unit %d no target", GetID ());
#endif
        m_dwReloadMod += theGame.GetOpersElapsed ();
        m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);
        return;
        }

    BOOL bOkTurret = TRUE;	// fake for non-fixed turrets below
    CUnit * pUnitChecked = NULL;

    // do we need to refigure LOS?
    if (m_pUnitTarget != NULL)
        {
        BOOL bRedo = FALSE;
        if (GetPtHead () != m_ptUs)
            {
            m_ptUs = GetPtHead ();
            bRedo = TRUE;
            }
        if (m_pUnitTarget->GetUnitType () == CUnit::vehicle)
            if (((CVehicle*) m_pUnitTarget)->GetPtHead () != m_ptTarget)
                {
                m_ptTarget = ((CVehicle*) m_pUnitTarget)->GetPtHead ();
                bRedo = TRUE;
                }

        // refigure LOS
        if (bRedo)
            {
            m_iLOS = theMap.GetRangeDistance (this, m_pUnitTarget);
            if (m_iLOS <= GetRange ())
                m_iLOS = theMap.LineOfSight (this, m_pUnitTarget);

            // check for fixed turrets
            if (GetTurret () == NULL)
                {
                bOkTurret = CanShootAt (m_pUnitTarget);
                pUnitChecked = m_pUnitTarget;
                }

            // if the target has to travel more than 2 hexes to get out of range - stop
            //   1 hex if a bldg
            int iXtra = m_pUnitTarget->GetUnitType () == CUnit::vehicle ? 1 : 0;
            if (bOkTurret && (m_iLOS >= 0) && (m_iLOS < GetRange () - iXtra))
                SetRouteMode (stop);
            }
        }

    // who are we shooting at
    CUnit * pTarget = NULL;
    if ( bOkTurret && (m_pUnitTarget != NULL) && ((abs (m_iLOS) <= GetRange ()) &&
                            ((m_iLOS >= 0) || (GetData()->GetBaseType () == CTransportData::artillery))))
        pTarget = m_pUnitTarget;
    else
        if (m_pUnitOppo != m_pUnitTarget)
            pTarget = m_pUnitOppo;

    // make sure we go after him
    if ( ( GetRouteMode () == stop ) && ( m_pUnitTarget != NULL ) && ( pTarget != m_pUnitTarget ) )
        {
        // if in range and turret or 1hex, we turn below
        // AND spotted
        if ( (m_iLOS < 0) || (m_iLOS >= GetRange ()) || ( (GetTurret () == NULL) &&
                                                                ( ! (GetData()->GetVehFlags () & CTransportData::FL1hex) ) ) )
            if ( (m_pUnitTarget->GetUnitType () != CUnit::vehicle) ||
                                                                                        (theMap.GetHex (m_ptTarget)->GetVisible ()) )
                SetDestAndMode (m_ptTarget, sub);
        }

    // if no target handle reload and exit
    if ( pTarget == NULL )
        {
#ifdef _LOGOUT
        logPrintf (LOG_PRI_VERBOSE, LOG_ATTACK, "Unit %d no target in range (oppo: %d, target: %d)",
                    GetID (), m_pUnitOppo == NULL ? 0 : m_pUnitOppo->GetID (), m_pUnitTarget == NULL ? 0 : m_pUnitTarget->GetID () );
#endif
        m_dwReloadMod += theGame.GetOpersElapsed ();
        m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);

        // can't target oppo unit
        SetOppo ( NULL );
        return;
        }

    // is our turret pointing at them?
    if ( GetTurret () != NULL )
        {
        int iNew = FastATan (CMapLoc::Diff (pTarget->GetWorldPixels().x - m_maploc.x),
                                                    CMapLoc::Diff (pTarget->GetWorldPixels().y - m_maploc.y));
        int iDiff = (((iNew - GetTurret()->m_iDir) + 64) & 127) - 64;
        int iMax = 2 * theGame.GetOpersElapsed () / AVG_SPEED_MUL;
        if (iDiff < -iMax)
            {
            if (iDiff < -iMax * 4)
                bOkTurret = FALSE;
            iDiff = -iMax;
            }
        else
            if (iDiff > iMax)
                {
                if (iDiff > iMax * 4)
                    bOkTurret = FALSE;
                iDiff = iMax;
                }

        // if we're moving we just did the above to see if we can shoot
        //   we now need to swivel the turret in place if sitting
        if ( (GetRouteMode () != moving) && (iDiff != 0) )
            {
            int iNewDir = GetTurret()->m_iDir + iDiff;
            GetTurret()->m_iDir = __roll (0, FULL_ROT, iNewDir );
            }
        }

    else
        // can turn in place
        if ( GetData()->GetVehFlags () & CTransportData::FL1hex )
            {
            int iNew = FastATan (CMapLoc::Diff (pTarget->GetWorldPixels().x - m_maploc.x),
                                                    CMapLoc::Diff (pTarget->GetWorldPixels().y - m_maploc.y));
            int iDiff = (((iNew - m_iDir) + 64) & 127) - 64;
            if (iDiff < -2)
                {
                if (iDiff < -8)
                    bOkTurret = FALSE;
                iDiff = -2;
                }
            else
                if (iDiff > 2)
                    {
                    if (iDiff > 8)
                        bOkTurret = FALSE;
                    iDiff = 2;
                    }

            // if we're moving we just did the above to see if we can shoot
            //   we now need to swivel the turret in place if sitting
            if ( (GetRouteMode () != moving) && (iDiff != 0) )
                {
                int iNewDir = m_iDir + iDiff;
                m_iDir = __roll (0, FULL_ROT, iNewDir );
                }
            }

        else
            // check for fixed turrets
            if ( pUnitChecked != pTarget )
                if (! CanShootAt (pTarget))
                    bOkTurret = FALSE;

    // ok, can we shoot at them
    m_dwReloadMod += theGame.GetOpersElapsed ();
#ifdef _LOGOUT
    logPrintf (LOG_PRI_VERBOSE, LOG_ATTACK, "Unit %d reload: %d, OK: %d", GetID (), m_dwReloadMod, bOkTurret);
#endif

    if ((m_dwReloadMod >= (DWORD) GetFireRate () * AVG_SPEED_MUL) && (bOkTurret))
        {
        // artillery has indirect fire
        if ((m_pUnitTarget != NULL) && (pTarget == m_pUnitTarget))
            {
            // is it our first shot?
            if (m_pUnitOppo != m_pUnitTarget)
                {
                if (m_pUnitTarget->GetUnitType () == CUnit::building)
                    theGame.Event (EVENT_ATK_TARGET, EVENT_NOTIFY, this);
                m_pUnitOppo = m_pUnitTarget;

                // tell the target
                CMsgUnitAttacked msg (this,  m_pUnitTarget);
                theGame.PostToClient ( m_pUnitTarget->GetOwner(), &msg, sizeof (msg));
                }

            Shoot (m_pUnitTarget, m_iLOS);

            // turn it off once it's hit it
            if ( m_bFlags & temp_target )
                {
                m_bFlags &= ~ temp_target;
                m_pUnitOppo = m_pUnitTarget;
                m_pUnitTarget = NULL;
                }
            }

        // can't hit it - do we have oppo fire?
        else
            if ((m_pUnitOppo != NULL) && (m_pUnitOppo != m_pUnitTarget))
                {
                if (m_ptHead != m_ptUs)
                    {
                    m_ptUs = m_ptHead;
                    m_iOppoLOS = theMap.GetRangeDistance (this, m_pUnitOppo);
                    if (m_iOppoLOS <= GetRange ())
                        m_iOppoLOS = theMap.LineOfSight (this, m_pUnitOppo);
                    }
                else
                    if (m_pUnitOppo->GetUnitType () == CUnit::vehicle)
                        if (((CVehicle*) m_pUnitOppo)->GetPtHead () != m_ptOppo)
                            {
                            m_ptOppo = ((CVehicle*) m_pUnitOppo)->GetPtHead ();
                            m_iOppoLOS = theMap.GetRangeDistance (this, m_pUnitOppo);
                            if (m_iOppoLOS <= GetRange ())
                                m_iOppoLOS = theMap.LineOfSight (this, m_pUnitOppo);
                            }

                if ( (abs (m_iOppoLOS) <= GetRange ()) &&
                                ((m_iOppoLOS >= 0) || (GetData()->GetBaseType () == CTransportData::artillery)))
                    Shoot (m_pUnitOppo, m_iOppoLOS);
                // if we can't hit it it's not an oppo fire anymore
                else
                    bOkTurret = FALSE;
                }
        }

    // bad turret - do we give up on oppo?
    if ( ( ! bOkTurret ) && ( m_pUnitOppo != NULL ) )
        {
        // only care if we can't turn in place
        if ( ( (GetTurret () == NULL) && ( ! ( GetData()->GetVehFlags () & CTransportData::FL1hex ) ) ) ||
                        ( m_iOppoLOS < 0 ) || ( abs (m_iOppoLOS) <= GetRange () ) )
            {
            // see if we can find any target
            SetOppo (NULL);

            // if we're stopped AND we have no target AND our oppo fire unit is stopped - go get him
            if ( ( GetRouteMode () == stop ) && ( m_pUnitTarget == NULL ) &&
                            ( m_pUnitOppo != NULL ) && ( m_pUnitOppo->GetUnitType () == CUnit::vehicle ) &&
                                                                ( ((CVehicle *)m_pUnitOppo)->GetRouteMode () == stop ) )
                {
                MsgSetTarget ( m_pUnitOppo );
                if ( m_pUnitOppo != m_pUnitTarget )
                    m_bFlags |= temp_target;
                }
            }
        }

    // if can't hit our target - get going!!!
    if ( ( ! (m_bFlags & temp_target)) && ( GetRouteMode () == stop ) &&
                                                                        ( m_pUnitTarget != NULL ) && ( ! bOkTurret) )
        {
        // if next to we push away - otherwise we close
        if ( (m_pUnitTarget->GetUnitType () != CUnit::vehicle) ||
                                                                                        (theMap.GetHex (m_ptTarget)->GetVisible ()) )
            {
            if ( abs (m_iLOS) >= 2 )
                SetDestAndMode (m_ptTarget, sub);
            else
                if ( (GetTurret () == NULL) && ( ! ( GetData()->GetVehFlags () & CTransportData::FL1hex ) ) )
                    {
                    int xDif = CMapLoc::Diff (m_ptTarget.x - m_ptHead.x);
                    int yDif = CMapLoc::Diff (m_ptTarget.y - m_ptHead.y);
                    CSubHex _sub ( m_ptHead.x - 2 * __minmax (-1, 1, xDif), m_ptHead.y - 2 * __minmax (-1, 1, yDif) );
                    _sub.Wrap ();
                    SetDestAndMode (_sub, sub);
                    }
            }
        }

    // can't build up 2 shots if can't hit (safer than each correct if above)
    m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);
}

void CBuilding::HandleCombat ()
{

    ASSERT_STRICT (GetOwner()->IsLocal ());
    ASSERT_STRICT ((m_iEvent == attack) || (m_pUnitOppo != NULL));
    ASSERT_STRICT_VALID_OR_NULL (m_pUnitOppo);

    // we handle this on each local system
    // must be able to shoot
    if ( (! GetOwner()->IsLocal ()) || (GetFireRate () == 0) )
        return;

    // if we are under construction we can't shoot
    if ( IsConstructing () )
        {
        TRAP ();
        return;
        }

    // must have a target
    if ( (m_pUnitTarget == NULL) && (m_pUnitOppo == NULL) )
        {
        m_dwReloadMod += theGame.GetOpersElapsed ();
        m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);
        return;
        }

    // do we need to refigure LOS?
    if (m_pUnitTarget != NULL)
        {
        BOOL bRedo = FALSE;
        if (m_pUnitTarget->GetUnitType () == CUnit::vehicle)
            if (((CVehicle*) m_pUnitTarget)->GetPtHead () != m_ptTarget)
                {
                m_ptTarget = ((CVehicle*) m_pUnitTarget)->GetPtHead ();
                // find the closest point in the building
                bRedo = TRUE;
                }

        // refigure LOS
        if (bRedo)
            {
            m_iLOS = theMap.GetRangeDistance (this, m_pUnitTarget);
            if (m_iLOS < GetRange ())
                m_iLOS = theMap.LineOfSight (this, m_pUnitTarget);
            }
        }

    // who are we shooting at
    CUnit * pTarget = NULL;
    if ( (m_pUnitTarget != NULL) && (abs (m_iLOS) <= GetRange ()) )
        pTarget = m_pUnitTarget;
    else
        if (m_pUnitOppo != m_pUnitTarget)
            pTarget = m_pUnitOppo;

    m_dwReloadMod += theGame.GetOpersElapsed ();

    // vamoose if no one to shoot at
    if ( pTarget == NULL )
        {
        m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);
        return;
        }

    // ok, can we shoot at them
    if ( m_dwReloadMod >= (DWORD) GetFireRate () * AVG_SPEED_MUL )
        {
        // artillery has indirect fire
        if ((m_pUnitTarget != NULL) && (pTarget == m_pUnitTarget))
            {
            // is it our first shot?
            if (m_pUnitOppo != m_pUnitTarget)
                {
                m_pUnitOppo = m_pUnitTarget;

                // tell the target
                CMsgUnitAttacked msg (this,  m_pUnitTarget);
                theGame.PostToClient ( m_pUnitTarget->GetOwner(), &msg, sizeof (msg));
                }

            Shoot (m_pUnitTarget, m_iLOS);
            }

        // can't hit it - do we have oppo fire?
        else
            if ((m_pUnitOppo != NULL) && (m_pUnitOppo != m_pUnitTarget))
                {
                if (m_pUnitOppo->GetUnitType () == CUnit::vehicle)
                    if (((CVehicle*) m_pUnitOppo)->GetPtHead () != m_ptOppo)
                        {
                        m_ptOppo = ((CVehicle*) m_pUnitOppo)->GetPtHead ();
                        m_iOppoLOS = theMap.GetRangeDistance (this, m_pUnitOppo);
                        if (m_iOppoLOS <= GetRange ())
                            m_iOppoLOS = theMap.LineOfSight (this, m_pUnitOppo);
                        }
                if ( abs (m_iOppoLOS) <= GetRange () )
                    Shoot (m_pUnitOppo, m_iOppoLOS);
                // if we can't hit it it's not an oppo fire anymore
                else
                    SetOppo (NULL);
                }
        }

    // can't build up 2 shots if can't hit (safer than each correct if above)
    m_dwReloadMod = __min (m_dwReloadMod, (DWORD) GetFireRate () * AVG_SPEED_MUL);
}

void CUnit::Shoot (CUnit * pUnit, int iLOS)
{

    ASSERT_STRICT (GetOwner()->IsLocal ());

    // figure the number of shots

    div_t dtShoot = div ( m_dwReloadMod, GetFireRate () * AVG_SPEED_MUL );
    m_dwReloadMod = dtShoot.rem;
    if ( dtShoot.quot == 0 )
        {
        TRAP ();
        return;
        }

#ifdef _LOGOUT
    logPrintf (LOG_PRI_USEFUL, LOG_ATTACK, "Unit %d firing %d shots at unit %d", GetID (), dtShoot.quot, pUnit->GetID ());
#endif

    // get where it hits based on:
    //   turret/veh direction (vehicle only)
    //   accuracy
    CMapLoc mlTarget = pUnit->GetWorldPixels ();

    // now accuracy enters the picture
    // accuracy sucks for hovercraft on water
    int iAccuracy = GetAccuracy ();
    if ( GetUnitType () == CUnit::vehicle )
        {
        // no boats on water have lousy accuracy
        CVehicle * pVeh = (CVehicle *) this;
        if (pVeh->GetData()->GetWheelType () != CWheelTypes::water)
            if (theMap.GetHex (pVeh->GetPtHead ())->IsWater ())
                iAccuracy = (iAccuracy + 2) * 2;
        // moving add 1
        if ( pVeh->GetRouteMode () == CVehicle::moving )
            iAccuracy ++;
        }

    int iRange = ((abs (iLOS) + 1) * iAccuracy) / 4;
    iRange = __min (16, iRange);
    const int aSqrt[17] = {0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,4};
    iRange = aSqrt [ iRange ];
    if (iRange > 0)
        {
        mlTarget.x += iRange * (MAX_HEX_HT / 4) - RandNum (iRange * (MAX_HEX_HT / 2));
        mlTarget.y += iRange * (MAX_HEX_HT / 4) - RandNum (iRange * (MAX_HEX_HT / 2));
        }
    mlTarget.Wrap ();

    // fire away
    // we can do this because this message is for the projectile sprite, NOT the damage
    if ( !theGame.AreMessagesPaused() )
        {
        int iNum = theGame.m_msgShoot.AddShot ( this, pUnit, mlTarget, dtShoot.quot );
        if ( iNum >= NUM_SHOOT_ELEM )
            {
            theGame.PostToAll ( &(theGame.m_msgShoot), theGame.m_msgShoot.SendSize (), FALSE);
            theGame.m_msgShoot.Reset ();
            }
        }
}

void CUnit::MsgSetFire (CMsgShootElem * pMsg)
{

    // get target
    CUnit * pTarget = ::GetUnit (pMsg->m_dwIDTarget);

    // do we show the proj (could be AI)
    BOOL bShowProj;
    if ( (GetOwner()->IsMe ()) || ( (pTarget != NULL) && (pTarget->GetOwner()->IsMe ()) ) )
        bShowProj = TRUE;
    else
        {
        // is it visible
        if ( ( (GetUnitType() == CUnit::building) && (((CBuilding *)this)->IsLive ()) ) ||
                 ( (GetUnitType() == CUnit::vehicle) && (theMap._GetHex (((CVehicle *)this)->GetPtHead ())->GetVisible ()) ) )
            bShowProj = TRUE;
        else
            bShowProj = FALSE;
        }

    // if we aren't shooting and aren't near the target - we don't care
    if ( ( ! bShowProj ) && ( ! GetOwner()->IsLocal () ) )
        return;

    CMapLoc mlDest ( pMsg->m_wDestX, pMsg->m_wDestY );

    // get the projectile
    if ( bShowProj )
        {
        // play sound if we shoot or we're hit
        if ( (theMusicPlayer.SoundsPlaying ()) && (GetData ()->GetSoundShoot () > 0) )
            {
            CWndArea * pWndArea = theAreaList.GetTop ();
            if ( pWndArea )
                {
                int iPan, iVol;
                pWndArea->GetPanAndVol ( this, iPan, iVol );
                theMusicPlayer.PlayForegroundSound (SOUNDS::GetID ( GetData ()->GetSoundShoot () ), iPan, iVol );
                }
            }

        // turn on muzzle flash
        if ( GetData()->GetUnitFlags () & CUnitData::FLflash1 )
            {
            GetAmbient ( CSpriteView::ANIM_FRONT_1 )->SetOneShot ( TRUE );
            GetAmbient ( CSpriteView::ANIM_FRONT_1 )->Enable ( TRUE );
            }
        if ( GetData()->GetUnitFlags () & CUnitData::FLflash2 )
            {
            TRAP ();
            GetAmbient ( CSpriteView::ANIM_FRONT_2 )->SetOneShot ( TRUE );
            GetAmbient ( CSpriteView::ANIM_FRONT_2 )->Enable ( TRUE );
            }
        if ( GetTurret() )
            GetTurret()->DoMuzzleFlash();

        if ( ( GetData()->GetProjectile () != 0 ) && ( theProjMap.GetCount () < MAX_NUM_PROJECTILES ) )
            {
            // start the projectile
            CProjectile * pProj = new CProjectile (this, mlDest, pMsg->m_dwIDTarget, pMsg->m_wNumShots);
            if (pProj->m_iSteps > 0)
                theProjMap.Add (pProj);
            else
                {
                if ( pProj->m_pEd->m_pExpSprite != NULL )
                    {
                    CExplosion * pExpl = new CExplosion (pProj, pTarget);
                    theProjMap.Add (pExpl);
                    }
                delete pProj;
                AssessDamage ( mlDest, pMsg->m_wNumShots, pTarget);
                }
            return;
            }
        }

    // just do damage - no projectile (or not visible)
    AssessDamage (mlDest, pMsg->m_wNumShots, pTarget);
}

static int fnEnumBlowUpBldg (CHex *, CHexCoord hex, void * pData)
{

    if ( theProjMap.GetCount () > MAX_NUM_EXPLOSIONS + 4 )
        return (TRUE);

    // middle of the hex
    CMapLoc ml ( hex );
    ml.x += 32;
    ml.y += 32;
    ml.Wrap ();

    CExplosion * pExpl = new CExplosion ( (CBuilding *) pData, ml, 0 );
    theProjMap.Add (pExpl);

    return (FALSE);
}

void CUnit::PrepareToDie (DWORD dwIDKiller)
{

    // if we are already dying - don't send again
    if (m_unitFlags & dying)
        return;

    if ( ( theApp.m_pLogFile != NULL ) && ( GetUnitType () == CUnit::building ) )
        {
        CBuilding * pBldg = (CBuilding*) this;
        char sBuf [80];
        sprintf ( sBuf, "PrepareToDie building %d at %d,%d",
                                            pBldg->GetID (), pBldg->GetHex().X(), pBldg->GetHex().Y() );
        theApp.Log ( sBuf );
        }

    // this gets us out of being called for operate
    SetFlag (dying);

    // explosion - if visible
    BOOL bSfx = FALSE;
    if ( GetUnitType () == CUnit::vehicle )
        {
        if ( GetOwner()->IsMe () || theMap._GetHex ( ((CVehicle*)this)->GetPtHead ())->GetVisible () )
            {
            if ( theProjMap.GetCount () < MAX_NUM_EXPLOSIONS + 2 )
                {
                CExplosion * pExpl = new CExplosion ( this, m_maploc, dwIDKiller );
                theProjMap.Add (pExpl);
                }
            bSfx = TRUE;
            }
        }
    else
        if ( GetUnitType () == CUnit::building )
            {
            CBuilding * pBldg = (CBuilding *) this;
            if ( GetOwner()->IsMe () || pBldg->IsLive () )
                {
                theMap.EnumHexes (pBldg->GetHex (), pBldg->GetCX (), pBldg->GetCY (), fnEnumBlowUpBldg, pBldg);
                bSfx = TRUE;
                }
            }

    // special building dead sound
    if ( bSfx && theMusicPlayer.SoundsPlaying () )
        {
        CWndArea * pWndArea = theAreaList.GetTop ();
        if ( pWndArea )
            {
            int iPan, iVol;

            pWndArea->GetPanAndVol ( this, iPan, iVol );
            theMusicPlayer.PlayForegroundSound (SOUNDS::GetID
                                (GetUnitType () == CUnit::building ? SOUNDS::explosion_4 : SOUNDS::explosion_3),
                                iPan, iVol);
            }
        }

    m_dwPlyrKiller = dwIDKiller;

    // tell the player
    CUnit * pShooter = ::GetUnit (dwIDKiller);
    if ((pShooter != NULL) && (pShooter->GetOwner()->IsMe ()) && (pShooter->GetTarget () == this))
        if ((this != NULL) && (this->GetUnitType () == CUnit::building))
            theGame.Event (EVENT_ATK_DESTROYED, EVENT_NOTIFY, this);

    // tell everyone
    if ( theGame.AmServer () )
        {
        CMsgDeleteUnit msg (this, dwIDKiller);
        theGame.PostToAll (&msg, sizeof (msg));
        }
}


/////////////////////////////////////////////////////////////////////////////
// we keep a map (based on CHex) of lists (all proj in a hex)

void CProjMap::Add (CProjBase * pProj)
{

    pProj->m_pNext = NULL;

    CHexCoord _hex (pProj->GetMapLoc ());
    DWORD dwKey = ToArg (_hex);

    CProjBase * pPb;

    // see if already have a list
    if (Lookup (dwKey, pPb) != NULL)
        {
        // put us at the end
        while ( pPb->m_pNext != NULL )
            pPb = pPb->m_pNext;
        pPb->m_pNext = pProj;
        return;
        }

    // no list - just us
    SetAt (dwKey, pProj);

    // tell theMap
    theMap._GetHex (_hex)->OrUnits (CHex::proj);
}

void CProjMap::Remove (CHexCoord const & _hex, CProjBase * pProj)
{

    ASSERT (theMap._GetHex (_hex)->GetUnits () & CHex::proj);

    DWORD dwKey = ToArg (_hex);
    CProjBase * pPb;

    // if no list - BUG
    if (Lookup (dwKey, pPb) == NULL)
        {
        TRAP ();
        ASSERT (FALSE);
        return;
        }

    // if first, replace in the map
    if ( pPb == pProj )
        {
        // if only 1 - kill it
        if ( pPb->m_pNext == NULL )
            {
            RemoveKey (dwKey);
            theMap._GetHex (_hex)->NandUnits (CHex::proj);
            return;
            }

        // set it to next
        SetAt (dwKey, pPb->m_pNext);
        return;
        }

    // remove it from the list
    while ( pPb->m_pNext != pProj)
        {
        // bug!!
        if ( pPb->m_pNext == NULL )
            {
            TRAP ();
            return;
            }
        pPb = pPb->m_pNext;
        }

    // take it out
    pPb->m_pNext = pProj->m_pNext;
}

