//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// sprite.cpp : the sprites
//

#include "stdafx.h"
#include "event.h"
#include "lastplnt.h"
#include "player.h"
#include "sprite.h"
#include "cpathmgr.h"
#include "error.h"
#include "chproute.hpp"
#include "ai.h"
#include "icons.h"
#include "bitmaps.h"
#include "area.h"
#include "relation.h"
#include "sfx.h"

#include "terrain.inl"
#include "minerals.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


CUnitShowStat uShowStat;


void ShowBuilding( int iInd, CBuilding* pBldg );
void UnshowBuilding( CBuilding* pBldg );


CString  CUnit::m_sDamage;
COLORREF CUnit::m_clrOk;
COLORREF CUnit::m_clrWarn;
COLORREF CUnit::m_clrBad;

CString  CMaterialTypes::m_saDesc[num_types];
COLORREF CMaterialTypes::m_rgb[num_types + 1];

// GG: Default sprite for each vehicle category.
// 	 All .DAT files should include these sprites.

int CTransport::g_aiDefaultID[] = {
    CTransportData::heavy_truck,    // default non-combat
    CTransportData::light_art,      // default artillery
    CTransportData::infantry,       // default troops
    CTransportData::landing_craft,  // default ship
    CTransportData::light_tank      // default combat
};

void UnitStatusText( void* pData, CString& str )
{

    CUnit* pUnit = (CUnit*)pData;

    pUnit->ShowStatusText( str );
}

void CUnit::ShowStatusText( CString& str )
{

    // list the damage level
    str.LoadString( IDS_STAT_DAMAGE );
    int iPer     = 100 - GetDamagePer( );
    iPer         = __min( 99, iPer );
    CString sNum = IntToCString( iPer );
    csPrintf( &str, (char const*)sNum );
}

void CBuilding::ShowStatusText( CString& str )
{

    // if under construction show % done
    if ( m_iConstDone != -1 )
    {
        str.LoadString( IDS_STAT_CONST );
        CString sNum = IntToCString( m_iLastPer );
        csPrintf( &str, (char const*)sNum );
        return;
    }

    if ( GetData( )->GetType( ) == CStructureData::rocket )
    {
        str.LoadString( IDS_STAT_ROCKET );
        CString sNum1 = IntToCString( theGame.GetMe( )->GetPplTotal( ) );
        CString sNum2 = IntToCString( theGame.GetMe( )->m_iAptCap );
        CString sNum3 = IntToCString( theGame.GetMe( )->GetPplBldg( ) );
        CString sNum4 = IntToCString( theGame.GetMe( )->m_iOfcCap );
        CString sNum5 = IntToCString( (int)( 15.0 * GetFrameProd( 1 ) ) );
        csPrintf( &str, (char const*)sNum1, (char const*)sNum2, (char const*)sNum3, (char const*)sNum4,
                  (char const*)sNum5 );
        return;
    }

    // research
    if ( GetData( )->GetType( ) == CStructureData::research )
    {
        if ( theGame.GetMe( )->GetRsrchItem( ) <= 0 )
        {
            str.LoadString( IDS_STAT_IDLE );
            return;
        }
        str.LoadString( IDS_RESEARCHING );
        csPrintf( &str, (char const*)theRsrch.ElementAt( theGame.GetMe( )->GetRsrchItem( ) ).m_sName );
        return;
    }

    // list the damage level
    CUnit::ShowStatusText( str );
}

void CMaterialBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    // show creating rate
    CBuildMaterials const* pBm  = GetData( )->GetBldMaterials( );
    int                    iTyp = 0;
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumTypes( ); iInd++ )
    {
        if ( pBm->GetOutput( iInd ) > 0 )
        {
            iTyp = iInd;
            break;
        }
    }

    // this is the only way it generates code correctly
    char const* pDesc = CMaterialTypes::GetDesc( iTyp );
    char        sNum[14];
    int         iNum = (int)( GetFrameProd( GetOwner( )->GetMtrlsProd( ) * float( 24 * 60 * pBm->GetOutput( iTyp ) ) /
                                            float( pBm->GetTime( ) ) ) );
    itoa( iNum, sNum, 10 );

    str.LoadString( IDS_STAT_MATERIAL );
    csPrintf( &str, pDesc, sNum );
}

void CVehicleBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    CBuildUnit const* pBldUnt = GetBldUnt( );
    if ( pBldUnt == NULL )
    {
        str.LoadString( IDS_STAT_IDLE );
        return;
    }

    // show what we are building
    str.LoadString( IDS_STAT_BUILD );
    CString sNum = IntToCString( m_iLastPer );
    csPrintf( &str, (LPCSTR)theTransports.GetData( pBldUnt->GetVehType( ) )->GetDesc( ), (char const*)sNum );
}

void CFarmBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    str.LoadString( IDS_STAT_FARM );
    CBuildFarm* pBf = GetData( )->GetBldFarm( );
    CString     sNum =
        IntToCString( GetFrameProd( GetOwner( )->GetFarmProd( ) * m_iTerMult * float( 24 * 60 * pBf->GetQuantity( ) ) /
                                    float( pBf->GetTimeToFarm( ) ) ),
                      10, TRUE );
    csPrintf( &str, (char const*)sNum );
}

void CPowerBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    CBuildPower* pBp = GetData( )->GetBldPower( );
    if ( pBp->GetInput( ) >= 0 )
        if ( GetStore( pBp->GetInput( ) ) <= 0 )
        {
            str.LoadString( IDS_STAT_IDLE );
            return;
        }

    str.LoadString( IDS_STAT_POWER );
    CString sNum  = IntToCString( (int)( GetFrameProd( 1 ) * (float)GetData( )->GetBldPower( )->GetPower( ) ) );
    CString sNum2 = IntToCString( GetData( )->GetBldPower( )->GetPower( ) );
    csPrintf( &str, (char const*)sNum, (char const*)sNum2 );
}

void CRepairBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    if ( m_pVehRepairing == NULL )
    {
        str.LoadString( IDS_STAT_IDLE );
        return;
    }

    // show what we are building
    str.LoadString( IDS_STAT_REPAIR );
    CString sNum = IntToCString( m_iLastPer );
    csPrintf( &str, (LPCSTR)m_pVehRepairing->GetData( )->GetDesc( ), (char const*)sNum );
}

void CShipyardBuilding::ShowStatusText( CString& str )
{

    if ( ( m_iConstDone != -1 ) || ( m_iMode != repair ) || ( m_pVehRepairing == NULL ) )
    {
        CVehicleBuilding::ShowStatusText( str );
        return;
    }

    // show what we are building
    str.LoadString( IDS_STAT_REPAIR );
    CString sNum = IntToCString( m_iLastPer );
    csPrintf( &str, (LPCSTR)m_pVehRepairing->GetData( )->GetDesc( ), (char const*)sNum );
}

void CHousingBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    // show housing needs
    int iHave, iNeed, iRes1, iRes2;
    if ( GetData( )->GetBldgType( ) == CStructureData::apartment )
    {
        iHave = theGame.GetMe( )->m_iAptCap;
        iNeed = theGame.GetMe( )->GetPplTotal( );
        iRes1 = IDS_STAT_APT;
        iRes2 = IDS_STAT_APT2;
    }
    else
    {
        iHave = theGame.GetMe( )->m_iOfcCap;
        iNeed = theGame.GetMe( )->GetPplBldg( );
        iRes1 = IDS_STAT_OFFICE;
        iRes2 = IDS_STAT_OFFICE2;
    }

    str.LoadString( iHave >= iNeed ? iRes1 : iRes2 );
    CString sNum1 = IntToCString( iHave >= iNeed ? iNeed : iHave, 10, TRUE );
    CString sNum2 = IntToCString( iHave >= iNeed ? iHave - iNeed : iNeed, 10, TRUE );
    csPrintf( &str, (char const*)sNum1, (char const*)sNum2 );
}

void CMineBuilding::ShowStatusText( CString& str )
{

    if ( m_iConstDone != -1 )
    {
        CBuilding::ShowStatusText( str );
        return;
    }

    // if out - say so
    if ( m_iMinerals <= 0 )
    {
        str.LoadString( IDS_EXHAUSTED );
        csPrintf( &str, (LPCSTR)CMaterialTypes::GetDesc( GetData( )->GetBldMine( )->GetTypeMines( ) ) );
        return;
    }

    CBuildMine* pBm = GetData( )->GetBldMine( );
    int         iRate10 =
        (int)( GetFrameProd( GetOwner( )->GetMineProd( ) * (float)( 10 * m_iDensity * 24 * 60 * pBm->GetAmount( ) ) /
                             (float)( CMinerals::DensityDiv( ) * pBm->GetTimeToMine( ) ) ) );
    CString sNum1;
    if ( iRate10 >= 20 )
        sNum1 = IntToCString( iRate10 / 10 );
    else
        sNum1 = IntToCString( iRate10 / 10 ) + "." + IntToCString( iRate10 % 10 );

    str.LoadString( IDS_STAT_MINE );
    CString sNum2 = IntToCString( m_iMinerals, 10, TRUE );
    CString sNum3 = IntToCString( m_iDensity, 10, TRUE );
    csPrintf( &str, (char const*)sNum1, (char const*)sNum2, (char const*)sNum3 );
}

void CVehicle::ShowStatusText( CString& str )
{

    if ( GetData( )->IsCarrier( ) )
    {
        str.LoadString( IDS_STAT_CARRIER );
        CString sNum = IntToCString( m_lstCargo.GetCount( ) );
        csPrintf( &str, (char const*)sNum );
        return;
    }

    CUnit::ShowStatusText( str );
}

void CUnitShowStat::Init( )
{

    m_siText.Attach( &theIcons, ICON_BAR_TEXT );
    for ( int iOn = 0; iOn < 3; iOn++ ) m_si[iOn][0].Attach( &theIcons, ICON_DAMAGE );
}

void CUnitShowStat::Close( )
{

    delete m_siText.m_pDib;
    m_siText.m_pDib  = NULL;
    m_siText.m_rDest = CRect( 0, 0, 0, 0 );

    for ( int iOn = 0; iOn < 3; iOn++ )
        for ( int iNum = 0; iNum < 3; iNum++ )
        {
            delete m_si[iOn][iNum].m_pDib;
            m_si[iOn][iNum].m_pDib  = NULL;
            m_si[iOn][iNum].m_rDest = CRect( 0, 0, 0, 0 );
        }
}

void _UnitShowStatus( BOOL bText, void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDibBack, CPoint const& ptOff )
{

    CUnit* pUnit = (CUnit*)pData;

    CRect rect( rDraw );

    // first the description
    if ( bText )
    {
        uShowStat.m_siText.SetBack( pDibBack, ptOff );
        int iWid   = 14 * 8 + uShowStat.m_siText.m_pStatData->m_leftOff + uShowStat.m_siText.m_pStatData->m_rightOff;
        iWid       = __min( rect.Width( ) / 2, iWid );
        rect.right = rect.left + iWid;
        uShowStat.m_siText.SetSize( rect );

        uShowStat.m_siText.SetText( pUnit->GetData( )->GetDesc( ) );

#ifdef _CHEAT
        if ( _bShowStatus )
        {
            CString sText( IntToCString( pUnit->GetID( ) ) + CString( "(" ) +
                           IntToCString( pUnit->GetOwner( )->GetPlyrNum( ) ) + CString( "): " ) +
                           pUnit->GetData( )->GetDesc( ) );
            uShowStat.m_siText.SetText( sText );
        }
#endif

        uShowStat.m_siText.DrawIcon( pDc );
        rect.left = rect.right;
    }

    // if it's not us - name & damage only
    if ( !pUnit->GetOwner( )->IsMe( ) )
    {
        // name
        CStatInst* pSi = &( uShowStat.m_si[1][0] );
        rect.left++;
        rect.right = rect.left + ( rDraw.right - rect.left ) / 2;
        pSi->Attach( &theIcons, ICON_BAR_TEXT );
        pSi->SetBack( pDibBack, ptOff );
        pSi->SetSize( rect );
        pSi->SetText( pUnit->GetOwner( )->GetName( ) );
        pSi->DrawIcon( pDc );

        // damage
        rect.left = rect.right + 1;
        rect.right += rDraw.right - rect.left;
        pSi++;
        pSi->Attach( &theIcons, ICON_DAMAGE );
        pSi->SetBack( pDibBack, ptOff );
        pSi->SetSize( rect );
        pUnit->CUnit::PaintStatusBars( pSi, 0, pDc );
        return;
    }

    // now the status
    int iNum       = pUnit->GetNumStatusBars( );
    rect.right     = rect.left;
    CStatInst* pSi = &( uShowStat.m_si[iNum - 1][0] );
    if ( iNum >= 3 )
        pSi = &( uShowStat.m_si[0][0] );

    for ( int iOn = 0; iOn < iNum; iOn++ )
    {
        rect.left = rect.right + 1;
        rect.right += ( rDraw.right - rect.left ) / ( iNum - iOn );

        pSi->SetBack( pDibBack, ptOff );
        pSi->SetSize( rect );
        pUnit->PaintStatusBars( pSi, iOn, pDc );
        pSi++;
    }
}

void UnitShowStatus( void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDibBack, CPoint const& ptOff )
{

    _UnitShowStatus( TRUE, pData, pDc, rDraw, pDibBack, ptOff );
}

// ignore the line number - we show damage
void CUnit::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    pSi->Attach( &theIcons, ICON_DAMAGE );
    pSi->SetPer( __max( 1, GetOwner( )->IsMe( ) ? GetDamagePer( ) : GetLastShowDamagePer( ) ) );
    pSi->DrawIcon( pDc );
}

void CUnit::PaintStatusMaterials( CStatInst* pSi, CDC* pDc ) const
{

    ASSERT_VALID( this );

    // get the icon
    pSi->Attach( &theIcons, ICON_MATERIALS );

    CStatData* pSd = pSi->m_pStatData;

    // draw the background
    pSi->DrawBackground( );

    // lets get the total
    int iTotal = GetTotalStore( );
    if ( iTotal > 0 )
    {

        // the icon area rect
        CRect rect( pSi->m_rDest );
        rect.OffsetRect( -rect.left, -rect.top );
        rect.top += ( rect.Height( ) - pSd->m_cyIcon ) / 2;
        rect.bottom = rect.top + pSd->m_cyIcon;
        rect.left += pSd->m_leftOff;
        rect.right -= pSd->m_rightOff;
        int iLen     = rect.Width( ) - pSd->m_cxIcon;
        int iRight   = rect.right - pSd->m_cxIcon;
        rect.right   = rect.left + pSd->m_cxIcon;
        int iIconAdd = pSd->m_cxIcon / 2;

        // this way we leave it part empty for vehicles that are not full
        if ( GetUnitType( ) == CUnit::vehicle )
        {
            if ( iTotal < ( (CVehicle*)this )->GetData( )->GetMaxMaterials( ) )
                iTotal = ( (CVehicle*)this )->GetData( )->GetMaxMaterials( );
        }
        else
            // this leaves it part empty for small amounts
            iTotal = __max( iTotal, 500 * ( iLen / iIconAdd ) );

        // we get the number of items to insure at least 1 icon of each
        int iNumTypes = 0;
        for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
            if ( GetStore( iOn ) != 0 )
                iNumTypes++;

        for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
            if ( GetStore( iOn ) != 0 )
            {
                // the below means we handle fractions and don't under/over-run
                int iWid = ( iLen * GetStore( iOn ) ) / iTotal;
                iWid     = __max( 1, iWid );                        // must show
                iNumTypes--;
                iWid = __min( iWid, iLen - iNumTypes * iIconAdd );  // room for 1 of ea remaining
                iTotal -= GetStore( iOn );
                iWid += rect.left;

                // draw them
                CPoint pt( iOn * pSd->m_cxIcon, 0 );
                for ( int x = rect.left; x < iWid; x += iIconAdd )
                {
                    if ( rect.left > iRight )
                        break;
                    pSd->m_pcDib->TranBlt( pSi->m_pDib, rect, pt );
                    rect.OffsetRect( iIconAdd, 0 );
                    iLen -= iIconAdd;
                }
            }
    }  // if iTotal . 0

    // draw it to the screen
    pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );
}

int CBuilding::GetNumStatusBars( ) const
{

    // damage, materials, const
    if ( m_iConstDone != -1 )
        return ( 3 );

    // seaports have 3
    if ( GetData( )->GetType( ) == CStructureData::seaport )
        return ( 3 );

    // if any materials (left over?) show it
    if ( GetTotalStore( ) > 0 )
        return ( 2 );

    // if accepts materials show it
    int iAcpts[CMaterialTypes::num_types];
    GetAccepts( iAcpts );
    for ( int iOn = 0; iOn < CMaterialTypes::num_types; iOn++ )
        if ( iAcpts[iOn] != 0 )
            return ( 2 );

    // damage
    return ( 1 );
}

void CBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    if ( iNum == 0 )
    {
        CUnit::PaintStatusBars( pSi, 0, pDc );
        return;
    }

    // materials
    if ( iNum == 1 )
    {
        PaintStatusMaterials( pSi, pDc );
        return;
    }

    // we are under construction
    if ( m_iConstDone != -1 )
    {
        pSi->Attach( &theIcons, ICON_CONSTRUCTION );
        pSi->SetPer( __max( 1, m_iLastPer ) );
        pSi->DrawIcon( pDc );
        return;
    }

    // show vehicles in us
    // get the icon
    pSi->Attach( &theIcons, ICON_VEHICLES );
    CStatData* pSd = pSi->m_pStatData;

    // draw the background
    pSi->DrawBackground( );

    // rect to render to
    CRect rect( pSi->m_rDest );
    rect.OffsetRect( -rect.left, -rect.top );
    rect.top += ( rect.Height( ) - pSd->m_cyIcon ) / 2;
    rect.bottom = rect.top + pSd->m_cyIcon;
    rect.left += pSd->m_leftOff;
    rect.right -= pSd->m_rightOff;
    int iRight = rect.right - pSd->m_cxIcon;
    rect.right = rect.left + pSd->m_cxIcon;

    // display them in order
    POSITION pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        // it's ours, it's in a building, and it's stopped (not unloading/deploying, etc)
        if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( pVeh->GetTransport( ) == NULL ) && ( !pVeh->GetHexOwnership( ) ) &&
             ( pVeh->GetRouteMode( ) == CVehicle::stop ) )
            if ( this == theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) )
            {
                if ( rect.left > iRight )
                    break;

                // draw it
                CPoint pt( pVeh->GetData( )->GetType( ) * pSd->m_cxIcon, 0 );
                pSd->m_pcDib->TranBlt( pSi->m_pDib, rect, pt );
                rect.OffsetRect( pSd->m_cxIcon, 0 );
            }
    }

    // draw it to the screen
    pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );
}

void CHousingBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    // lines 0 & 1 are damage & materials
    if ( ( m_iConstDone != -1 ) || ( iNum < 1 ) )
    {
        CBuilding::PaintStatusBars( pSi, iNum, pDc );
        return;
    }

    // pop it holds
    pSi->Attach( &theIcons, ICON_PEOPLE );
    CStatData::TYP_ICON ti       = pSi->m_pStatData->m_iTypIcon;
    pSi->m_pStatData->m_iTypIcon = CStatData::done;

    int iPer;
    if ( GetData( )->GetBldgType( ) == CStructureData::apartment )
    {
        if ( theGame.GetMe( )->m_iAptCap <= 0 )
            iPer = 0;
        else
            iPer = ( theGame.GetMe( )->GetPplTotal( ) * 100 ) / theGame.GetMe( )->m_iAptCap;
    }
    else
    {
        if ( theGame.GetMe( )->m_iOfcCap <= 0 )
            iPer = 0;
        else
            iPer = ( theGame.GetMe( )->GetPplBldg( ) * 100 ) / theGame.GetMe( )->m_iOfcCap;
    }
    pSi->SetPer( __min( 100, iPer ) );

    pSi->DrawIcon( pDc );
    pSi->m_pStatData->m_iTypIcon = ti;
}

int CFarmBuilding::GetNumStatusBars( ) const
{

    if ( m_iConstDone != -1 )
        return ( 3 );

    if ( GetData( )->GetType( ) == CStructureData::farm )
        return 2;
    return 3;
}

void CFarmBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    // lumber yard - show materials
    if ( ( m_iConstDone != -1 ) || ( iNum < 1 ) )
    {
        CBuilding::PaintStatusBars( pSi, iNum, pDc );
        return;
    }

    // if not a farm iNum == 1 is materials
    if ( ( iNum == 1 ) && ( GetData( )->GetType( ) != CStructureData::farm ) )
    {
        PaintStatusMaterials( pSi, pDc );
        return;
    }

    // show productivity
    pSi->Attach( &theIcons, ICON_DENSITY );

    pSi->SetPer( m_iTerMult * 10 );

    pSi->DrawIcon( pDc );
}

void CVehicleBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    // lines 0 & 1 are damage & materials
    if ( ( m_iConstDone != -1 ) || ( iNum < 2 ) )
    {
        CBuilding::PaintStatusBars( pSi, iNum, pDc );
        return;
    }

    // what are we building
    pSi->Attach( &theIcons, ICON_BUILD_VEH );

    CBuildUnit const* pBldUnt = GetBldUnt( );
    if ( pBldUnt == NULL )
    {
        pSi->SetPer( 0 );
        pSi->DrawIcon( pDc );
    }
    else

    {
        ASSERT_VALID( pBldUnt );

        if ( ( m_iLastPer == 0 ) && ( m_iBuildDone >= 0 ) )
            pSi->SetPer( 1 );
        else
            pSi->SetPer( __max( 1, m_iLastPer ) );

        // paint it
        pSi->DrawBackground( );
        pSi->DrawStatDone( );

        // put the name up
        CDC* pDcTxt = CDC::FromHandle( pSi->m_pDib->GetDC( ) );
        if ( pDcTxt != NULL )
        {
            pDcTxt->SetBkMode( TRANSPARENT );
            pDcTxt->SetTextColor( CLR_CONST );
            CString sText = theTransports.GetData( pBldUnt->GetVehType( ) )->GetDesc( );
            pDcTxt->DrawText( sText, -1, &( pSi->m_rDest ), DT_CENTER | DT_SINGLELINE | DT_VCENTER );
        }

        // blt the internal bitmap to the screen
        pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );
        if ( pSi->m_pDib->IsBitmapSelected( ) )
            pSi->m_pDib->ReleaseDC( );
    }
}

void CRepairBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    // lines 0 & 1 are damage & materials
    if ( ( m_iConstDone != -1 ) || ( iNum < 2 ) )
    {
        CBuilding::PaintStatusBars( pSi, iNum, pDc );
        return;
    }

    pSi->Attach( &theIcons, ICON_REPAIR_VEH );

    if ( m_pVehRepairing == NULL )
    {
        pSi->SetPer( 0 );
        pSi->DrawIcon( pDc );
    }
    else

    {
        ASSERT_VALID( m_pVehRepairing );

        pSi->SetPer( __max( 1, m_iLastPer ) );

        // paint it
        pSi->DrawBackground( );
        pSi->DrawStatDone( );

        // put the name up
        CDC* pDcTxt = CDC::FromHandle( pSi->m_pDib->GetDC( ) );
        if ( pDcTxt != NULL )
        {
            pDcTxt->SetBkMode( TRANSPARENT );
            pDcTxt->SetTextColor( CLR_CONST );
            CString sText = m_pVehRepairing->GetData( )->GetDesc( );
            pDcTxt->DrawText( sText, -1, &( pSi->m_rDest ), DT_CENTER | DT_SINGLELINE | DT_VCENTER );
        }

        // blt the internal bitmap to the screen
        pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );

        if ( pSi->m_pDib->IsBitmapSelected( ) )
            pSi->m_pDib->ReleaseDC( );
    }
}

void CShipyardBuilding::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    if ( ( m_iMode != repair ) || ( iNum != 2 ) )
    {
        CVehicleBuilding::PaintStatusBars( pSi, iNum, pDc );
        return;
    }
    ASSERT_VALID( m_pVehRepairing );

    pSi->Attach( &theIcons, ICON_REPAIR_VEH );

    if ( m_pVehRepairing == NULL )
    {
        pSi->SetPer( 0 );
        pSi->DrawIcon( pDc );
    }
    else

    {
        ASSERT_VALID( m_pVehRepairing );

        pSi->SetPer( __max( 1, m_iLastPer ) );

        // paint it
        pSi->DrawBackground( );
        pSi->DrawStatDone( );

        // put the name up
        CDC* pDcTxt = CDC::FromHandle( pSi->m_pDib->GetDC( ) );
        if ( pDcTxt != NULL )
        {
            pDcTxt->SetBkMode( TRANSPARENT );
            pDcTxt->SetTextColor( CLR_CONST );
            CString sText = m_pVehRepairing->GetData( )->GetDesc( );
            pDcTxt->DrawText( sText, -1, &( pSi->m_rDest ), DT_CENTER | DT_SINGLELINE | DT_VCENTER );
        }

        // blt the internal bitmap to the screen
        pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );

        if ( pSi->m_pDib->IsBitmapSelected( ) )
            pSi->m_pDib->ReleaseDC( );
    }
}

int CVehicle::GetNumStatusBars( ) const
{

    // freighter show materials & people
    // truck show dest
    if ( GetData( )->IsTransport( ) )
        return ( 3 );

    if ( GetData( )->IsTransport( ) || GetData( )->IsCrane( ) || GetData( )->IsCarrier( ) )
        return ( 2 );

    return ( 1 );
}

void CVehicle::PaintStatusBars( CStatInst* pSi, int iNum, CDC* pDc ) const
{

    if ( iNum == 0 )
    {
        CUnit::PaintStatusBars( pSi, 0, pDc );
        return;
    }

    // show people transport is carrying
    if ( GetData( )->IsCarrier( ) )
        if ( ( ( GetData( )->IsTransport( ) ) && ( iNum == 2 ) ) ||
             ( ( !GetData( )->IsTransport( ) ) && ( iNum == 1 ) ) )
        {
            PaintStatusCarrier( pSi, pDc );
            return;
        }

    // show route for truck
    if ( ( GetData( )->IsTransport( ) ) && ( iNum == 1 ) )
    {
        CString sText;
        if ( !IsHpControl( ) )
            sText = CTransportData::m_sAuto;
        else if ( GetEvent( ) == route )
            sText = CTransportData::m_sRoute;
        CBuilding* pBldg = theBuildingHex.GetBuilding( GetPtHead( ) );
        if ( ( pBldg == NULL ) || ( GetHexOwnership( ) ) )
            pBldg = theBuildingHex.GetBuilding( GetHexDest( ) );
        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) )
            sText += pBldg->GetData( )->GetDesc( );
        else if ( GetRouteMode( ) == stop )
            sText += CTransportData::m_sIdle;
        else
            sText += CTransportData::m_sTravel;
        pSi->Attach( &theIcons, ICON_BAR_TEXT );
        pSi->SetText( sText );
        pSi->DrawIcon( pDc );
        return;
    }

    // what are we building?
    if ( GetData( )->IsCrane( ) )
    {
        pSi->Attach( &theIcons, ICON_CONSTRUCTION );
        pSi->SetPer( 0 );
        if ( GetRouteMode( ) == run )
        {
            if ( ( GetEvent( ) == build ) || ( GetEvent( ) == repair_bldg ) )
            {
                if ( GetConst( ) != NULL )
                    pSi->SetPer( __max( 1, GetConst( )->GetBuildPer( ) ) );
            }
            else if ( GetEvent( ) == build_road )
            {
                pSi->Attach( &theIcons, ICON_BUILD_ROAD );
                pSi->SetPer( __max( 1, GetRoadPer( ) ) );
            }
        }
        pSi->DrawIcon( pDc );
        return;
    }

    // it's materials
    PaintStatusMaterials( pSi, pDc );
}

void CVehicle::PaintStatusCarrier( CStatInst* pSi, CDC* pDc ) const
{

    ASSERT_VALID( this );

    // get the icon
    pSi->Attach( &theIcons, ICON_VEHICLES );

    CStatData* pSd = pSi->m_pStatData;

    // draw the background
    pSi->DrawBackground( );

    // rect to render to
    CRect rect( pSi->m_rDest );
    rect.OffsetRect( -rect.left, -rect.top );
    rect.top += ( rect.Height( ) - pSd->m_cyIcon ) / 2;
    rect.bottom = rect.top + pSd->m_cyIcon;
    rect.left += pSd->m_leftOff;
    rect.right -= pSd->m_rightOff;
    int iRight = rect.right - pSd->m_cxIcon;
    rect.right = rect.left + pSd->m_cxIcon;

    // display them in order
    POSITION pos = m_lstCargo.GetHeadPosition( );
    while ( pos != NULL )
    {
        CVehicle* pVeh = m_lstCargo.GetNext( pos );

        if ( rect.left > iRight )
            break;

        // draw it
        CPoint pt( pVeh->GetData( )->GetType( ) * pSd->m_cxIcon, 0 );
        pSd->m_pcDib->TranBlt( pSi->m_pDib, rect, pt );
        rect.OffsetRect( pSd->m_cxIcon, 0 );
    }

    // draw it to the screen
    pSi->m_pDib->BitBlt( pDc->m_hDC, &( pSi->m_rDest ), CPoint( 0, 0 ) );
}

CUnitData::CUnitData( )
{

    m_iExpRadius = 0;
    memset( m_iRsrchReq, 0, sizeof( m_iRsrchReq ) );
}

BOOL CUnitData::IsDiscovered( ) const
{

    return ( PlyrIsDiscovered( theGame.GetMe( ) ) );
}

BOOL CUnitData::PlyrIsDiscovered( CPlayer* pPlyr ) const
{

    // we only allow units we have art for (shareware versions)
    if ( !( GetUnitFlags( ) & FLhaveArt ) )
        return ( FALSE );

    // we handle scenario limits here too
    if ( theGame.GetScenario( ) != -1 )
    {
        int iScene = theGame.GetScenario( );
        // if it's the AI and we are 5+ minutes into the game we up it one
        if ( pPlyr->IsAI( ) )
            if ( theGame.GetElapsedSeconds( ) > (UINT)( 4 - theGame.m_iAi ) * 5 * 60 )
                iScene++;
        if ( m_iScenario > iScene )
            return ( FALSE );
    }

    for ( int iOn = 0; iOn < 4; iOn++ )
        if ( !pPlyr->GetRsrch( m_iRsrchReq[iOn] ).m_bDiscovered )
            return ( FALSE );

    return ( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// init colors

static void GetClr( CDC& dc, COLORREF& clr, COLORREF clrSrc )
{

    clr = dc.GetNearestColor( clrSrc | 0x02000000 );
}

void InitColors( )
{

    CClientDC dc( &theApp.m_wndMain );
    dc.SelectPalette( CPalette::FromHandle( thePal.hPal( ) ), TRUE );

    GetClr( dc, CUnit::m_clrOk, RGB( 74, 224, 65 ) );
    GetClr( dc, CUnit::m_clrWarn, RGB( 201, 221, 19 ) );
    GetClr( dc, CUnit::m_clrBad, RGB( 235, 51, 41 ) );

    GetClr( dc, EventclrOk, RGB( 0, 0, 0 ) );
    GetClr( dc, EventclrWarn, RGB( 201, 221, 19 ) );
    GetClr( dc, EventclrBad, RGB( 235, 51, 41 ) );
}


/////////////////////////////////////////////////////////////////////////////
// CMaterial - a food, steel, etc.

void CMaterialTypes::ctor( )
{

    CMmio* pMmio = theDataFile.OpenAsMMIO( NULL, "LANG" );

    pMmio->DescendRiff( 'L', 'A', 'N', 'G' );
    pMmio->DescendList( 'M', 'T', 'R', 'L' );

    CString* pStr = m_saDesc;
    for ( int iOn = 0; iOn < GetNumTypes( ); iOn++, pStr++ )
    {
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        pMmio->ReadString( *pStr );
        pMmio->AscendChunk( );
    }
    pMmio->AscendList( );
    delete pMmio;

    CWindowDC dc( NULL );
    m_rgb[0]  = dc.GetNearestColor( PALETTERGB( 230, 154, 108 ) );
    m_rgb[1]  = dc.GetNearestColor( PALETTERGB( 128, 128, 255 ) );
    m_rgb[2]  = dc.GetNearestColor( PALETTERGB( 232, 163, 60 ) );
    m_rgb[3]  = dc.GetNearestColor( PALETTERGB( 198, 98, 103 ) );
    m_rgb[4]  = dc.GetNearestColor( PALETTERGB( 255, 255, 0 ) );
    m_rgb[5]  = dc.GetNearestColor( PALETTERGB( 128, 255, 64 ) );
    m_rgb[6]  = dc.GetNearestColor( PALETTERGB( 130, 105, 87 ) );
    m_rgb[7]  = dc.GetNearestColor( PALETTERGB( 5, 3, 1 ) );
    m_rgb[8]  = dc.GetNearestColor( PALETTERGB( 0, 64, 128 ) );
    m_rgb[9]  = dc.GetNearestColor( PALETTERGB( 125, 21, 21 ) );
    m_rgb[10] = dc.GetNearestColor( PALETTERGB( 255, 255, 255 ) );
}

void CMaterialTypes::dtor( )
{

    CString* pStr = m_saDesc;
    for ( int iOn = 0; iOn < GetNumTypes( ); iOn++, pStr++ )
    {
        pStr->ReleaseBuffer( 0 );
        pStr->FreeExtra( );
    }
}


// CTile

CTile::CTile( TILE_TYPE eTile ): m_byType( BYTE( eTile ) )
{
}

/////////////////////////////////////////////////////////////////////////////
// CUnit - a building or vehicle

CUnit::CUnit( ): m_ptTarget( 0, 0 ), m_ptUs( 0, 0 ), m_ptOppo( 0, 0 ), m_ptUsOppo( 0, 0 )
{

    m_dwID   = 0;
    m_pOwner = NULL;

    m_dwReloadMod = 0;
    m_pUnitTarget = NULL;
    m_pUnitOppo   = NULL;
    m_iLOS = m_iOppoLOS = 0;

    memset( m_aiStore, 0, sizeof( m_aiStore ) );
    memset( m_aiBeforeUpdate, 0, sizeof( m_aiStore ) );

    m_iDamagePoints      = 1000;
    m_iDamagePer         = 100;
    m_iLastShowDamagePer = 100;
    m_fDamageMult        = 1.0f;
    m_fDamPerfMult       = 1.0f;
    m_iDamageThisTurn    = 0;

    m_unitFlags = (UNIT_FLAGS)0;

    memset( m_dwaSpot, 0, sizeof( m_dwaSpot ) );
    m_iVisible = 0;

    m_pdwPlyrsSee = new DWORD[theGame.GetMaxPlyrNum( )];
    memset( m_pdwPlyrsSee, 0, sizeof( DWORD ) * theGame.GetMaxPlyrNum( ) );

    m_iFrameHit     = 0;
    m_iVoice        = -1;
    m_dwLastMatTime = 0;
    m_iUpdateMat    = 0;

    m_bSpotted = FALSE;

#ifdef _DEBUG
    m_Initialized = FALSE;
#endif
}

void CUnit::AddUnit( )
{

    if ( !theApp.AmInGame( ) )
        return;

    memset( m_pdwPlyrsSee, 0, sizeof( DWORD ) * theGame.GetMaxPlyrNum( ) );

    // add held materials from our global totals
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        GetOwner( )->IncMaterialHave( iInd, GetStore( iInd ) );

    if ( GetOwner( )->IsLocal( ) )
        SetOppo( NULL );

    CWndArea* pAreaWnd = theAreaList.GetTop( );
    if ( pAreaWnd != NULL )
    {
        CRect rect;
        pAreaWnd->GetClientRect( &rect );
        CPoint ptBldg = pAreaWnd->GetAA( ).WrapWorldToWindow( CMapLoc3D( GetWorldPixels( ) ) );
        if ( rect.PtInRect( ptBldg ) )
            pAreaWnd->InvalidateSound( );
    }
}

int CUnit::GetVoice( )
{

    if ( m_iVoice != -1 )
        return ( m_iVoice );

    if ( ( theApp.HaveMultVoices( ) ) && ( MyRand( ) & 0x1000 ) )
        m_iVoice = 1;
    else
        m_iVoice = 0;

    return ( m_iVoice );
}

//---------------------------------------------------------------------------
// CUnit::InitSmokeAndFlame
//---------------------------------------------------------------------------
void CUnit::InitSmokeAndFlame( )
{
    ASSERT( m_psprite );

    m_ptrdamagedisplay = new CDamageDisplay( *m_psprite );
}

//---------------------------------------------------------------------------
// CUnit::SetTurret
//---------------------------------------------------------------------------
void CUnit::SetTurret( int iTurretID )
{
    m_ptrturret = new CTurret( iTurretID );
}

void CUnit::AssignData( CUnitData const* pData )
{

    ASSERT_VALID( pData );

    m_pUnitData = pData;

    // this brings in the R&D multipliers
    m_iSpottingRange = GetData( )->_GetSpottingRange( ) +
                       ( GetData( )->_GetSpottingRange( ) >> ( 4 - GetOwner( )->GetSpottingLevel( ) ) );
    m_iRange = GetData( )->_GetRange( ) + ( GetData( )->_GetRange( ) >> ( 4 - GetOwner( )->GetRangeLevel( ) ) );
    for ( int iInd = 0; iInd < CUnitData::num_attacks; iInd++ )
        m_iAttack[iInd] =
            (int)( 0.5f + GetOwner( )->GetAttackMult( ) *
                              (float)( GetData( )->_GetAttack( iInd ) +
                                       ( GetData( )->_GetAttack( iInd ) >> ( 4 - GetOwner( )->GetAttackLevel( ) ) ) ) );
    m_iDefense =
        (int)( 0.5f + GetOwner( )->GetDefenseMult( ) *
                          (float)( GetData( )->_GetDefense( ) +
                                   ( GetData( )->_GetDefense( ) >> ( 4 - GetOwner( )->GetDefenseLevel( ) ) ) ) );
    m_iAccuracy =
        GetData( )->_GetAccuracy( ) - ( GetData( )->_GetAccuracy( ) >> ( 4 - GetOwner( )->GetAccuracyLevel( ) ) );

    m_iFireRate = pData->_GetFireRate( );

    // muzzle flash
    if ( GetData( )->GetUnitFlags( ) & CUnitData::FLflash1 )
    {
        GetAmbient( CSpriteView::ANIM_FRONT_1 )->SetOneShot( TRUE );
        GetAmbient( CSpriteView::ANIM_FRONT_1 )->Pause( TRUE );
    }
    if ( GetData( )->GetUnitFlags( ) & CUnitData::FLflash2 )
    {
        TRAP( );
        GetAmbient( CSpriteView::ANIM_FRONT_2 )->SetOneShot( TRUE );
        GetAmbient( CSpriteView::ANIM_FRONT_2 )->Pause( TRUE );
    }
}

CUnit::~CUnit( )
{

#ifdef _DEBUG
    m_Initialized = FALSE;
#endif

    // pull us off the toolbar and area status lines
    if ( theApp.m_wndBar.m_wndText[1].GetStatusData( ) == this )
        theApp.m_wndBar.m_wndText[1].SetStatusFunc( NULL );

    // if any area windows have us selected kill it
    theAreaList.UnitDying( this );

    // if we are anyone's target its time to take us away
    if ( theApp.AmInGame( ) )
    {
        // track for game over
        if ( GetOwner( )->IsLocal( ) )
        {
            // remove held materials from our global totals
            for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
                GetOwner( )->IncMaterialHave( iInd, -GetStore( iInd ) );
        }

        POSITION pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            ASSERT_STRICT_VALID( pBldg );
            if ( pBldg->m_pUnitTarget == this )
            {
                pBldg->m_pUnitTarget = NULL;
                pBldg->SetOppo( NULL );
            }
            if ( pBldg->m_pUnitOppo == this )
                pBldg->SetOppo( NULL );
        }

        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            ASSERT_STRICT_VALID( pVeh );
            if ( pVeh->m_pUnitTarget == this )
            {
                pVeh->m_pUnitTarget = NULL;
                pVeh->SetEvent( CVehicle::none );
                pVeh->SetRouteMode( CVehicle::stop );
                pVeh->SetOppo( NULL );
            }
            if ( pVeh->m_pUnitOppo == this )
                pVeh->SetOppo( NULL );
        }
    }

    delete[] m_pdwPlyrsSee;
}

void CUnit::RemoveUnit( )
{

    if ( !theApp.AmInGame( ) )
        return;

    // track for game over
    if ( !GetOwner( )->IsLocal( ) )
        return;

    _SetTarget( NULL );

    // remove held materials from our global totals
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        GetOwner( )->IncMaterialHave( iInd, -GetStore( iInd ) );
}

// see if we have enough materials to finish our job
void CUnit::MaterialChange( )
{

    ASSERT_STRICT_VALID( this );

    // update mouse_on window
    CWndArea* pAreaWnd = theAreaList.GetTop( );
    if ( ( pAreaWnd != NULL ) && ( pAreaWnd->GetStaticUnit( ) == this ) )
    {
        CPoint pt;
        ::GetCursorPos( &pt );
        if ( CWnd::WindowFromPoint( pt ) == pAreaWnd->_GetStatBar( ) )
        {
            CString str;
            ShowStatusText( str );
            theApp.m_wndBar.SetStatusText( 1, str );
            theApp.m_wndBar.m_wndText[1].InvalidateRect( NULL );
        }
    }

    // update status bar 1
    theApp.m_wndBar.InvalidateStatus( this );

    // update tooltip windows
    theAreaList.MaterialChange( this );

    if ( !GetOwner( )->IsMe( ) )
        return;

    InvalidateStatus( );
}

void CBuilding::MaterialMessage( )
{

    if ( !GetOwner( )->IsLocal( ) )
        return;

    // tell the AI
    if ( GetOwner( )->IsAI( ) )
    {
        CMsgBldgStat msg( this );
        msg.m_iFlags = CMsgBldgStat::out_mat;
        theGame.PostToClient( GetOwner( ), &msg, sizeof( msg ) );
    }
    else if ( GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgOutMat( this );
}

static int fnEnumHex( CHex* pHex, CHexCoord hex, void* pData )
{

    CBuilding* pUnit = (CBuilding*)pData;

    ASSERT_STRICT_VALID( pHex );
    ASSERT_STRICT_VALID( pUnit );

    if ( !pHex->IsWater( ) )
    {
        pHex->SetType( CHex::city );
        pHex->SetVisibleType( CHex::city );
        pHex->m_psprite = theTerrain.GetSprite( CHex::city, CITY_BUILD_OFF + RandNum( CITY_BUILD_NUM - 1 ) );
    }

    return ( FALSE );
}

// invalidate hexes bldg on and touching
static int fnEnumInv( CHex*, CHexCoord hex, void* )
{

    hex.SetInvalidated( );

    return ( FALSE );
}

void CBuilding::AnimateOperating( BOOL bEnable )
{

    BOOL bUpdate = FALSE;

    if ( GetData( )->GetBldgFlags( ) & CStructureData::FlOperAmb1 )
        if ( GetAmbient( CSpriteView::ANIM_FRONT_1 )->IsEnabled( ) != bEnable )
        {
            bUpdate = TRUE;
            if ( IsLive( ) )
                GetAmbient( CSpriteView::ANIM_FRONT_1 )->Enable( bEnable );
        }

    if ( GetData( )->GetBldgFlags( ) & CStructureData::FlOperAmb2 )
        if ( GetAmbient( CSpriteView::ANIM_FRONT_2 )->IsEnabled( ) != bEnable )
        {
            bUpdate = TRUE;
            if ( IsLive( ) )
                GetAmbient( CSpriteView::ANIM_FRONT_2 )->Enable( bEnable );
        }

    // tell everyone
    if ( bUpdate )
    {
        CMsgBldgStat msg( this );
        msg.m_iFlags = bEnable ? CMsgBldgStat::resumed : CMsgBldgStat::paused;
        theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
    }
}

// makes a building visible on the screen
void CBuilding::MakeBldgVisible( )
{

    ASSERT_STRICT_VALID( this );

    // set all the hexes to this unit
    theMap.EnumHexes( m_hex, GetCX( ), GetCY( ), fnEnumHex, this );

    // invalidate surrounding hexes too (altitude change)
    CHexCoord _hex( m_hex.X( ) - 1, m_hex.Y( ) - 1 );
    _hex.Wrap( );
    theMap.EnumHexes( _hex, GetCX( ) + 2, GetCY( ) + 2, fnEnumInv, this );

    IncVisible( );

    // show it
    SetConstPer( );
    UpdateDamageLevel( );

    if ( ( !IsConstructing( ) ) && ( !IsFlag( stopped ) ) )
    {
        if ( !GetAmbient( CSpriteView::ANIM_FRONT_1 )->IsEnabled( ) )
            EnableAnimations( TRUE );
        PauseAnimations( FALSE );
    }

    SetInvalidated( );

    theApp.m_wndWorld.NewMode( );
}

static int fnEnumSetAlt( CHex* pHex, CHexCoord, void* pData )
{

    pHex->SetAlt( *( (int*)pData ) );

    return ( FALSE );
}

// we pass in the upper left corner in the CTerrain coordinate space
void CBuilding::AssignToHex( CHexCoord hex, int iAlt )
{

    ASSERT_STRICT_VALID_STRUCT( &hex );
    ASSERT_STRICT_VALID( this );

    // save the hex for iDir == 0
    m_hex = hex;

    m_maploc.x = m_hex.X( ) * MAX_HEX_HT + ( GetCX( ) * MAX_HEX_HT ) / 2;
    m_maploc.y = m_hex.Y( ) * MAX_HEX_HT + ( GetCY( ) * MAX_HEX_HT ) / 2;
    m_maploc.Wrap( );

    theBuildingHex.GrabHex( this );

    // set the altitude (all 4 vertices of each hex)
    CHexCoord _hex( hex );
    theMap.EnumHexes( _hex, GetCX( ) + 1, GetCY( ) + 1, fnEnumSetAlt, &iAlt );

    theMap._GetHex( hex )->ClrUnitDir( );  // TESTING

    // assign to the hex's we draw building at for each direction
    theMap._GetHex( hex )->SetUnitDir( 0x01 );
    hex.X( hex.X( ) + GetCX( ) - 1 );
    theMap.GetHex( hex )->SetUnitDir( 0x02 );
    hex.Y( hex.Y( ) + GetCY( ) - 1 );
    theMap.GetHex( hex )->SetUnitDir( 0x04 );
    hex.X( hex.X( ) - ( GetCX( ) - 1 ) );
    theMap.GetHex( hex )->SetUnitDir( 0x08 );
}

// x,y: hex
// ex,ey: bldg size in hexes
// func: call it
// data: user data
void CGameMap::EnumHexes( CHexCoord const& hex, int ex, int ey, FNENUMHEX fnEnum, void* pData )
{

    ASSERT_STRICT_VALID_STRUCT( &hex );
    ASSERT_STRICT_VALID( this );

    CHexCoord _hex( hex );

    for ( int yNum = 0; yNum < ey; yNum++ )
    {
        _hex.m_iX  = hex.m_iX;
        CHex* pHex = _GetHex( _hex );

        for ( int xNum = 0; xNum < ex; xNum++ )
        {
            ASSERT_STRICT_VALID( pHex );
            if ( fnEnum( pHex, _hex, pData ) )
                return;

            _hex.m_iX++;
            if ( _hex.m_iX >= m_eX )
            {
                _hex.m_iX = 0;
                pHex      = theMap.GetHex( _hex );
            }
            else
                pHex = _Xinc( pHex );
        }

        _hex.m_iY++;
        if ( _hex.m_iY >= m_eY )
            _hex.m_iY = 0;
    }
}


//---------------------------------------------------------------------------
// CGameMap::_EnumHexes		Non-wrapping version - GG
//---------------------------------------------------------------------------
void CGameMap::_EnumHexes( CHexCoord const& hex, int ex, int ey, FNENUMHEX fnEnum, void* pData )
{
    ASSERT_STRICT_VALID_STRUCT( &hex );
    ASSERT_STRICT_VALID( this );

    CHexCoord _hex( hex );
    CHex*     pHex = NULL;

    for ( int y = 0; y < ey; ++y )
    {
        _hex.m_iX = hex.m_iX;

        for ( int x = 0; x < ex; ++x )
        {
            pHex = GetHex( _hex );

            ASSERT_STRICT_VALID( pHex );

            if ( fnEnum( pHex, _hex, pData ) )
                return;

            _hex.m_iX++;
        }

        _hex.m_iY++;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CStructure

#ifdef _DEBUG
void CStructureData::ctor( )
{

    m_iType      = (BLDG_TYPE)-1;
    m_iUnionType = (BLDG_UNION_TYPE)-1;
    m_iBldgCat   = -1;
}
#endif

// return TRUE if can build
BOOL CStructureData::CanBuild( CHexCoord const& hex, int iBldgDir, int iBldgTyp, BOOL bShip, BOOL bRocket )
{

    // first we test to see if this building has an exit
    CStructureData const* pData = theStructures.GetData( iBldgTyp );
    if ( ( bShip ) && ( !pData->HasShipExit( ) ) )
        return ( TRUE );

    // the exit hex must be clear
    if ( !bShip )
    {
        CHexCoord _hex;
        int       iExitDir;
        CBuilding::DetermineExit( iBldgDir, iBldgTyp, _hex, iExitDir, FALSE );
        _hex += hex;
        switch ( iExitDir )
        {
        case 0:
            _hex.Ydec( );
            break;
        case 1:
            _hex.Xinc( );
            break;
        case 2:
            _hex.Yinc( );
            break;
        case 3:
            _hex.Xdec( );
            break;
        }
        _hex.Wrap( );

        // must be open
        if ( theBuildingHex._GetBuilding( _hex ) != NULL )
            return ( FALSE );
    }

    // we need to make sure the crane & trucks can get in/out
    //   (diaganols ok for travel but need exit clear)
    if ( ( !bShip ) && ( !pData->HasVehExit( ) ) )
    {
        CHexCoord hexExit;
        int       iExitDir, xInc, yInc;
        CBuilding::DetermineExit( iBldgDir, iBldgTyp, hexExit, iExitDir, bShip );
        hexExit += hex;

        switch ( iExitDir )
        {
        case 0:
            hexExit.Xdec( );
            hexExit.Ydec( );
            xInc = 1;
            yInc = 0;
            break;
        case 1:
            hexExit.Xinc( );
            hexExit.Ydec( );
            xInc = 0;
            yInc = 1;
            break;
        case 2:
            hexExit.Xdec( );
            hexExit.Yinc( );
            xInc = 1;
            yInc = 0;
            break;
        case 3:
            hexExit.Xdec( );
            hexExit.Ydec( );
            xInc = 0;
            yInc = 1;
            break;
        }

        // see if we can handle tracks & wheels
        hexExit.Wrap( );
        BOOL bCrane = FALSE;
        BOOL bTruck = FALSE;
        for ( int iTest = 0; iTest < 3; iTest++ )
        {
            if ( theBuildingHex._GetBuilding( hexExit ) == NULL )
            {
                CHex* pHex = theMap._GetHex( hexExit );
                if ( !( pHex->GetUnits( ) & CHex::bldg ) )
                {
                    if ( theTransports.GetData( CTransportData::construction )->CanTravelHex( pHex ) )
                        bCrane = TRUE;
                    if ( theTransports.GetData( CTransportData::med_truck )->CanTravelHex( pHex ) )
                        bTruck = TRUE;
                }
            }

            hexExit.X( ) += xInc;
            hexExit.Y( ) += yInc;
            hexExit.Wrap( );
        }

        // if they can get in/out we're done
        if ( ( !bCrane ) || ( !bTruck ) )
            return ( FALSE );
        return ( TRUE );
    }

    // see if all vehicles can travel the direct exit
    CHexCoord hexExit;
    int       iExitDir;
    CBuilding::DetermineExit( iBldgDir, iBldgTyp, hexExit, iExitDir, bShip );
    hexExit += hex;
    CHexCoord hexRoad( hexExit );

    // get the hex outside it's door
    switch ( iExitDir )
    {
    case 0:
        hexExit.Ydec( );
        break;
    case 1:
        hexExit.Xinc( );
        break;
    case 2:
        hexExit.Yinc( );
        break;
    case 3:
        hexExit.Xdec( );
        break;
    }

    if ( bShip )
    {
        CHex* pHex = theMap._GetHex( hexExit );

        // building in the way?
        if ( pHex->GetUnits( ) & CHex::bldg )
            return ( FALSE );

        // can't be coastline
        if ( pHex->GetType( ) == CHex::coastline )
            return ( FALSE );

        // can it handle ships?
        if ( !theTransports.GetData( CTransportData::landing_craft )->CanTravelHex( pHex ) )
            return ( FALSE );

        return ( TRUE );
    }

    // for land vehicles we have to check around the road hex we drop out
    for ( int x = -1; x <= 1; x++ )
        for ( int y = -1; y <= 1; y++ )
            if ( abs( x ) + abs( y ) == 1 )
            {
                CHexCoord _hexTest( hexExit.X( ) + x, hexExit.Y( ) + y );
                _hexTest.Wrap( );
                if ( _hexTest != hexRoad )
                {

                    CHex* pHex = theMap._GetHex( _hexTest );
                    // building in the way?
                    if ( pHex->GetUnits( ) & CHex::bldg )
                        continue;
                    // can't be coastline
                    if ( pHex->GetType( ) == CHex::coastline )
                        continue;
                    if ( !theTransports.GetData( CTransportData::construction )->CanTravelHex( pHex ) )
                        continue;
                    if ( !theTransports.GetData( CTransportData::heavy_truck )->CanTravelHex( pHex ) )
                        continue;

                    // rocket - everything
                    if ( pData->GetType( ) == CStructureData::rocket )
                    {
                        if ( !theTransports.GetData( CTransportData::light_scout )->CanTravelHex( pHex ) )
                            continue;
                        if ( !theTransports.GetData( CTransportData::infantry )->CanTravelHex( pHex ) )
                            continue;
                    }

                    // repair - need hovercraft too
                    if ( pData->GetUnionType( ) == CStructureData::UTrepair )
                        if ( !theTransports.GetData( CTransportData::light_scout )->CanTravelHex( pHex ) )
                            continue;

                    // vehicle factory - everything it makes
                    if ( pData->GetUnionType( ) == CStructureData::UTvehicle )
                    {
                        CBuildVehicle const* pBv = pData->GetBldVehicle( );
                        for ( int iOn = 0, iInd = 0; iOn < pBv->GetSize( ); iOn++ )
                        {
                            CTransportData const* pTd = theTransports.GetData( pBv->GetUnit( iOn )->GetVehType( ) );
                            if ( !pTd->CanTravelHex( pHex ) )
                                goto NextLine;
                        }
                        // got it
                        return ( TRUE );
                    NextLine:
                        continue;
                    }

                    // got it
                    return ( TRUE );
                }
            }

    return ( FALSE );
}

CStructure::CStructure( char const* pszRifName ): CSpriteStore<CStructureSprite>( pszRifName )
{

    m_ppData                 = NULL;
    m_iTallestBuildingHeight = -1;
}

CStructure::~CStructure( )
{
    Close( );
}

//---------------------------------------------------------------------------
// CStructure::GetTallestBuildingHeight
//---------------------------------------------------------------------------
int CStructure::GetTallestBuildingHeight( int iZoom )
{
    if ( -1 == m_iTallestBuildingHeight )
        for ( int i = 0; i < GetCount( ); ++i )
        {
            CStructureSprite* psprite = GetSpriteByIndex( i );

            for ( int j = 0; j < psprite->GetNumViews( ); ++j )
            {
                CSpriteView* pspriteview = psprite->GetSpriteView( j );
                CRect        rect( pspriteview->Rect( theApp.GetZoomData( )->GetFirstZoom( ) ) );

                m_iTallestBuildingHeight = Max( m_iTallestBuildingHeight, rect.Height( ) );
            }
        }

    ASSERT( 0 < m_iTallestBuildingHeight );

    // was failing?
   // ASSERT( iZoom >= theApp.GetZoomData( )->GetFirstZoom( ) );

    return m_iTallestBuildingHeight >> iZoom - theApp.GetZoomData( )->GetFirstZoom( );
}

void CStructure::Close( )
{

    ASSERT_STRICT_VALID( this );

    CSpriteStore<CStructureSprite>::Close( );

    if ( m_ppData != NULL )
    {
        CStructureData** ppSd = m_ppData;
        for ( int iOn = 0; iOn < theStructures.GetNumBuildings( ); iOn++, ppSd++ ) delete *ppSd;
        delete[] m_ppData;
        m_ppData = NULL;
    }
}

int CStructure::GetIndex( CStructureData const* pData ) const
{

    for ( int iInd = 0; iInd < m_iNumBuildings; iInd++ )
        if ( *( m_ppData + iInd ) == pData )
            return ( iInd );

    ASSERT( FALSE );
    return ( 0 );
}


CBuildVehicle::CBuildVehicle( ): CStructureData( CStructureData::UTvehicle )
{

    for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ ) m_aiTotalInput[iOn] = 0;
}

CBuildVehicle::~CBuildVehicle( )
{

    for ( int iInd = 0; iInd < m_aBldUnits.GetSize( ); iInd++ )
    {
        CBuildUnit* pBldUnt = m_aBldUnits[iInd];
        if ( pBldUnt != NULL )
        {
            m_aBldUnits[iInd] = NULL;
            delete pBldUnt;
        }
    }
}

CBuildRepair::CBuildRepair( ): CStructureData( CStructureData::UTrepair )
{

    for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ ) m_aiTotalRepair[iOn] = 0;
}

CBuildRepair::~CBuildRepair( )
{

    for ( int iInd = 0; iInd < m_aRprUnits.GetSize( ); iInd++ )
    {
        CBuildUnit* pBldUnt = m_aRprUnits[iInd];
        if ( pBldUnt != NULL )
        {
            m_aRprUnits[iInd] = NULL;
            delete pBldUnt;
        }
    }
}

CBuildShipyard::CBuildShipyard( ): CBuildVehicle( )
{

    m_iUnionType = CStructureData::UTshipyard;
}

CBuildShipyard::~CBuildShipyard( )
{
}

/////////////////////////////////////////////////////////////////////////////
// CBuilding - a building

void CBuilding::ctor( )
{
    m_iUnitType = CUnit::building;

    m_fOperMod          = 0.0f;
    m_iBldgDir          = 0;
    m_iConstDone        = 0;
    m_iBuildDone        = 0;
    m_iLastPer          = 0;
    m_iFoundTime        = 0;
    m_iFoundPer         = 0;
    m_iSkltnPer         = 0;
    m_iFinalPer         = 0;
    m_iLastMaterialTake = 0;
    memset( &m_aiRepair[0], 0, sizeof( m_aiRepair ) );
    m_iRepairWork = 0;
    m_iRepairMod  = 0;
    m_iMatTo      = 0;

    m_iVisConstDone = 0;
    m_iVisFoundPer  = 0;
    m_iVisSkltnPer  = 0;
    m_iVisFinalPer  = 0;
}

CBuilding::CBuilding( int iBldg, int iBldgDir, int iOwner, DWORD dwID ): m_hex( 0, 0 )
{

    ctor( );
    m_pOwner = theGame.GetPlayerByPlyr( iOwner );
    GetFlag( )->Init( m_pOwner->GetPlyrNum( ) );
    ASSERT( dwID != 0 );
    m_dwID = dwID;

    m_iVisible = 0;

#ifdef _CHEAT
    if ( theApp.GetProfileInt( "Cheat", "SeeAll", 0 ) )
        m_iVisible = 1;
#endif

    ASSERT_STRICT( ( 0 < iBldg ) && ( iBldg <= theStructures.m_iNumBuildings ) );

    m_psprite = theStructures.GetSprite( iBldg );
    AssignData( theStructures.GetData( iBldg ) );

    InitSmokeAndFlame( );

    // set it's direction size
    m_iBldgDir = iBldgDir;
    if ( iBldgDir & 1 )
    {
        m_cx = GetData( )->GetCY( );
        m_cy = GetData( )->GetCX( );
    }
    else
    {
        m_cx = GetData( )->GetCX( );
        m_cy = GetData( )->GetCY( );
    }

    m_iDamagePoints = m_pUnitData->GetDamagePoints( );

    m_maploc.x = ( m_cx * MAX_HEX_HT ) / 2;
    m_maploc.y = ( m_cy * MAX_HEX_HT ) / 2;
    m_maploc.Wrap( );

    EnableAnimations( FALSE );

    ASSERT_STRICT_VALID( this );
}

void CBuilding::AddUnit( )
{

    CUnit::AddUnit( );

    // track buildings
    GetOwner( )->AddBldgsHave( 1 );
    GetOwner( )->AddExists( GetData( )->GetType( ), 1 );

    if ( !theApp.AmInGame( ) )
        return;

    // spotting
    if ( m_pOwner->IsMe( ) || ( m_pOwner->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
    {
        DetermineSpotting( );
        IncrementSpotting( m_hex );
    }

    if ( !GetOwner( )->IsLocal( ) )
        return;

    if ( GetOwner( )->IsAI( ) )
    {
        CMsgPlaceBldg msg( GetHex( ), GetDir( ), GetData( )->GetType( ) );
        msg.m_iPlyrNum = GetOwner( )->GetPlyrNum( );
        theGame.PostToAllAi( &msg, sizeof( msg ) );
    }
    else

    {
        MakeBldgVisible( );
        theApp.m_wndBldgs.AddToList( this );
        theGame.m_pHpRtr->MsgNewBldg( this );

        // if first research facility - put R&D window up
        if ( GetOwner( )->IsMe( ) )
            ResearchDiscovered( 0 );
        if ( ( GetData( )->GetType( ) == CStructureData::research ) &&
             ( GetOwner( )->GetExists( CStructureData::research ) == 1 ) )
            theApp.m_wndBar._GotoScience( );

        // if command center turn on the radar
        if ( ( GetData( )->GetType( ) == CStructureData::command_center ) &&
             ( GetOwner( )->GetExists( CStructureData::command_center ) == 1 ) )
        {
            theApp.m_wndWorld.CommandCenterChange( );
            theGame.Event( EVENT_HAVE_RADAR, EVENT_NOTIFY );
        }

        // if embassy bring up relations
        if ( ( GetData( )->GetType( ) == CStructureData::embassy ) &&
             ( GetOwner( )->GetExists( CStructureData::embassy ) == 1 ) )
            theApp.m_wndBar.GotoRelations( );
    }

    // get it going
    EventOff( );

    // update oppo fire
    OppoAndOthers( );
}

int fnEnumHex2( CHex* pHex, CHexCoord _hex, void* )
{

    ASSERT_STRICT_VALID( pHex );

    if ( !pHex->IsWater( ) )
    {
        pHex->SetType( CHex::city );
        pHex->SetVisibleType( CHex::city );
        pHex->m_psprite = theTerrain.GetSprite( CHex::city, CITY_DESTROYED_OFF + RandNum( CITY_DESTROYED_NUM - 1 ) );
    }

    pHex->ClrUnitDir( );

    // redraw it
    _hex.SetInvalidated( );

    return ( FALSE );
}

CBuilding::~CBuilding( )
{

#ifdef _DEBUG
    m_Initialized = FALSE;
#endif

    // if the game is over we don't have to track this stuff
    if ( theApp.AmInGame( ) )
    {
        if ( !IsFlag( CUnit::dead ) )
            GetOwner( )->AddBldgsHave( -1 );

        // remove from # total
        GetOwner( )->AddPplBldg( -( GetOwner( )->GetPplMult( ) * GetData( )->GetPeople( ) ) );

        // scenario 4 & 5 - we need to undo the extra visibility
        if ( ( theGame.GetScenario( ) == 4 ) || ( theGame.GetScenario( ) == 5 ) )
        {
            for ( int iInd = 0; iInd < 5; iInd++ )
                if ( theGame.m_adwScenarioUnits[iInd] == GetID( ) )
                {
                    if ( theGame.GetScenario( ) == 4 )
                        theGame.m_adwScenarioUnits[iInd] = 0;
                    else
                    {
                        TRAP( iInd == 0 );
                        TRAP( iInd != 0 );
                        theGame.m_adwScenarioUnits[iInd] = theGame.m_adwScenarioUnits[0];
                        theGame.m_adwScenarioUnits[0]    = 0;
                    }
                    UnshowBuilding( this );
                }
        }

        // mark as not built
        if ( m_iConstDone == -1 )
            GetOwner( )->AddExists( GetData( )->GetType( ), -1 );

        // scenario stuff
        if ( GetOwner( )->IsAI( ) && ( theGame.GetScenario( ) != -1 ) )
            switch ( GetData( )->GetType( ) )
            {
            // if it's a rocket - track it for scenario 8
            case CStructureData::rocket:
                if ( theGame.GetScenario( ) == 8 )
                    theGame.m_iScenarioVar++;
                break;

            // if it's a refinery or power plant track - see if game over for scenario 10
            case CStructureData::power:
            case CStructureData::refinery:
                if ( theGame.GetScenario( ) == 10 )
                {
                    // unshow around it
                    if ( GetOwner( )->IsAI( ) )
                        UnshowBuilding( this );

                    // we walk all AI players to see if one has no refinery & no power plants
                    BOOL     bNoRef = FALSE, bNoPwr = FALSE;
                    POSITION pos;
                    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
                    {
                        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                        if ( pPlr->IsAI( ) )
                        {
                            if ( !pPlr->GetExists( CStructureData::refinery ) )
                                bNoRef = TRUE;
                            if ( ( !pPlr->GetExists( CStructureData::power_1 ) ) &&
                                 ( !pPlr->GetExists( CStructureData::power_2 ) ) &&
                                 ( !pPlr->GetExists( CStructureData::power_3 ) ) )
                                bNoPwr = TRUE;
                        }
                    }
                    if ( bNoRef && bNoPwr )
                        theGame.m_iScenarioVar = TRUE;
                }
                break;
            }

        // scenario stuff
        switch ( GetData( )->GetType( ) )
        {
        case CStructureData::rocket:
            GetOwner( )->m_iAptCap -= ROCKET_APT_CAP;
            GetOwner( )->m_iOfcCap -= ROCKET_OFC_CAP;
            GetOwner( )->m_dwIDRocket = 0;
            break;

        // lose farm or refinery - take out of reserve
        case CStructureData::farm: {
            if ( ( !GetOwner( )->IsLocal( ) ) || ( m_iConstDone != -1 ) )
                break;
            int iLoss = GetOwner( )->GetFood( ) - GetOwner( )->GetFoodNeed( ) * 5;
            if ( iLoss > 0 )
            {
                if ( GetOwner( )->GetExists( CStructureData::farm ) >= 0 )
                    iLoss /= ( 1 + GetOwner( )->GetExists( CStructureData::farm ) );
                GetOwner( )->AddFood( -iLoss );
            }
            break;
        }

        case CStructureData::refinery: {
            if ( ( !GetOwner( )->IsLocal( ) ) || ( m_iConstDone != -1 ) )
                break;
            int iLoss =
                GetOwner( )->GetGasHave( ) - __max( 5 * GetOwner( )->GetVehsHave( ), GetOwner( )->GetGasNeed( ) ) * 5;
            if ( iLoss > 0 )
            {
                if ( GetOwner( )->GetExists( CStructureData::refinery ) >= 0 )
                    iLoss /= ( 1 + GetOwner( )->GetExists( CStructureData::refinery ) );
                GetOwner( )->AddGas( -iLoss );
            }
            break;
        }

        // if we lost an apartment or office - track it
        case CStructureData::apartment:
            if ( m_iConstDone == -1 )
                GetOwner( )->m_iAptCap -= GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
            break;

        case CStructureData::office:
            if ( m_iConstDone == -1 )
                GetOwner( )->m_iOfcCap -= GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
            break;
        }

        if ( GetOwner( )->IsMe( ) )
            theGame.m_pHpRtr->MsgDeleteUnit( this );

        // set as not built
        if ( GetOwner( )->IsMe( ) )
        {
            theApp.m_wndBar.CheckButtons( );

            // may reduce R&D options
            ResearchDiscovered( 0 );

            // if command center turn off the radar
            if ( GetData( )->GetType( ) == CStructureData::command_center )
                theApp.m_wndWorld.CommandCenterChange( );

            // if our last embassy switch all better relations to neutral
            if ( ( GetData( )->GetType( ) == CStructureData::embassy ) &&
                 ( theGame.GetMe( )->GetExists( CStructureData::embassy ) <= 0 ) )
            {
                for ( POSITION pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
                {
                    CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                    if ( ( !pPlr->IsMe( ) ) && ( pPlr->GetRelations( ) <= RELATIONS_PEACE ) )
                    {
                        pPlr->SetRelations( RELATIONS_NEUTRAL );
                        CMsgSetRelations msg( theGame.GetMe( ), pPlr, RELATIONS_NEUTRAL );
                        theGame.PostToClient( pPlr->GetPlyrNum( ), &msg, sizeof( msg ) );
                    }
                }
            }

            // if it was our last R&D center - kill the R&D window
            if ( ( GetData( )->GetBldgType( ) == CStructureData::research ) && ( theApp.m_pdlgRsrch != NULL ) )
                if ( theGame.GetMe( )->GetExists( CStructureData::research ) <= 0 )
                {
                    theApp.m_pdlgRsrch->DestroyWindow( );
                    theApp.m_pdlgRsrch = NULL;
                }

            // if it was our last embassy - kill the relations window
            if ( ( GetData( )->GetBldgType( ) == CStructureData::embassy ) && ( theApp.m_pdlgRelations != NULL ) )
                if ( theGame.GetMe( )->GetExists( CStructureData::embassy ) <= 0 )
                {
                    theApp.m_pdlgRelations->DestroyWindow( );
                    theApp.m_pdlgRelations = NULL;
                }
        }

        // if any vehicles are in us - kill them (except 1 crane)
        POSITION pos    = theVehicleMap.GetStartPosition( );
        BOOL     bCrane = FALSE;
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            ASSERT_STRICT_VALID( pVeh );
            if ( !pVeh->GetHexOwnership( ) )
            {
                int xDif = CHexCoord::Diff( pVeh->GetHexHead( ).X( ) - m_hex.X( ) );
                if ( ( 0 <= xDif ) && ( xDif < m_cx ) )
                {
                    int yDif = CHexCoord::Diff( pVeh->GetHexHead( ).Y( ) - m_hex.Y( ) );
                    if ( ( 0 <= yDif ) && ( yDif < m_cy ) )
                    {
                        if ( ( !bCrane ) && ( pVeh->GetData( )->GetType( ) == CTransportData::construction ) )
                        {
                            CVehicle::StopConstruction( this );
                            bCrane = TRUE;
                        }
                        else
                            pVeh->PrepareToDie( GetKiller( ) );
                    }
                }
            }
        }

        // ok, if it's me and we drop below 3 buildings OR 5 buildings and no rocket - we're dead
        if ( ( GetOwner( )->IsMe( ) ) &&
             ( ( GetOwner( )->GetBldgsHave( ) < 3 ) ||
               ( ( GetOwner( )->GetBldgsHave( ) < 5 ) && ( GetOwner( )->m_dwIDRocket == 0 ) ) ) )
        {
            // find all of our buildings
            POSITION pos = theBuildingMap.GetStartPosition( );
            while ( pos != NULL )
            {
                DWORD      dwID;
                CBuilding* pBldg;
                theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
                if ( ( pBldg != this ) && ( pBldg->GetOwner( ) == GetOwner( ) ) )
                {
                    CMsgSeeUnit msg;
                    msg.m_dwIDme       = 0;
                    msg.m_hexMe        = CHexCoord( 0, 0 );
                    msg.m_dwIDspot     = pBldg->GetID( );
                    msg.m_iPlyrNumSpot = pBldg->GetOwner( )->GetPlyrNum( );
                    msg.m_hexSpot      = pBldg->GetHex( );

                    // tell the AI players
                    for ( POSITION pos = theGame.GetAi( ).GetHeadPosition( ); pos != NULL; )
                    {
                        CPlayer* pPlr    = theGame.GetAi( ).GetNext( pos );
                        msg.m_iPlyrNumMe = pPlr->GetPlyrNum( );
                        theGame.PostToClient( pPlr, &msg, sizeof( msg ) );
                    }
                }
            }
        }

        // we can't see anything anymore
        if ( SpottingOn( ) )
            DecrementSpotting( );
    }  // in game

    theMap.EnumHexes( m_hex, m_cx, m_cy, fnEnumHex2, NULL );
    theBuildingHex.ReleaseHex( this );
    theBuildingMap.Remove( this );

    // if any vehicles are building this - stop it
    CVehicle::StopConstruction( this );

    DestroyAllWindows( );

    if ( theApp.m_wndBldgs.m_hWnd != NULL )
    {
        int iInd = theApp.m_wndBldgs.FindItem( this );
        if ( iInd >= 0 )
        {
            // Win32s bug
            if ( ( iInd == 0 ) && ( iWinType == W32s ) )
            {
                theApp.m_wndBldgs.m_ListBox.SetItemData( iInd, NULL );
                theApp.m_wndBldgs.m_ListBox.SetCurSel( -1 );
                theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL );
            }
            else
                theApp.m_wndBldgs.m_ListBox.DeleteString( iInd );

            // if we're done, we can delete this guy now
            if ( iWinType == W32s )
                if ( theApp.m_wndBldgs.m_ListBox.GetCount( ) == 1 )
                    if ( theApp.m_wndBldgs.m_ListBox.GetItemData( 0 ) == NULL )
                        theApp.m_wndBldgs.m_ListBox.ResetContent( );
        }
    }

    // remove from world map
    if ( theApp.AmInGame( ) )
        theApp.m_wndWorld.NewMode( );
}

void CBuilding::RemoveUnit( )
{

    CUnit::RemoveUnit( );

    // track buildings
    GetOwner( )->AddBldgsHave( -1 );
    GetOwner( )->AddExists( GetData( )->GetType( ), -1 );

    if ( !theApp.AmInGame( ) )
        return;

    // we can't see anything anymore (may be an alliance unit)
    if ( SpottingOn( ) )
        DecrementSpotting( );

    if ( !GetOwner( )->IsLocal( ) )
        return;

    // remove from # total
    GetOwner( )->AddPplBldg( -( GetOwner( )->GetPplMult( ) * GetData( )->GetPeople( ) ) );

    // mark as not built
    if ( m_iConstDone == -1 )
        GetOwner( )->AddExists( GetData( )->GetType( ), -1 );

    switch ( GetData( )->GetType( ) )
    {
    // if we lost an apartment or office - track it
    case CStructureData::apartment:
        GetOwner( )->m_iAptCap -= GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
        break;
    case CStructureData::office:
        GetOwner( )->m_iOfcCap -= GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
        break;
    }

    // set as not built
    if ( GetOwner( )->IsMe( ) )
    {
        theApp.m_wndBar.CheckButtons( );

        // may reduce R&D options
        ResearchDiscovered( 0 );

        // if command center turn off the radar
        if ( GetData( )->GetType( ) == CStructureData::command_center )
            theApp.m_wndWorld.CommandCenterChange( );

        // if our last embassy switch all better relations to neutral
        if ( ( GetData( )->GetType( ) == CStructureData::embassy ) &&
             ( theGame.GetMe( )->GetExists( CStructureData::embassy ) <= 0 ) )
        {
            for ( POSITION pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
            {
                CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                if ( ( !pPlr->IsMe( ) ) && ( pPlr->GetRelations( ) <= RELATIONS_PEACE ) )
                {
                    pPlr->SetRelations( RELATIONS_NEUTRAL );
                    CMsgSetRelations msg( theGame.GetMe( ), pPlr, RELATIONS_NEUTRAL );
                    theGame.PostToClient( pPlr->GetPlyrNum( ), &msg, sizeof( msg ) );
                }
            }
            if ( theApp.m_pdlgRelations != NULL )
            {
                theApp.m_pdlgRelations->DestroyWindow( );
                theApp.m_pdlgRelations = NULL;
            }
        }

        // if it was our last R&D center - kill the R&D window
        if ( ( GetData( )->GetBldgType( ) == CStructureData::research ) && ( theApp.m_pdlgRsrch != NULL ) )
            if ( theGame.GetMe( )->GetExists( CStructureData::research ) <= 0 )
            {
                theApp.m_pdlgRsrch->DestroyWindow( );
                theApp.m_pdlgRsrch = NULL;
            }

        DestroyAllWindows( );

        // remove from listbox
        if ( theApp.m_wndBldgs.m_hWnd != NULL )
        {
            int iInd = theApp.m_wndBldgs.FindItem( this );
            if ( iInd >= 0 )
            {
                // Win32s bug
                if ( ( iInd == 0 ) && ( iWinType == W32s ) )
                {
                    theApp.m_wndBldgs.m_ListBox.SetItemData( iInd, NULL );
                    theApp.m_wndBldgs.m_ListBox.SetCurSel( -1 );
                    theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL );
                }
                else
                    theApp.m_wndBldgs.m_ListBox.DeleteString( iInd );

                // if we're done, we can delete this guy now
                if ( iWinType == W32s )
                    if ( theApp.m_wndBldgs.m_ListBox.GetCount( ) == 1 )
                        if ( theApp.m_wndBldgs.m_ListBox.GetItemData( 0 ) == NULL )
                            theApp.m_wndBldgs.m_ListBox.ResetContent( );
            }
        }
    }  // IsMe

    // tell the router goodbye
    if ( GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgDeleteUnit( this );

    // remove from world map
    theApp.m_wndWorld.NewMode( );
}

// make an instance of a building
CBuilding* CBuilding::Alloc( int iBldg, int iBldgDir, int iOwner, DWORD ID )
{

    CBuilding* pBldg;
    switch ( theStructures.GetData( iBldg )->GetUnionType( ) )
    {
    case CStructureData::UTmaterials:
        pBldg = new CMaterialBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTvehicle:
        pBldg = new CVehicleBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTshipyard:
        pBldg = new CShipyardBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTpower:
        pBldg = new CPowerBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTrepair:
        pBldg = new CRepairBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTwarehouse:
        pBldg = new CWarehouseBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTmine:
        pBldg = new CMineBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    case CStructureData::UTfarm:
        pBldg = new CFarmBuilding( iBldg, iBldgDir, iOwner, ID );
        break;

    case CStructureData::UThousing:
        pBldg = new CHousingBuilding( iBldg, iBldgDir, iOwner, ID );
        break;

    case CStructureData::UTcommand:
    case CStructureData::UTembassy:
    case CStructureData::UTfort:
    case CStructureData::UTresearch:
        pBldg = new CBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    default:
        ASSERT( FALSE );
        pBldg = new CBuilding( iBldg, iBldgDir, iOwner, ID );
        break;
    }

    return ( pBldg );
}

void CBuilding::DropUnits( int iVehTyp, int iWheelTyp, int iNum, int iNumSkip, int* piTime, int iBlkSiz,
                           CHexCoord const& hex, CHexCoord const& hexUL )
{

    // set distance on each
    int*      piOn = piTime;
    CHexCoord hexDest( hexUL );
    CHexCoord hexSrc( GetExit( CWheelTypes::wheel ) );

    switch ( GetExitDir( ) )
    {
    case 0:
        hexSrc.Ydec( );
        break;
    case 1:
        hexSrc.Xinc( );
        break;
    case 2:
        hexSrc.Yinc( );
        break;
    case 3:
        hexSrc.Xdec( );
        break;
    }

    for ( int x = 0; x < iBlkSiz; x++ )
    {
        for ( int y = 0; y < iBlkSiz; y++ )
        {
            CHex* pHex = theMap._GetHex( hexDest );
            if ( ( *piOn >= 0 ) && ( !( pHex->GetUnits( ) & CHex::bldg ) ) && ( MyRand( ) & 0x70 ) )
            {
                // rough - cost is distance times dest terrain
                int iCost = theTerrain.GetData( pHex->GetType( ) ).GetWheelMult( iWheelTyp );
                *piOn     = iCost * CHexCoord::Dist( hexSrc, hexDest );
            }
            piOn++;
            hexDest.Yinc( );
        }
        hexDest.Xinc( );
        hexDest.Y( ) = hexUL.Y( );
    }

    // skip the ones needed for later
    while ( iNumSkip-- )
    {
        int       iFastest  = -1;
        int*      piOn      = piTime;
        int*      piFastest = piTime;
        CHexCoord hexDest;
        for ( int x = 0; x < iBlkSiz; x++ )
            for ( int y = 0; y < iBlkSiz; y++ )
            {
                if ( ( *piOn > 0 ) && ( ( iFastest == -1 ) || ( ( *piOn != -1 ) && ( *piOn < iFastest ) ) ) )
                {
                    iFastest  = *piOn;
                    piFastest = piOn;
                }
                piOn++;
            }

        if ( iFastest == -1 )
            break;
        else
            *piFastest = 0;
    }

    // find the shortest distance we need to go
    while ( iNum-- )
    {
        int       iFastest  = -1;
        int*      piOn      = piTime;
        int*      piFastest = piTime;
        CHexCoord hexDest;
        for ( int x = 0; x < iBlkSiz; x++ )
            for ( int y = 0; y < iBlkSiz; y++ )
            {
                if ( ( *piOn > 0 ) && ( ( iFastest == -1 ) || ( ( *piOn != -1 ) && ( *piOn < iFastest ) ) ) )
                {
                    iFastest  = *piOn;
                    piFastest = piOn;
                    hexDest.X( hexUL.X( ) + x );
                    hexDest.Y( hexUL.Y( ) + y );
                }
                piOn++;
            }

        if ( iFastest == -1 )
        {
            hexDest.X( ) = hexUL.X( ) - RandNum( 4 );
            hexDest.Y( ) = hexUL.Y( ) - RandNum( 4 );
            hexDest.Wrap( );
        }
        else
            *piFastest = -1;

        CMsgPlaceVeh msg( this, hexDest, -1, iVehTyp );
        theGame.PostToServer( &msg, sizeof( msg ) );
        GetOwner( )->_DecVehsBuilt( );
    }
}

void CBuilding::DetermineExit( int iBldgDir, int iBldgTyp, CHexCoord& hexExit, int& iExitDir, BOOL bShip )
{

    CStructureData const* pData = theStructures.GetData( iBldgTyp );
    ASSERT_VALID( pData );

    CHexCoord _hex;
    if ( bShip )
    {
        iExitDir = ( pData->GetShipDir( ) - iBldgDir ) & 0x03;
        _hex     = pData->GetShipHex( );
    }
    else
    {
        iExitDir = ( pData->GetExitDir( ) - iBldgDir ) & 0x03;
        _hex     = pData->GetExitHex( );
    }

    switch ( iBldgDir )
    {
    case 0:
        hexExit = _hex;
        break;
    case 1:
        hexExit.X( ) = _hex.Y( );
        hexExit.Y( ) = pData->GetCX( ) - _hex.X( ) - 1;
        break;
    case 2:
        hexExit.X( ) = pData->GetCX( ) - _hex.X( ) - 1;
        hexExit.Y( ) = pData->GetCY( ) - _hex.Y( ) - 1;
        break;
    case 3:
        hexExit.X( ) = pData->GetCY( ) - _hex.Y( ) - 1;
        hexExit.Y( ) = _hex.X( );
        break;

#ifdef _DEBUG
    default:
        ASSERT( FALSE );
        break;
#endif
    }

#ifndef _GG
    ASSERT( ( ( hexExit.X( ) == 0 ) && ( iExitDir == 3 ) ) ||
            ( ( hexExit.X( ) == ( ( iBldgDir & 1 ) ? pData->GetCY( ) : pData->GetCX( ) ) - 1 ) && ( iExitDir == 1 ) ) ||
            ( ( hexExit.Y( ) == 0 ) && ( iExitDir == 0 ) ) ||
            ( ( hexExit.Y( ) == ( ( iBldgDir & 1 ) ? pData->GetCX( ) : pData->GetCY( ) ) - 1 ) && ( iExitDir == 2 ) ) );
#endif
}

static int fnEnumDrop( CHex* pHex, CHexCoord, void* )
{

    if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
        pHex->SetOceanAlt( CTransportData::GetMaxDraft( ) );

    return ( FALSE );
}

static void FindChannel( CHexCoord& _hex )
{

    // walk in a spiral - we will try 8 * 8 = 64 hexes.
    CHexCoord _hexOn( _hex );
    int       iTry      = theGame.GetSideSize( ) * theGame.GetSideSize( );
    int       cx        = 1;
    int       iNum      = 1;
    int       iDir      = 0;
    int       iMaxFound = CHex::sea_level;
    CHexCoord hexFound;
    CHex*     pHex;

    while ( iTry-- > 0 )
    {
        switch ( iDir )
        {
        case 0:  // move right
            iNum--;
            _hexOn.Xinc( );
            if ( iNum <= 0 )
            {
                iDir = 1;
                iNum = cx;
            }
            break;
        case 1:  // move down
            iNum--;
            _hexOn.Yinc( );
            if ( iNum <= 0 )
            {
                iDir = 2;
                iNum = cx;
            }
            break;
        case 2:  // move left
            iNum--;
            _hexOn.Xdec( );
            if ( iNum <= 0 )
            {
                iDir = 3;
                iNum = cx;
            }
            break;
        case 3:  // move up
            iNum--;
            _hexOn.Ydec( );
            if ( iNum <= 0 )
            {
                iDir = 0;
                cx += 2;
                iNum = cx;
                _hexOn.Xdec( );
                _hexOn.Ydec( );
            }
            break;
        }

        // lets check it
        pHex = theMap._GetHex( _hexOn );
        if ( pHex->GetAlt( ) > CTransportData::GetMaxDraft( ) )
        {
            if ( pHex->GetAlt( ) < iMaxFound )
            {
                iMaxFound = pHex->GetAlt( );
                hexFound  = _hexOn;
            }
        }
        else

        {
            // see if we can find a path
        DoIt:
            int        iPathLen;
            CHexCoord* phexPath =
                thePathMgr.GetPath( NULL, _hex, _hexOn, iPathLen, CTransportData::landing_craft, FALSE, FALSE );
            if ( iPathLen <= 0 )
            {
                int xDif = CHexCoord::Diff( _hexOn.X( ) - _hex.X( ) );
                int yDif = CHexCoord::Diff( _hexOn.Y( ) - _hex.Y( ) );
                if ( ( abs( xDif ) > 1 ) || ( abs( yDif ) > 1 ) )
                {
                    TRAP( );
                    return;
                }

                TRAP( );
                // fake one up if same or adjoining
                phexPath  = new CHexCoord[3];
                *phexPath = _hex;
                iPathLen  = 1;
                CHexCoord _tmp( _hex.X( ) + xDif, _hex.Y( ) + yDif );
                _tmp.Wrap( );
                if ( *phexPath != _tmp )
                {
                    *( phexPath + 1 ) = _tmp;
                    iPathLen          = 2;
                }
                if ( *( phexPath + iPathLen - 1 ) != _hexOn )
                {
                    *( phexPath + 1 ) = _hexOn;
                    iPathLen++;
                }
            }

            // ok, we walk from _hexOn along the path dropping the altitude
            CHexCoord  hexPrev( _hex );
            CHexCoord* phexOn = phexPath;
            while ( iPathLen-- > 0 )
            {
                pHex = theMap._GetHex( *phexOn );

                // something weird going on here
                if ( ( pHex->GetType( ) != CHex::lake ) && ( pHex->GetType( ) != CHex::ocean ) &&
                     ( pHex->GetType( ) != CHex::city ) )
                    break;
                // we made it
                if ( pHex->GetAlt( ) <= CTransportData::GetMaxDraft( ) )
                    break;

                // drop this one, and square out
                for ( int x = -1; x <= 1; x++ )
                    for ( int y = -1; y <= 1; y++ )
                    {
                        CHex* pHex = theMap.GetHex( phexOn->X( ) + x, phexOn->Y( ) + y );
                        if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
                            if ( pHex->GetAlt( ) > CTransportData::GetMaxDraft( ) )
                                pHex->SetOceanAlt( CTransportData::GetMaxDraft( ) );
                    }

                hexPrev = *phexOn++;
            }

            // square out if diag
            if ( ( hexPrev.X( ) != _hexOn.X( ) ) && ( hexPrev.Y( ) != _hexOn.Y( ) ) )
            {
                pHex = theMap._GetHex( hexPrev.X( ), _hexOn.Y( ) );
                if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
                    pHex->SetOceanAlt( CTransportData::GetMaxDraft( ) );
                pHex = theMap._GetHex( _hexOn.X( ), hexPrev.Y( ) );
                if ( ( pHex->GetType( ) == CHex::lake ) || ( pHex->GetType( ) == CHex::ocean ) )
                    pHex->SetOceanAlt( CTransportData::GetMaxDraft( ) );
            }

            // drop all adjoining hexes
            CHexCoord hexNext( _hex.X( ) - 1, _hex.Y( ) - 1 );
            hexNext.Wrap( );
            theMap.EnumHexes( hexNext, 3, 3, fnEnumDrop, NULL );

            delete[] phexPath;
            return;
        }
    }

    // make a channel to the deepest spot
    if ( iMaxFound >= CHex::sea_level )
        return;

    _hexOn = hexFound;
    goto DoIt;
}

BOOL CBuilding::InitRocket( CHexCoord const& hex, BOOL bMe )
{

    if ( bMe )
    {
        const int aiSupplies[] = { CRaceDef::light_scout,      CRaceDef::med_scout,  CRaceDef::infantry,
                                   CRaceDef::infantry_carrier, CRaceDef::light_tank, CRaceDef::med_tank,
                                   CRaceDef::light_art,        CRaceDef::crane,      CRaceDef::heavy_truck };
        const int aiVehicles[] = { CTransportData::light_scout, CTransportData::med_scout,
                                   CTransportData::infantry,    CTransportData::infantry_carrier,
                                   CTransportData::light_tank,  CTransportData::med_tank,
                                   CTransportData::light_art,   CTransportData::construction,
                                   CTransportData::heavy_truck };
        const int NUM_VEH_SUP  = sizeof( aiSupplies ) / sizeof( int );

        // how many vehicles
        int iTotalLeft = 0;
        for ( int iOn = 0; iOn < NUM_VEH_SUP; iOn++ )
            iTotalLeft += GetOwner( )->m_InitData.GetSupplies( aiSupplies[iOn] );

        // ok, we get the time to travel to the hexes in a nxn square around the rocket
        // we will send vehicles to the fastest ones.
        int  iBldgSiz = theStructures.GetData( CStructureData::rocket )->GetCX( );
        int  iBlkSiz  = (int)sqrt( (float)( 5 * iTotalLeft + 12 ) ) + 3 + iBldgSiz;
        int  iTotal   = iBlkSiz * iBlkSiz;
        int* aiTime   = new int[iTotal];
        memset( aiTime, 0, iBlkSiz * iBlkSiz * sizeof( int ) );

        CHexCoord hexUL;
        hexUL.X( m_hexExit.X( ) - iBlkSiz / 2 );
        hexUL.Y( m_hexExit.Y( ) - iBlkSiz / 2 );

        // set every other to -1
        int* piFill = aiTime;
        while ( iTotal > 0 )
        {
            *piFill = -1;
            piFill += 2;
            iTotal -= 2;
        }

        // not next to the rocket
        piFill = aiTime + ( iBlkSiz / 2 - 1 ) * iBlkSiz + iBlkSiz / 2 - 2;
        for ( int iInd = 0; iInd < iBldgSiz + 2; iInd++, piFill += iBldgSiz )
            for ( int iFill = 0; iFill < iBldgSiz + 2; iFill++ ) *( piFill + iFill ) = -1;

        for ( int iOn = 0; iOn < NUM_VEH_SUP; iOn++ )
        {
            iTotalLeft -= GetOwner( )->m_InitData.GetSupplies( aiSupplies[iOn] );
            DropUnits( aiVehicles[iOn], theTransports.GetData( aiVehicles[iOn] )->GetWheelType( ),
                       GetOwner( )->m_InitData.GetSupplies( aiSupplies[iOn] ), iTotalLeft, aiTime, iBlkSiz, m_hexExit,
                       hexUL );
            if ( !theGame.ShouldProcessMessages() )
                return ( TRUE );
        }

#ifdef _CHEAT
        if ( theApp.GetProfileInt( "Cheat", "AllUnits", 0 ) )
        {
            int i = 0;
            for ( i = 0; i < CTransportData::light_cargo; i++ )
            {
                CTransportData const* pData = theTransports.GetData( i );
                if ( pData->GetUnitFlags( ) & CUnitData::FLhaveArt )
                    DropUnits( i, pData->GetWheelType( ), 1, 27 - i, aiTime, iBlkSiz, m_hexExit, hexUL );
            }
            for (i = CTransportData::infantry; i < CTransportData::marines; i++ )
            {
                CTransportData const* pData = theTransports.GetData( i );
                if ( pData->GetUnitFlags( ) & CUnitData::FLhaveArt )
                    DropUnits( i, pData->GetWheelType( ), 1, 15 - i, aiTime, iBlkSiz, m_hexExit, hexUL );
            }
            CTransportData const* pData = theTransports.GetData( CTransportData::construction );
            if ( pData->GetUnitFlags( ) & CUnitData::FLhaveArt )
                DropUnits(i, pData->GetWheelType( ), 3, 3, aiTime, iBlkSiz, m_hexExit, hexUL );
        }

        if ( ( !GetOwner( )->IsAI( ) ) && ( theApp.GetProfileInt( "Cheat", "MoreTanks", 0 ) ) )
        {
            DropUnits( CTransportData::heavy_tank, theTransports.GetData( CTransportData::heavy_tank )->GetWheelType( ),
                       8, 8, aiTime, iBlkSiz, m_hexExit, hexUL );
            DropUnits( CTransportData::heavy_art, theTransports.GetData( CTransportData::heavy_art )->GetWheelType( ),
                       8, 0, aiTime, iBlkSiz, m_hexExit, hexUL );
        }
#endif

        delete[] aiTime;
    }

    // tell the AI to come get us if it's scenario 0
    if ( ( theGame.GetScenario( ) == 0 ) && ( bMe ) )
        theApp.ScenarioStart( );

    return ( FALSE );
}

static int fnEnumClrOwner( CHex* pHex, CHexCoord _hex, void* pData )
{

    if ( pHex->GetUnits( ) & CHex::bldg )
    {
        CBuilding* pBldg = theBuildingHex._GetBuilding( _hex );
        if ( pBldg->IsFlag( CUnit::dead ) || pBldg->IsFlag( CUnit::dying ) )
            delete pBldg;
        else
            TRAP( );
    }

    return ( FALSE );
}

static int fnEnumDecVis( CHex* pHex, CHexCoord, void* )
{

    pHex->DecVisible( );

    return ( FALSE );
}

CBuilding* CBuilding::Create( CHexCoord const& hex, int iBldg, int iBldgDir, CVehicle* pVeh, int iOwner, DWORD ID,
                              BOOL bShare )
{

    ASSERT_STRICT_VALID_OR_NULL( pVeh );
    ASSERT( ( iBldg != CStructureData::apartment_3_1 ) && ( iBldg != CStructureData::apartment_3_2 ) &&
            ( iBldg != CStructureData::office_3_2 ) && ( iBldg != CStructureData::barracks_3 ) );

    // having problems with this
    if ( theGame._GetPlayerByPlyr( iOwner ) == NULL )
    {
        TRAP( );
        return NULL;
    }

    // this takes awhile
    BOOL bRocket;
    if ( iBldg == CStructureData::rocket )
    {
        // we'll be awhile
        ::SetCursor( theApp.LoadStandardCursor( IDC_WAIT ) );
        bRocket = TRUE;
    }
    else
        bRocket = FALSE;

    // make an instance of a building
    CBuilding* pBldg = Alloc( iBldg, iBldgDir, iOwner, ID );

    // in a net game a dead building may still be assigned to show it - we kill it here
    theMap.EnumHexes( hex, pBldg->GetCX( ), pBldg->GetCY( ), fnEnumClrOwner, 0 );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_UNIT_STAT, "Building %d (%s) located at (%d,%d) built by %s", pBldg->GetID( ),
               (LPCSTR)pBldg->GetData( )->GetDesc( ), hex.X( ), hex.Y( ), pBldg->GetOwner( )->GetName( ) );
#endif

    // now set m_bVisible to 0 for our landing block
    // since we start with the entire landing area visible
    if ( ( bRocket ) && ( pBldg->GetOwner( )->IsMe( ) ) )
    {
        CHexCoord hexVis( theGame.GetMe( )->m_hexMapStart.X( ) - theGame.GetSideSize( ) / 2,
                          theGame.GetMe( )->m_hexMapStart.Y( ) - theGame.GetSideSize( ) / 2 );
        theMap.EnumHexes( hexVis, theGame.GetSideSize( ), theGame.GetSideSize( ), fnEnumDecVis, NULL );


        theApp.m_wndWorld.VisibleOff( );

        // we need to redraw everything NOW because of the spotting
        CWndAnim::InvalidateAllWindows( );
    }

    // get the foundation cost
    int iAlt;
    pBldg->m_iFoundTime = theMap.FoundationCost( hex, iBldg, iBldgDir, NULL, &iAlt ) * 24;
    if ( pBldg->m_iFoundTime < 0 )
    {
        ASSERT( FALSE );
        pBldg->m_iFoundTime = 0;
        iAlt                = ( theMap.GetHex( hex )->GetAlt( ) + 2 ) & ~0x03;
    }
    if ( ( pVeh == NULL ) && bRocket )
    {
        pBldg->m_iFoundTime              = 0;
        pBldg->GetOwner( )->m_dwIDRocket = pBldg->GetID( );
    }

    // assign it to the hex's at the 4 corners
    pBldg->AssignToHex( hex, iAlt );

    // set up the exit info
    DetermineExit( iBldgDir, iBldg, pBldg->m_hexExit, pBldg->m_iExitDir, FALSE );
    pBldg->m_hexExit += pBldg->m_hex;
    DetermineExit( iBldgDir, iBldg, pBldg->m_hexShip, pBldg->m_iShipDir, TRUE );
    pBldg->m_hexShip += pBldg->m_hex;

    // type based actions
    switch ( pBldg->GetData( )->GetUnionType( ) )
    {
    case CStructureData::UTmine:
        // get the ground data
        ( (CMineBuilding*)pBldg )->UpdateMine( );
        break;
    case CStructureData::UTfarm:
        // get the ground data
        ( (CFarmBuilding*)pBldg )->UpdateFarm( );
        break;
    }

    // add to our count of buildings (rocket already counted)
    if ( !bRocket )
    {
        pBldg->GetOwner( )->IncBldgsBuilt( );
        pBldg->GetOwner( )->AddBldgsHave( 1 );
    }

    // building based actions
    switch ( iBldg )
    {
    case CStructureData::rocket:
        // people live on it
        pBldg->GetOwner( )->m_iAptCap += ROCKET_APT_CAP;
        pBldg->GetOwner( )->m_iOfcCap += ROCKET_OFC_CAP;
        pBldg->GetOwner( )->m_bPlacedRocket = TRUE;

        // give it all materials
        if ( pBldg->GetOwner( )->IsLocal( ) )
        {
            pBldg->SetStore( CMaterialTypes::lumber, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::lumber ) );
            pBldg->SetStore( CMaterialTypes::coal, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::coal ) );
            pBldg->SetStore( CMaterialTypes::iron, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::iron ) );
            pBldg->SetStore( CMaterialTypes::steel, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::steel ) );
            pBldg->SetStore( CMaterialTypes::copper, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::copper ) );
            pBldg->SetStore( CMaterialTypes::oil, pBldg->GetOwner( )->m_InitData.GetSupplies( CRaceDef::oil ) );
        }

#ifdef _CHEAT
        if ( ( _bMaxMaterials || _bMaxRocket ) && ( pBldg->GetOwner( )->IsLocal( ) ) )
            for ( int iOn = 0; iOn < CMaterialTypes::num_types; iOn++ )
                if ( ( iOn != CMaterialTypes::food ) && ( iOn != CMaterialTypes::gas ) &&
                     ( iOn != CMaterialTypes::moly ) && ( iOn != CMaterialTypes::goods ) )
                    pBldg->SetStore( iOn, 64000 );
#endif
        break;

#ifdef _CHEAT
    default:
        if ( ( _bMaxMaterials ) && ( pBldg->GetOwner( )->IsLocal( ) ) )
        {
            int iAcpts[CMaterialTypes::num_types];
            pBldg->GetAccepts( iAcpts );
            int iOn = 0;
            for ( iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
                if ( iAcpts[iOn] )
                    pBldg->SetStore( iOn, 64000 );
            for ( iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ )
                pBldg->AddToStore( iOn, pBldg->GetData( )->GetBuild( iOn ) );
        }
#endif
    }

    // AI cheat - we give it const materials
    if ( ( pBldg->GetOwner( )->IsAI( ) ) && ( pBldg->GetOwner( )->IsLocal( ) ) )
        for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ )
            pBldg->AddToStore( iOn, ( theGame.m_iAi * pBldg->GetData( )->GetBuild( iOn ) ) / 3 );

    // mark that we have the materials in the building
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
        pBldg->GetOwner( )->IncMaterialHave( iOn, pBldg->GetStore( iOn ) );

    // put it at the end of the list
    theBuildingMap.Add( pBldg );

    // if its the initial setup
    if ( ( pVeh == NULL ) && bRocket )
    {
        pBldg->m_iConstDone = -1;

        if ( ( pBldg->GetOwner( )->IsMe( ) ) || ( theMap.GetHex( pBldg->GetHex( ) )->GetVisible( ) ) )
            pBldg->SetConstPer( );

        // tell everyone
        if ( theGame.AmServer( ) )
        {
            CMsgBldgStat msg( pBldg );
            msg.m_iFlags = CMsgBldgStat::built;
            theGame.PostToAllClients( &msg, sizeof( msg ) );
        }
    }

    // see if it's us
    BOOL bMe;
    if ( ( theGame.HaveHP( ) ) && ( ( iOwner == 0 ) || ( iOwner == theGame.GetMe( )->GetPlyrNum( ) ) ) )
        bMe = TRUE;
    else
        bMe = FALSE;

    // it's visible if it's ours or on a visible hex
    if ( bMe || ( pBldg->GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
        pBldg->MakeBldgVisible( );
    else
    {
        // if they are demo version and we aren't - we see it
        if ( bShare && ( !theApp.IsShareware( ) ) )
        {
            pBldg->MakeBldgVisible( );
            pBldg->SetFlag( CUnit::show_bldg );
            goto IsVis;
        }

        CHexCoord _hex( pBldg->m_hex );
        for ( int x = 0; x < pBldg->GetCX( ); x++ )
            for ( int y = 0; y < pBldg->GetCY( ); y++ )
                if ( theMap.GetHex( pBldg->m_hex.X( ) + x, pBldg->m_hex.Y( ) + y )->GetVisibility( ) )
                {
                    pBldg->MakeBldgVisible( );
                    goto IsVis;
                }
    }
IsVis:

    // add to the listbox - only do this if its ours
    if ( bMe || ( pBldg->GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
    {
        if ( bMe )
            theApp.m_wndBldgs.AddToList( pBldg );

        // set it's visibility
        if ( ( pVeh == NULL ) && bRocket )
            pBldg->DetermineSpotting( );
        else

        // set the base to spotted
        {
            // set it to nothing spotted
            memset( pBldg->m_dwaSpot, 0, sizeof( pBldg->m_dwaSpot ) );

            int iInd = MAX_SPOTTING;

            for ( int y = 0; y < pBldg->GetData( )->GetCY( ); y++ )
            {
                int iMask = 1 << MAX_SPOTTING;
                for ( int x = 0; x < pBldg->GetCX( ); x++ )
                {
                    pBldg->m_dwaSpot[iInd] |= iMask;
                    iMask <<= 1;
                }
                iInd++;
            }
        }

        pBldg->IncrementSpotting( pBldg->m_hex );
    }

    // have the vehicle start construction
    if ( pVeh != NULL )
        pVeh->StartConst( pBldg );

    // if its a sea*** we may need a channel
    if ( pBldg->GetData( )->HasShipExit( ) )
    {
        CHexCoord _hex( pBldg->GetShipHex( ) );
        FindChannel( _hex );
    }

    // if a vehicle exit we drop a road on it
    if ( pBldg->GetData( )->HasVehExit( ) )
    {
        CHexCoord _hex( pBldg->GetExit( CWheelTypes::wheel ) );
        theMap._GetHex( _hex )->ChangeToRoad( _hex, TRUE, pBldg->GetOwner( )->IsMe( ) );
    }

    // have it update its material list
    pBldg->MaterialChange( );

    // if our rocket, stock up the vehicles, find city center
    if ( ( iBldg == CStructureData::rocket ) && ( pBldg->GetOwner( )->IsLocal( ) ) )
        if ( pBldg->InitRocket( hex, bMe ) )
            return ( NULL );

    // world map
    if ( pBldg->IsVisible( ) )
        theApp.m_wndWorld.NewMode( );

    // if visible start the sound
    CWndArea* pAreaWnd = theAreaList.GetTop( );
    if ( ( pAreaWnd != NULL ) && ( pBldg->IsVisible( ) ) )
    {
        CRect rect;
        pAreaWnd->GetClientRect( &rect );
        CPoint ptBldg = pAreaWnd->GetAA( ).WrapWorldToWindow( CMapLoc3D( pBldg->GetWorldPixels( ) ) );
        if ( rect.PtInRect( ptBldg ) )
            pAreaWnd->InvalidateSound( );
    }

    // if us, tell the router
    if ( bMe )
    {
        if ( pVeh == NULL )
            theGame.m_pHpRtr->MsgNewBldg( pBldg );
        else
            theGame.m_pHpRtr->MsgBuiltBldg( pBldg );

        // as good a place as any to unload these
        if ( !bRocket )
        {
            theMusicPlayer.KillForegroundSound( SOUNDS::GetID( SOUNDS::player_join ) );
            theMusicPlayer.KillForegroundSound( SOUNDS::GetID( SOUNDS::rocket_landing ) );
            theMusicPlayer.KillForegroundSound( VOICES::GetID( VOICES::rocket_cant_land, 0 ) );
        }
    }

    // will get us the materials to build it
    if ( pVeh && pBldg->GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgOutMat( pBldg );
    else
        pBldg->MaterialMessage( );

    // if scenario 10 and another refinery/power - show it
    if ( theGame.GetScenario( ) == 10 )
        if ( ( pBldg->GetOwner( )->IsAI( ) ) && ( ( pBldg->GetData( )->GetType( ) == CStructureData::refinery ) ||
                                                  ( pBldg->GetData( )->GetType( ) == CStructureData::power_1 ) ||
                                                  ( pBldg->GetData( )->GetType( ) == CStructureData::power_2 ) ||
                                                  ( pBldg->GetData( )->GetType( ) == CStructureData::power_3 ) ) )
        {
            TRAP( );
            ShowBuilding( 0, pBldg );
        }

    // get it going
    if ( pBldg->GetData( )->GetBldgFlags( ) & CStructureData::FlConstAmb1 )
        pBldg->GetAmbient( CSpriteView::ANIM_FRONT_1 )->Enable( TRUE );
    if ( pBldg->GetData( )->GetBldgFlags( ) & CStructureData::FlConstAmb2 )
        pBldg->GetAmbient( CSpriteView::ANIM_FRONT_2 )->Enable( TRUE );

#ifdef _DEBUG
    pBldg->m_Initialized = TRUE;
    ASSERT_VALID( pBldg );
    ASSERT_STRICT_VALID( &theBuildingMap );
#endif

    return ( pBldg );
}

void CBuilding::DetermineSpotting( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT( !m_bSpotted );

    // set it to nothing spotted
    memset( m_dwaSpot, 0, sizeof( m_dwaSpot ) );

    int* piOn = CVehicle::m_apiWid[GetSpottingRange( )];
    int  iInd = MAX_SPOTTING - GetSpottingRange( );

    for ( int y = -GetSpottingRange( ); y < GetSpottingRange( ) + GetData( )->GetCY( ); y++ )
    {
        ASSERT( ( 0 <= iInd ) && ( iInd < SPOTTING_LINE ) );

        int xMax = ( *piOn ) * 2 + GetCX( );
        ASSERT( ( 0 <= ( MAX_SPOTTING - *piOn ) ) && ( ( MAX_SPOTTING - *piOn ) < SPOTTING_LINE ) );
        int iMask = 1 << ( MAX_SPOTTING - *piOn );

        for ( int x = 0; x < xMax; x++ )
        {
            m_dwaSpot[iInd] |= iMask;
            iMask <<= 1;
        }

        if ( ( y < 0 ) || ( GetData( )->GetCY( ) <= y ) )
            piOn++;
        iInd++;
    }

    theApp.m_wndWorld.InvalidateWindow( CWndWorld::my_units | CWndWorld::other_units );
}

void CBuilding::FixUp( )
{

    GetOwner( )->AddExists( GetData( )->GetType( ), 1 );
    CUnit::FixUp( );
}

// see if we have enough materials to finish our job
void CBuilding::MaterialChange( )
{

    ASSERT_STRICT_VALID( this );

    CUnit::MaterialChange( );

    if ( !GetOwner( )->IsMe( ) )
        return;

    if ( m_iConstDone != -1 )
    {
        if ( m_unitFlags & ( stopped | event ) )
        {
            m_iMatTo = m_iLastPer;
            return;
        }

        // note: this comes from mainloop.cpp
        m_iMatTo = 100;
        for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
            if ( NeedToBuild( iInd ) > GetStore( iInd ) )
            {
                int iPer = m_iLastMaterialTake +
                           ( ( GetStore( iInd ) * ( 100 - m_iLastMaterialTake ) ) / NeedToBuild( iInd ) );
                if ( iPer < m_iMatTo )
                    m_iMatTo = iPer;
            }
        if ( m_iMatTo > m_iLastPer )
            theGame.Event( EVENT_CONST_HALTED, EVENT_OFF );
        return;
    }

    // ok, show available materials
    m_iMatTo = 100;
}

void CVehicleBuilding::DestroyAllWindows( )
{

    if ( m_pDlgTransport != NULL )
    {
        if ( m_pDlgTransport->m_hWnd != NULL )
            m_pDlgTransport->DestroyWindow( );
        else
            delete m_pDlgTransport;
        m_pDlgTransport = NULL;
    }

    CBuilding::DestroyAllWindows( );
}

CVehicleBuilding::~CVehicleBuilding( )
{

    if ( ( m_pDlgTransport != NULL ) && ( m_pDlgTransport->m_hWnd != NULL ) )
        m_pDlgTransport->DestroyWindow( );
}

// see if we have enough materials to finish our job
void CVehicleBuilding::MaterialChange( )
{

    CBuilding::MaterialChange( );

    if ( !GetOwner( )->IsMe( ) )
        return;

    if ( m_iConstDone != -1 )
        return;

    // not building anything
    if ( m_pBldUnt == NULL )
        return;

    InvalidateStatus( );

    // stopped
    if ( m_unitFlags & ( stopped | event ) )
    {
        m_iMatTo = m_iLastPer;
        return;
    }

    // got all materials
    // note: from here down pulled from mainloop
    m_iMatTo = 100;
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
        if ( m_pBldUnt->GetInput( iInd ) > 0 )
        {
            int iPer = ( ( m_aiUsed[iInd] + GetStore( iInd ) ) * 100 ) / m_pBldUnt->GetInput( iInd );
            if ( iPer < m_iMatTo )
                m_iMatTo = iPer;
        }
}

void CVehicleBuilding::AssignBldUnit( int iIndex )
{

    m_pBldUnt = NULL;
    for ( int iInd = 0; iInd < GetData( )->GetBldVehicle( )->GetSize( ); iInd++ )
        if ( GetData( )->GetBldVehicle( )->GetUnit( iInd )->GetVehType( ) == iIndex )
        {
            m_pBldUnt = GetData( )->GetBldVehicle( )->GetUnit( iInd );
            return;
        }
}

void CVehicleBuilding::StartVehicle( int iIndex, int iNum )
{

    ASSERT_STRICT_VALID( this );
    // BUGBUG - switch to list in m_aBldUnits
    ASSERT_STRICT( ( 0 <= iIndex ) && ( iIndex < theTransports.GetNumTransports( ) ) );
    ASSERT( ( 0 < iNum ) && ( iNum < 200 ) );

    // give back used materials
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
        if ( m_aiUsed[iInd] > 0 )
        {
            AddToStore( iInd, m_aiUsed[iInd] );
            GetOwner( )->IncMaterialHave( iInd, m_aiUsed[iInd] );
        }
    memset( m_aiUsed, 0, sizeof( m_aiUsed ) );

    m_iNum = iNum;
    AssignBldUnit( iIndex );

    if ( m_pBldUnt == NULL )
    {
        ASSERT( FALSE );
        return;
    }
    m_iBuildDone = 0;
    m_iLastPer   = 0;
    m_fOperMod   = 0.0f;
    MaterialChange( );

    // will get us materials if we need them now
    if ( GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgOutMat( this );
    else
        MaterialMessage( );

    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
        if ( m_pBldUnt->GetInput( iInd ) > 0 )
            if ( GetStore( iInd ) <= 0 )
            {
                theGame.Event( EVENT_BUILD_HALTED, EVENT_WARN, this );
                return;
            }

    AnimateOperating( TRUE );

    // show it on the dialog
    CDlgBuildTransport* pDlg = QueryDlgBuild( );
    if ( pDlg != NULL )
        pDlg->UpdateStatus( 1 );

    theGame.MulEvent( MEVENT_BUILD_FACTORY, this );
}

void CVehicleBuilding::ctor( )
{

    m_pBldUnt       = NULL;
    m_pDlgTransport = NULL;
    memset( m_aiUsed, 0, sizeof( m_aiUsed ) );
    m_iNum = 1;
}

CDlgBuildTransport* CVehicleBuilding::GetDlgBuild( )
{

    if ( m_pDlgTransport == NULL )
        m_pDlgTransport = new CDlgBuildTransport( theApp.m_pMainWnd, this );
    ASSERT_STRICT_VALID( m_pDlgTransport );

    if ( m_pBldUnt == NULL )
        m_iLastPer = 0;

    if ( m_pDlgTransport->m_hWnd == NULL )
        m_pDlgTransport->Create( theApp.m_pMainWnd );

    m_pDlgTransport->ShowWindow( SW_SHOWNORMAL );
    m_pDlgTransport->SetFocus( );

    return ( m_pDlgTransport );
}

/////////////////////////////////////////////////////////////////////////////
// CPowerBuilding - a power plant

CPowerBuilding::CPowerBuilding( )
{
}

CPowerBuilding::CPowerBuilding( int iBldg, int iDir, int iOwner, DWORD ID ): CBuilding( iBldg, iDir, iOwner, ID )
{
}


/////////////////////////////////////////////////////////////////////////////
// CMineBuilding - a mine

void CMineBuilding::ctor( )
{

    m_iDensity = m_iMinerals = 0;
    m_fAmountMod             = 0.0f;
}

class CSetMin
{
  public:
    CSetMin( int iType )
    {
        m_iDen = m_iQuan = 0;
        m_iType          = iType;
    }

    int m_iDen;
    int m_iQuan;
    int m_iType;
};

static int fnAddAiMine( CHex*, CHexCoord hex, void* pData )
{

    CMinerals* pMn;
    if ( !theMinerals.Lookup( hex, pMn ) )
        return ( FALSE );

    if ( pMn->GetType( ) == (int)pData )
    {
        int iDen = pMn->GetDensity( );
        iDen += ( iDen * theGame.m_iAi ) / 4;
        if ( (int)pData == CMaterialTypes::oil )
            iDen *= 2;
        pMn->SetDensity( __min( MAX_MINERAL_DENSITY, iDen ) );

        int iQuan = pMn->GetQuantity( );
        iQuan += ( iQuan * theGame.m_iAi ) / 2;
        pMn->SetQuantity( __min( MAX_MINERAL_QUANTITY, iQuan ) );
    }
    return ( FALSE );
}

static int fnMineFromGround( CHex*, CHexCoord hex, void* pData )
{

    CMinerals* pMn;
    if ( !theMinerals.Lookup( hex, pMn ) )
        return ( FALSE );

    CSetMin* pSm = (CSetMin*)pData;

    if ( pMn->GetType( ) == pSm->m_iType )
    {
        pSm->m_iDen += pMn->GetDensity( );
        pSm->m_iQuan += pMn->GetQuantity( );
    }
    return ( FALSE );
}

void CMineBuilding::UpdateMine( )
{

    // for the AI we up the minerals & density
    if ( GetOwner( )->IsAI( ) )
        theMap.EnumHexes( m_hex, GetCX( ), GetCY( ), fnAddAiMine,
                          (void*)( GetData( )->GetBldMine( )->GetTypeMines( ) ) );

    // pull the minerals from the ground
    CSetMin sm( GetData( )->GetBldMine( )->GetTypeMines( ) );
    theMap.EnumHexes( m_hex, GetCX( ), GetCY( ), fnMineFromGround, &sm );
    m_iDensity  = sm.m_iDen / ( GetCX( ) * GetCY( ) );
    m_iMinerals = sm.m_iQuan;
}

int CMineBuilding::TotalQuantity( CHexCoord const& hex, int iBldgTyp, int iBldgDir )
{

    CStructureData const* pData = theStructures.GetData( iBldgTyp );
    if ( pData->GetUnionType( ) != CStructureData::UTmine )
    {
        ASSERT( FALSE );
        return ( 1 );
    }

    CBuildMine const* pMine = pData->GetBldMine( );

    CSetMin sm( pMine->GetTypeMines( ) );

    int cx = iBldgDir & 1 ? pData->GetCY( ) : pData->GetCX( );
    int cy = iBldgDir & 1 ? pData->GetCX( ) : pData->GetCY( );
    theMap.EnumHexes( hex, cx, cy, fnMineFromGround, &sm );
    return ( sm.m_iQuan );
}

int CMineBuilding::TotalDensity( CHexCoord const& hex, int iBldgTyp, int iBldgDir )
{

    CStructureData const* pData = theStructures.GetData( iBldgTyp );
    if ( pData->GetUnionType( ) != CStructureData::UTmine )
    {
        ASSERT( FALSE );
        return ( 1 );
    }

    CBuildMine const* pMine = pData->GetBldMine( );

    CSetMin sm( pMine->GetTypeMines( ) );

    int cx = iBldgDir & 1 ? pData->GetCY( ) : pData->GetCX( );
    int cy = iBldgDir & 1 ? pData->GetCX( ) : pData->GetCY( );
    theMap.EnumHexes( hex, cx, cy, fnMineFromGround, &sm );
    return ( sm.m_iDen );
}

class CGetMin
{
  public:
    CGetMin( float fMul, int iType )
    {
        m_fMul  = fMul;
        m_iType = iType;
    }

    int   m_iType;
    float m_fMul;
};

static int fnMineToGround( CHex*, CHexCoord hex, void* pData )
{

    CMinerals* pMn;
    if ( !theMinerals.Lookup( hex, pMn ) )
        return ( FALSE );

    CGetMin* pGm = (CGetMin*)pData;

    if ( pMn->GetType( ) == pGm->m_iType )
    {
        int iQuan = (int)( (float)pMn->GetQuantity( ) * pGm->m_fMul );
        if ( iQuan > 1 )
            pMn->SetQuantity( iQuan );
        else
        {
            // all gone - kill it
            delete pMn;
            theMinerals.RemoveKey( hex );
        }
    }
    return ( FALSE );
}

void CMineBuilding::UpdateGround( )
{

    ASSERT_STRICT_VALID( this );

    // find what's left - get a multiplier for the loss
    CSetMin sm( GetData( )->GetBldMine( )->GetTypeMines( ) );
    theMap.EnumHexes( m_hex, GetCX( ), GetCY( ), fnMineFromGround, &sm );
    float fChange;
    if ( sm.m_iQuan > 0 )
        fChange = (float)m_iMinerals / (float)sm.m_iQuan;
    else
        fChange = 0;

    // reduce everyone
    if ( fChange < 1.0 )
    {
        CGetMin gm( fChange, GetData( )->GetBldMine( )->GetTypeMines( ) );
        theMap.EnumHexes( m_hex, GetCX( ), GetCY( ), fnMineToGround, &gm );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CFarmBuilding - farm or lumber mill

void CFarmBuilding::ctor( )
{

    m_iTerMult = 0;
}

static int fnFarmFromGround( CHex* pHex, CHexCoord, void* pData )
{

    *( (int*)pData ) += theTerrain.GetData( pHex->GetType( ) ).GetFarmMult( );

    return ( FALSE );
}

static int fnLumberFromGround( CHex* pHex, CHexCoord, void* pData )
{

    if ( pHex->GetType( ) == CHex::forest )
        *( (int*)pData ) += 10;
    return ( FALSE );
}

int CFarmBuilding::LandMult( CHexCoord _hex, int iTyp, int iDir )
{

    CStructureData const* pData = theStructures.GetData( iTyp );
    int                   cx    = iDir & 1 ? pData->GetCY( ) : pData->GetCX( );
    int                   cy    = iDir & 1 ? pData->GetCX( ) : pData->GetCY( );

    if ( iTyp == CStructureData::lumber )
    {
        // get the number of forest hexes
        int iRtn = 0;
        _hex.X( _hex.X( ) - 3 );
        _hex.Y( _hex.Y( ) - 3 );
        theMap.EnumHexes( _hex, cx + 6, cy + 6, fnLumberFromGround, &iRtn );
        return ( iRtn / ( ( cx + 6 ) * ( cy + 6 ) ) );
    }

    // get the terrain multiplier
    int iRtn = 0;
    _hex.Xdec( );
    _hex.Ydec( );
    theMap.EnumHexes( _hex, cx + 2, cy + 2, fnFarmFromGround, &iRtn );
    return ( iRtn / ( ( cx + 2 ) * ( cy + 2 ) ) );
}

void CFarmBuilding::UpdateFarm( )
{

    m_iTerMult = LandMult( m_hex, GetData( )->GetType( ), GetDir( ) );
}


/////////////////////////////////////////////////////////////////////////////
// CRepairBuilding - a repair facility

void CRepairBuilding::ctor( )
{

    m_pVehRepairing = NULL;
    m_pBldUnt       = NULL;
}

CRepairBuilding::~CRepairBuilding( )
{

    if ( theApp.AmInGame( ) )
    {
        // kill all units we are repairing
        while ( m_lstNext.GetCount( ) > 0 )
        {
            CVehicle* pVeh = m_lstNext.RemoveHead( );
            delete pVeh;
        }

        if ( m_pVehRepairing != NULL )
        {
            CVehicle* pVeh  = m_pVehRepairing;
            m_pVehRepairing = NULL;
            delete pVeh;
        }
    }
}

void CRepairBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );

    memset( pVals, 0, sizeof( int ) * CMaterialTypes::GetNumTypes( ) );

    CBuildRepair* pBldRpr = GetData( )->GetBldRepair( );
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ ) *pVals++ = pBldRpr->GetTotalRepair( iOn );
}

void CRepairBuilding::AssignBldUnit( int iIndex )
{

    for ( int iInd = 0; iInd < GetData( )->GetSize( ); iInd++ )
        if ( GetData( )->GetRepair( iInd )->GetVehType( ) == iIndex )
        {
            m_pBldUnt = GetData( )->GetBldVehicle( )->GetUnit( iInd );
            return;
        }
}

void CRepairBuilding::RepairVehicle( CVehicle* pVeh )
{

    if ( ( m_pVehRepairing == NULL ) && ( m_lstNext.GetCount( ) == 0 ) )
    {
        m_pVehRepairing = pVeh;

        int iIndex = m_pVehRepairing->GetData( )->GetType( );
        AssignBldUnit( iIndex );
    }
    else
        m_lstNext.AddTail( pVeh );

    if ( m_pBldUnt == NULL )
    {
        m_pVehRepairing = NULL;
        AnimateOperating( FALSE );
    }
    else
        AnimateOperating( TRUE );

    // tell the router we need materials
    if ( GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgOutMat( this );
    else
        MaterialMessage( );

    pVeh->SetEvent( CVehicle::none );
}

void CRepairBuilding::VehicleLeaving( CVehicle* pVeh )
{

    // if in list remove it
    //   this also handles 1 pVehRep
    if ( m_pVehRepairing != pVeh )
    {
        POSITION pos = m_lstNext.Find( pVeh );
        if ( pos != NULL )
        {
            m_lstNext.RemoveAt( pos );
            return;
        }
        return;
    }

    // last one - just close it down
    if ( m_lstNext.GetCount( ) == 0 )
    {
        m_iLastPer      = 100;
        m_pVehRepairing = NULL;
        m_pBldUnt       = NULL;
        AnimateOperating( FALSE );
        MaterialChange( );
        theAreaList.MaterialChange( this );
        return;
    }

    // next
    while ( m_lstNext.GetCount( ) > 0 )
    {
        m_pVehRepairing = m_lstNext.RemoveHead( );
        // is it still here?
        if ( ( ( m_pVehRepairing->GetRouteMode( ) != CVehicle::stop ) &&
               ( m_pVehRepairing->GetRouteMode( ) != CVehicle::repair_self ) ) ||
             ( theBuildingHex._GetBuilding( m_pVehRepairing->GetPtHead( ) ) != this ) )
        {
            TRAP( );
            m_pVehRepairing = NULL;
        }
        else
        {
            ASSERT_VALID( m_pVehRepairing );
            m_iLastPer =
                ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            m_pBldUnt = GetData( )->GetBldVehicle( )->GetUnit( m_pVehRepairing->GetData( )->GetType( ) );
            break;
        }
    }

    // update the status
    if ( m_pVehRepairing == NULL )
    {
        m_iLastPer = 100;
        m_pBldUnt  = NULL;
        AnimateOperating( FALSE );
    }
    MaterialChange( );
    theAreaList.MaterialChange( this );
}

void CRepairBuilding::CancelUnit( )
{

    TRAP( );
    if ( m_pVehRepairing == NULL )
        return;
    TRAP( );

    // tell everyone our new damage level
    CMsgUnitDamage msg( this, this, this, 0 );
    theGame.PostToServer( &msg, sizeof( msg ) );

    TRAP( );
    m_pVehRepairing->ExitBuilding( );

    MaterialChange( );

    // update area list buttons
    theAreaList.MaterialChange( this );

    if ( m_lstNext.GetCount( ) == 0 )
    {
        m_pVehRepairing = NULL;
        m_pBldUnt       = NULL;
        AnimateOperating( FALSE );
        return;
    }
    TRAP( );

    m_pVehRepairing = m_lstNext.RemoveHead( );
    int iIndex      = m_pVehRepairing->GetData( )->GetType( );
    AssignBldUnit( iIndex );

    if ( m_pBldUnt == NULL )
        m_pVehRepairing = NULL;
}

void CRepairBuilding::MaterialChange( )
{

    CBuilding::MaterialChange( );

    if ( !GetOwner( )->IsMe( ) )
        return;

    // under const
    if ( m_iConstDone != -1 )
        return;

    // not repairing anything
    if ( ( m_pVehRepairing == NULL ) || ( m_pBldUnt == NULL ) )
        return;

    // set % done
    m_iLastPer = ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );

    InvalidateStatus( );

    // got all materials
    // note: from here down pulled from mainloop
    m_iMatTo = 100;
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        if ( iTotal > 0 )
        {
            int iLastAmount =
                ( iTotal * m_pVehRepairing->GetDamagePoints( ) ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iTotalAmount = ( iTotal * m_pVehRepairing->GetData( )->GetDamagePoints( ) ) /
                               m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff = iTotalAmount - iLastAmount;
            if ( iDiff > GetStore( iInd ) )
            {
                int iPer = ( ( iLastAmount + GetStore( iInd ) ) * 100 ) / iTotalAmount;
                ASSERT( ( 0 <= iPer ) && ( iPer <= 100 ) );
                if ( iPer < m_iMatTo )
                    m_iMatTo = iPer;
            }
        }
    }
}

void CRepairBuilding::Serialize( CArchive& ar )
{

    CBuilding::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        if ( m_pVehRepairing == NULL )
        {
            ar << (LONG)0;
            ar << (LONG)0;
        }
        else
        {
            ar << (LONG)m_pVehRepairing->GetID( );
            ar << (LONG)m_pVehRepairing->GetData( )->GetType( );
        }

        ar << (LONG)m_lstNext.GetCount( );
        POSITION pos;
        for ( pos = m_lstNext.GetHeadPosition( ); pos != NULL; )
        {
            CVehicle* pVeh = m_lstNext.GetNext( pos );
            ASSERT_STRICT_VALID( pVeh );
            ar << (LONG)pVeh->GetID( );
        }
    }

    else
    {
        LONG iIndex;
        ar >> iIndex;
        m_pVehRepairing = (CVehicle*)iIndex;

        ar >> iIndex;
        if ( m_pVehRepairing == NULL )
            m_pBldUnt = NULL;
        else
            AssignBldUnit( iIndex );

        ASSERT( m_lstNext.GetCount( ) == 0 );
        ar >> iIndex;
        while ( iIndex-- > 0 )
        {
            LONG id;
            ar >> id;
            m_lstNext.AddTail( (CVehicle*)id );
        }
    }
}

void CRepairBuilding::FixUp( )
{

    CBuilding::FixUp( );

    if ( m_pVehRepairing != NULL )
        m_pVehRepairing = theVehicleMap.GetVehicle( (DWORD)m_pVehRepairing );

    // fix the list
    POSITION pos;
    for ( pos = m_lstNext.GetHeadPosition( ); pos != NULL; )
    {
        POSITION  prevPos = pos;
        CVehicle* pVeh    = m_lstNext.GetNext( pos );
        pVeh              = theVehicleMap.GetVehicle( (DWORD)pVeh );
        m_lstNext.SetAt( prevPos, pVeh );
    }
}

void CShipyardBuilding::ctor( )
{

    m_pVehRepairing = NULL;
    m_iMode         = nothing;
}

CShipyardBuilding::~CShipyardBuilding( )
{

    if ( theApp.AmInGame( ) )
    {
        // kill all units we are repairing
        while ( m_lstNext.GetCount( ) > 0 )
        {
            CVehicle* pVeh = m_lstNext.RemoveHead( );
            delete pVeh;
        }

        if ( m_pVehRepairing != NULL )
        {
            CVehicle* pVeh  = m_pVehRepairing;
            m_pVehRepairing = NULL;
            delete pVeh;
        }
    }
}

void CShipyardBuilding::GetInputs( int* pVals ) const
{

    ASSERT_VALID( this );

    CVehicleBuilding::GetInputs( pVals );

    CBuildRepair* pBldRpr = GetData( )->GetBldRepair( );
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ ) *pVals++ = pBldRpr->GetTotalRepair( iOn );
}

void CShipyardBuilding::AssignBldUnit( int iIndex )
{

    for ( int iInd = 0; iInd < GetData( )->GetSize( ); iInd++ )
        if ( GetData( )->GetUnit( iInd )->GetVehType( ) == iIndex )
        {
            m_pBldUnt = GetData( )->GetBldVehicle( )->GetUnit( iInd );
            return;
        }
    TRAP( );
}

void CShipyardBuilding::RepairVehicle( CVehicle* pVeh )
{

    if ( ( m_iMode != build ) && ( m_pVehRepairing == NULL ) && ( m_lstNext.GetCount( ) == 0 ) )
    {
        m_pVehRepairing = pVeh;
        AssignBldUnit( m_pVehRepairing->GetData( )->GetType( ) );
        if ( m_pBldUnt == NULL )
        {
            TRAP( );
            m_pVehRepairing = NULL;
            m_iMode         = nothing;
            return;
        }

        m_iMode = repair;
        AnimateOperating( TRUE );
    }
    else
        m_lstNext.AddTail( pVeh );

    // tell the router we need materials
    if ( GetOwner( )->IsMe( ) )
        theGame.m_pHpRtr->MsgOutMat( this );
    else
        MaterialMessage( );

    pVeh->SetEvent( CVehicle::none );
}

void CShipyardBuilding::CancelUnit( )
{

    TRAP( );
    if ( m_pVehRepairing == NULL )
        return;
    TRAP( );

    // tell everyone our new damage level
    CMsgUnitDamage msg( this, this, this, 0 );
    theGame.PostToServer( &msg, sizeof( msg ) );

    TRAP( );
    m_pVehRepairing->ExitBuilding( );

    MaterialChange( );

    // update area list buttons
    theAreaList.MaterialChange( this );

    if ( m_lstNext.GetCount( ) == 0 )
    {
        m_pVehRepairing = NULL;
        if ( m_iMode == repair )
            m_iMode = nothing;
        AnimateOperating( FALSE );
        return;
    }
    TRAP( );

    m_pVehRepairing = m_lstNext.RemoveHead( );
}

void CShipyardBuilding::VehicleLeaving( CVehicle* pVeh )
{

    // if in list remove it
    //   this also handles 1 pVehRep
    if ( m_pVehRepairing != pVeh )
    {
        POSITION pos = m_lstNext.Find( pVeh );
        if ( pos != NULL )
        {
            TRAP( );
            m_lstNext.RemoveAt( pos );
            return;
        }
        return;
    }

    // last one - just close it down
    if ( m_lstNext.GetCount( ) == 0 )
    {
        m_iLastPer      = 100;
        m_pVehRepairing = NULL;
        m_pBldUnt       = NULL;
        if ( m_iMode == repair )
            m_iMode = nothing;
        AnimateOperating( FALSE );
        MaterialChange( );
        theAreaList.MaterialChange( this );
        return;
    }

    TRAP( );
    while ( m_lstNext.GetCount( ) > 0 )
    {
        m_pVehRepairing = m_lstNext.RemoveHead( );
        // is it still here?
        if ( ( ( m_pVehRepairing->GetRouteMode( ) != CVehicle::stop ) &&
               ( m_pVehRepairing->GetRouteMode( ) != CVehicle::repair_self ) ) ||
             ( theBuildingHex._GetBuilding( m_pVehRepairing->GetPtHead( ) ) != this ) )
        {
            TRAP( );
            m_pVehRepairing = NULL;
        }
        else
        {
            TRAP( );
            ASSERT_VALID( m_pVehRepairing );
            m_iLastPer =
                ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            m_pBldUnt = GetData( )->GetBldVehicle( )->GetUnit( m_pVehRepairing->GetData( )->GetType( ) );
            break;
        }
    }

    // update the status
    if ( m_pVehRepairing == NULL )
    {
        m_iLastPer = 100;
        m_pBldUnt  = NULL;
        if ( m_iMode == repair )
            m_iMode = nothing;
        AnimateOperating( FALSE );
    }
    MaterialChange( );
    theAreaList.MaterialChange( this );
}

void CShipyardBuilding::MaterialChange( )
{

    if ( ( m_iMode != repair ) || ( m_pVehRepairing == NULL ) )
    {
        CVehicleBuilding::MaterialChange( );
        return;
    }

    CBuilding::MaterialChange( );

    if ( !GetOwner( )->IsMe( ) )
    {
        TRAP( );
        return;
    }

    // set % done
    m_iLastPer = ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );

    InvalidateStatus( );

    // stopped
    if ( ( m_unitFlags & ( stopped | event ) ) || ( m_pBldUnt == NULL ) )
        return;

    // got all materials
    // note: from here down pulled from mainloop
    m_iMatTo = 100;
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        if ( iTotal > 0 )
        {
            int iLastAmount =
                ( iTotal * m_pVehRepairing->GetDamagePoints( ) ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iTotalAmount = ( iTotal * m_pVehRepairing->GetData( )->GetDamagePoints( ) ) /
                               m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff = iTotalAmount - iLastAmount;
            if ( iDiff > GetStore( iInd ) )
            {
                int iPer = ( ( iLastAmount + GetStore( iInd ) ) * 100 ) / iTotalAmount;
                ASSERT( ( 0 <= iPer ) && ( iPer <= 100 ) );
                if ( iPer < m_iMatTo )
                    m_iMatTo = iPer;
            }
        }
    }
}

void CShipyardBuilding::Serialize( CArchive& ar )
{

    CVehicleBuilding::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        if ( m_pVehRepairing == NULL )
        {
            ar << (LONG)0;
            ar << (LONG)0;
        }
        else
        {
            ar << (LONG)m_pVehRepairing->GetID( );
            ar << (LONG)m_pVehRepairing->GetData( )->GetType( );
        }

        ar << (LONG)m_lstNext.GetCount( );
        POSITION pos;
        for ( pos = m_lstNext.GetHeadPosition( ); pos != NULL; )
        {
            CVehicle* pVeh = m_lstNext.GetNext( pos );
            ASSERT_STRICT_VALID( pVeh );
            ar << (LONG)pVeh->GetID( );
        }
    }

    else
    {
        LONG iIndex;
        ar >> iIndex;
        m_pVehRepairing = (CVehicle*)iIndex;

        ar >> iIndex;
        if ( iIndex == 0 )
            m_pBldUnt = NULL;
        else
        {
            TRAP( );
            AssignBldUnit( iIndex );
        }

        ASSERT( m_lstNext.GetCount( ) == 0 );
        ar >> iIndex;
        while ( iIndex-- > 0 )
        {
            LONG id;
            ar >> id;
            m_lstNext.AddTail( (CVehicle*)id );
        }
    }
}

void CShipyardBuilding::FixUp( )
{

    CVehicleBuilding::FixUp( );

    if ( m_pVehRepairing != NULL )
    {
        TRAP( );
        m_pVehRepairing = theVehicleMap.GetVehicle( (DWORD)m_pVehRepairing );
    }

    // fix the list
    POSITION pos;
    for ( pos = m_lstNext.GetHeadPosition( ); pos != NULL; )
    {
        POSITION  prevPos = pos;
        CVehicle* pVeh    = m_lstNext.GetNext( pos );
        pVeh              = theVehicleMap.GetVehicle( (DWORD)pVeh );
        m_lstNext.SetAt( prevPos, pVeh );
    }
}

//---------------------------- C T u r r e t s ------------------------------

CTurrets::CTurrets( char const* pszRifName ): CSpriteStore<CVehicleSprite>( pszRifName )
{
}

//----------------------- C M u z z l e F l a s h e s -----------------------

CMuzzleFlashes::CMuzzleFlashes( char const* pszRifName ): CSpriteStore<CVehicleSprite>( pszRifName )
{
}


/////////////////////////////////////////////////////////////////////////////
// CVehicle - vehicle

CTransport::CTransport( char const* pszRifName ): CSpriteStore<CVehicleSprite>( pszRifName )
{

    m_pData = NULL;
}

CTransport::~CTransport( )
{

    Close( );
}

//---------------------------------------------------------------------------
// CTransport::GetSprite
//---------------------------------------------------------------------------
CVehicleSprite* CTransport::GetSprite( int iID ) const
{
    // Get requested sprite
    CVehicleSprite* pvehiclesprite = CSpriteStore<CVehicleSprite>::GetSprite( iID, 0, TRUE );

    // Not there? Get default sprite for the requested type

    if ( !pvehiclesprite )
    {
        CTransportData const* ptransportdata = GetData( iID );

        if ( ptransportdata )
        {
            CTransportData::TRANS_BASE_TYPE eType = ptransportdata->GetBaseType( );

            pvehiclesprite = CSpriteStore<CVehicleSprite>::GetSprite( g_aiDefaultID[int( eType )], 0, TRUE );
        }
    }

    // Still not there? Get anything
    if ( !pvehiclesprite )
        pvehiclesprite = CSpriteStore<CVehicleSprite>::GetSprite( iID, 0, FALSE );

    return pvehiclesprite;
}

int CTransport::GetIndex( CTransportData const* pData ) const
{

    for ( int iInd = 0; iInd < m_iNumTransports; iInd++ )
        if ( ( m_pData + iInd ) == pData )
            return ( iInd );

    ASSERT( FALSE );
    return ( 0 );
}

//--------------------------------------------------------------------------
// CEffect::CEffect
//--------------------------------------------------------------------------
CEffect::CEffect( char const* pszRifName ): CSpriteStore<CEffectSprite>( pszRifName ), m_ptrees( NULL ), m_nTrees( 0 )
{
}

//--------------------------------------------------------------------------
// CEffect::~CEffect
//--------------------------------------------------------------------------
CEffect::~CEffect( )
{
    Close( );
}

//--------------------------------------------------------------------------
// CTurret::CTurret
//--------------------------------------------------------------------------
CTurret::CTurret( )
{
}

//--------------------------------------------------------------------------
// CTurret::CTurret
//--------------------------------------------------------------------------
CTurret::CTurret( int iTurretID )
{
    Init( iTurretID );
}

//--------------------------------------------------------------------------
// CTurret::Init
//--------------------------------------------------------------------------
void CTurret::Init( int iTurretID )
{
    m_iDir = 0;

    int iMuzzleFlashID = -1;

    switch ( iTurretID )
    {
    case CTurrets::light_tank:
        iMuzzleFlashID = CMuzzleFlashes::small_flash;
        break;
    case CTurrets::boss8800:
        iMuzzleFlashID = CMuzzleFlashes::large_flash;
        break;
    }

    if ( -1 != iMuzzleFlashID )
        m_muzzleflash.m_psprite = theFlashes.GetSprite( iMuzzleFlashID );

    m_psprite = theTurrets.GetSprite( iTurretID );
}

//-------------------------------- C F l a g --------------------------------

//--------------------------------------------------------------------------
// CFlag::CFlag
//--------------------------------------------------------------------------
CFlag::CFlag( ): m_iPlayer( -1 )
{
    Init( 0 );
}

//--------------------------------------------------------------------------
// CFlag::CFlag
//--------------------------------------------------------------------------
CFlag::CFlag( int iPlayer )
{
    Init( iPlayer );
}

//--------------------------------------------------------------------------
// CFlag::Init
//--------------------------------------------------------------------------
void CFlag::Init( int iPlayer )
{
    m_iPlayer = iPlayer;

    int nSprites = theEffects.GetCount( CEffect::flag );

    ASSERT( 0 < nSprites );

    m_psprite = theEffects.GetSprite( CEffect::flag, m_iPlayer % nSprites );
}

/////////////////////////////////////////////////////////////////////////////
// CVehicle - vehicle

void CVehicle::ctor( )
{
    m_iUnitType = CUnit::vehicle;

    m_ptDest = m_ptTail = m_ptNext = m_ptHead = CSubHex( 0, 0 );
    m_hexDest = m_hexLastDest = m_hexNext = CHexCoord( 0, 0 );
    m_iDir = m_maploc.x = m_maploc.y = m_iSpeed = 0;
    m_iDestMode                                 = sub;
    m_bFlags                                    = told_ai_stop;

    m_hexVis = CHexCoord( -1, -1 );

    m_iDelay = 0;

    m_iEvent   = none;
    m_cMode    = stop;
    m_pos      = NULL;
    m_phexPath = NULL;
    m_iPathOff = 0;
    m_iPathLen = 0;
    m_pBldg    = NULL;

    m_iBuildDone = -1;
    m_lOperMod   = 0;
    m_iLastPer   = 0;

    m_iStepsLeft = 0;
    m_iSpeed     = 0;
    m_iXadd      = 0;
    m_iYadd      = 0;
    m_iDadd      = 0;
    m_iTadd      = 0;
    m_fVehMove   = 0;

    m_iTimesOn    = 0;
    m_iNumClosest = 0;
    m_iClosest    = INT_MAX;
    ;
    m_iNumRetries   = 0;
    m_dwTimeBlocked = 0;
    m_iBlockCount   = 0;

    m_pWndRoute     = NULL;
    m_cOwn          = FALSE;
    m_pDlgStructure = NULL;
    m_pDlgLoad      = NULL;

    m_pVehLoadOn = NULL;
    m_pTransport = NULL;
    m_iCargoSize = 0;

    // everything is visible if on a visible hex
    m_iVisible = 1;
}

// this turns wheels on & off
void CVehicle::Wheels( BOOL bEnable, BOOL bSfx )
{

    // set vehicle sound
    if ( bSfx && ( GetOwner( )->IsMe( ) ) && ( IsSelected( ) ) )
    {
        if ( bEnable )
        {
            theMusicPlayer.DecBackgroundSound( GetData( )->GetSoundIdle( ) );
            theMusicPlayer.IncBackgroundSound( GetData( )->GetSoundRun( ) );
        }
        else
        {
            theMusicPlayer.DecBackgroundSound( GetData( )->GetSoundRun( ) );
            theMusicPlayer.IncBackgroundSound( GetData( )->GetSoundIdle( ) );
        }
    }

    // GG	if (GetData()->GetVehFlags () & CTransportData::FLwheel_amb1)
    GetAmbient( CSpriteView::ANIM_BACK_1 )->Pause( !bEnable );
    //	GG	if (GetData()->GetVehFlags () & CTransportData::FLwheel_amb2)
    GetAmbient( CSpriteView::ANIM_BACK_2 )->Pause( !bEnable );
}

CVehicle::CVehicle( int iVeh, int iOwner, DWORD ID )
{

    ctor( );

    m_dwTimeJump = theGame.GettimeGetTime( ) + 1000 * TRUCK_JUMP_TIME;

    for ( int iOn = 0; iOn < NUM_SUBS_OWNED; iOn++ ) SubsOwned[iOn] = CSubHex( -1, -1 );

    m_pOwner = theGame.GetPlayerByPlyr( iOwner );
    GetFlag( )->Init( m_pOwner->GetPlyrNum( ) );
    ASSERT( ID != 0 );
    m_dwID = ID;

    ASSERT_STRICT( ( 0 <= iVeh ) && ( iVeh < theTransports.m_iNumTransports ) );

    m_psprite = theTransports.GetSprite( iVeh );
    AssignData( theTransports.GetData( iVeh ) );

    // we do spotting
    if ( m_pOwner->IsMe( ) || ( ( m_pOwner->GetTheirRelations( ) == RELATIONS_ALLIANCE ) &&
                                ( ( GetData( )->GetType( ) == CTransportData::light_scout ) ||
                                  ( GetData( )->GetType( ) == CTransportData::med_scout ) ) ) )
        DoSpottingOn( );

    // update counts
    switch ( GetData( )->GetType( ) )
    {
    case CTransportData::construction:
        m_pOwner->m_iNumCranes++;
        break;
    case CTransportData::med_truck:
    case CTransportData::heavy_truck:
        m_pOwner->m_iNumTrucks++;
        break;
    }

    InitSmokeAndFlame( );

    // assign default bridge location
    if ( GetData( )->IsBoat( ) )
        m_bFlags |= on_water;

    // Assign turret

    int iTurretID = -1;

    switch ( iVeh )
    {
    case CTransportData::light_tank:
        iTurretID = CTurrets::light_tank;
        break;
    case CTransportData::med_tank:
        iTurretID = CTurrets::boss8800;
        break;
    }

    if ( -1 != iTurretID )
        SetTurret( iTurretID );

    // turn wheels off
    Wheels( FALSE, FALSE );

    m_iDamagePoints = m_pUnitData->GetDamagePoints( );

    ASSERT_STRICT_VALID( this );
}

void CVehicle::AddUnit( )
{

    CUnit::AddUnit( );

    // track
    GetOwner( )->AddVehsHave( 1 );

    if ( !theApp.AmInGame( ) )
        return;

    // spotting
    if ( m_pOwner->IsMe( ) || ( ( m_pOwner->GetTheirRelations( ) == RELATIONS_ALLIANCE ) &&
                                ( ( GetData( )->GetType( ) == CTransportData::light_scout ) ||
                                  ( GetData( )->GetType( ) == CTransportData::light_scout ) ) ) )
    {
        DoSpottingOn( );
        if ( GetHexOwnership( ) )
        {
            DetermineSpotting( );
            IncrementSpotting( GetHexHead( ) );
        }
    }
    else
        DoSpottingOff( );

    if ( !GetOwner( )->IsLocal( ) )
        return;

    m_pOwner->PplBldgToVeh( GetData( )->GetPeople( ) );

    switch ( GetData( )->GetType( ) )
    {
    case CTransportData::construction:
        m_pOwner->m_iNumCranes++;
        break;
    case CTransportData::med_truck:
    case CTransportData::heavy_truck:
        m_pOwner->m_iNumTrucks++;
        break;
    }

    if ( GetOwner( )->IsAI( ) )
    {
        CMsgPlaceVeh msg( GetPtHead( ), GetHexDest( ), GetOwner( )->GetPlyrNum( ), GetData( )->GetType( ) );
        theGame.PostToAllAi( &msg, sizeof( msg ) );
    }
    else

    // IsMe
    {
        m_iVisible = 1;
        theApp.m_wndVehicles.AddToList( this );
        if ( GetData( )->IsTransport( ) )
            theGame.m_pHpRtr->MsgVehNew( this, 0 );
    }
}

CVehicle::~CVehicle( )
{

#ifdef _DEBUG
    m_Initialized = FALSE;
#endif

    if ( ( theApp.AmInGame( ) ) && ( GetOwner( )->IsLocal( ) ) )
    {
        GetOwner( )->AddVehsHave( -1 );

        if ( ( GetOwner( )->IsMe( ) ) && ( GetData( )->IsTransport( ) ) )
            theGame.m_pHpRtr->MsgDeleteUnit( this );

        // remove from # driving vehicles, # total
        GetOwner( )->AddPplVeh( -GetData( )->GetPeople( ) );
        ASSERT( GetOwner( )->GetPplVeh( ) >= 0 );

        if ( theGame.GetScenario( ) == 6 )
        {
            // if destroyed then shift the remainder
            for ( int iInd = 0; iInd < 5; iInd++ )
                if ( theGame.m_adwScenarioUnits[iInd] == m_dwID )
                {
                    TRAP( );
                    if ( iInd > 4 )
                        memmove( &theGame.m_adwScenarioUnits[iInd], &theGame.m_adwScenarioUnits[iInd + 1], 4 - iInd );
                    theGame.m_adwScenarioUnits[4] = 0;
                    break;
                }
        }

        // we can't see anything anymore
        if ( SpottingOn( ) )
            DecrementSpotting( );

        // update counts, build if necessary
        switch ( GetData( )->GetType( ) )
        {
        case CTransportData::construction:
            GetOwner( )->m_iNumCranes--;
            if ( GetOwner( )->m_iNumCranes <= 0 )
            {
                CBuilding* pRocket = theBuildingMap.GetBldg( GetOwner( )->m_dwIDRocket );
                if ( pRocket != NULL )
                {
                    CHexCoord    hexDest( pRocket->GetExit( CWheelTypes::track ) );
                    CMsgPlaceVeh msg( pRocket, hexDest, -1, CTransportData::construction );
                    int          iDist = CHexCoord::Dist( pRocket->GetHex( ), GetHexHead( ) );
                    if ( iDist < 30 )
                        msg.m_iDelay = ( 30 - iDist ) * ( FRAME_RATE << 4 );
                    theGame.PostToServer( &msg, sizeof( msg ) );
                    if ( GetOwner( )->IsMe( ) )
                        theGame.Event( EVENT_NEW_CRANE, EVENT_NOTIFY );
                }
            }
            break;

        case CTransportData::med_truck:
        case CTransportData::heavy_truck:
            GetOwner( )->m_iNumTrucks--;
            if ( GetOwner( )->m_iNumTrucks <= 0 )
            {
                CBuilding* pRocket = theBuildingMap.GetBldg( GetOwner( )->m_dwIDRocket );
                if ( pRocket != NULL )
                {
                    CHexCoord    hexDest( pRocket->GetExit( CWheelTypes::wheel ) );
                    CMsgPlaceVeh msg( pRocket, hexDest, -1, CTransportData::heavy_truck );
                    int          iDist = CHexCoord::Dist( pRocket->GetHex( ), GetHexHead( ) );
                    if ( iDist < 30 )
                        msg.m_iDelay = ( 30 - iDist ) * ( FRAME_RATE << 4 );
                    theGame.PostToServer( &msg, sizeof( msg ) );
                    if ( GetOwner( )->IsMe( ) )
                        theGame.Event( EVENT_NEW_TRUCK, EVENT_NOTIFY );
                }
            }
            break;
        }
    }

    // delete our route
    POSITION pos = m_route.GetHeadPosition( );
    while ( pos != NULL )
    {
        CRoute* pR = m_route.GetNext( pos );
        delete pR;
    }
    m_route.RemoveAll( );

    if ( m_cOwn )
    {
#ifdef _DEBUG
        if ( m_cMode == moving )
            ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptNext ) == this );
        ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptHead ) == this );
        ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptTail ) == this );
#endif

        // repaint
        CHexCoord _hexNext( m_ptNext );
        _hexNext.SetInvalidated( );
        CHexCoord _hexHead( m_ptHead );
        _hexHead.SetInvalidated( );
        CHexCoord _hexTail( m_ptTail );
        _hexTail.SetInvalidated( );
    }

    // we release if we hold it
    if ( theVehicleHex.GetVehicle( m_ptNext ) == this )
        theVehicleHex.ReleaseHex( m_ptNext, this );
    if ( theVehicleHex.GetVehicle( m_ptHead ) == this )
        theVehicleHex.ReleaseHex( m_ptHead, this );
    if ( theVehicleHex.GetVehicle( m_ptTail ) == this )
        theVehicleHex.ReleaseHex( m_ptTail, this );
    m_cOwn = FALSE;

    // now we play it REAL safe
    for ( int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++ )
        if ( ( SubsOwned[iInd].x != -1 ) && ( theVehicleHex.GetVehicle( SubsOwned[iInd] ) == this ) )
        {
            TRAP( );  // this is bad - a hex wasn't freed
            theVehicleHex.ReleaseHex( SubsOwned[iInd], this );
        }

        // we don't trust that we didn't leave a VehicleHex hanging
#ifdef BUGBUG
    pos = theVehicleHex.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleHex.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh == this )
        {
            ASSERT( FALSE );
            theVehicleHex.RemoveKey( dwID );
        }
    }
#endif

    // if we are being carried, remove us from the list (when ending game)
    if ( m_pTransport != NULL )
    {
        pos = m_pTransport->m_lstCargo.Find( this );
        if ( pos != NULL )
        {
            m_pTransport->m_lstCargo.RemoveAt( pos );
            if ( GetData( )->IsPeople( ) )
                m_pTransport->m_iCargoSize -= 1;
            else
                m_pTransport->m_iCargoSize -= MAX_CARGO;
        }
        m_pTransport = NULL;
    }

    // kill anyone we are carrying
    if ( theApp.AmInGame( ) )
        while ( m_lstCargo.GetCount( ) > 0 )
        {
            CVehicle* pVeh = m_lstCargo.RemoveHead( );
            pVeh->PrepareToDie( GetKiller( ) );
        }

    // correct guys pointing at us
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh->m_pTransport == this )
            pVeh->m_pTransport = NULL;
        if ( pVeh->m_pVehLoadOn == this )
            pVeh->m_pVehLoadOn = NULL;
    }

    DestroyAllWindows( );

    theVehicleMap.Remove( this );

    if ( theApp.m_wndVehicles.m_hWnd != NULL )
    {
        int iInd = theApp.m_wndVehicles.FindItem( this );
        ASSERT_STRICT( iInd >= 0 );
        if ( iInd >= 0 )
        {
            // Win32s bug
            if ( ( iInd == 0 ) && ( iWinType == W32s ) )
            {
                theApp.m_wndVehicles.m_ListBox.SetItemData( iInd, NULL );
                theApp.m_wndVehicles.m_ListBox.SetCurSel( -1 );
                theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL );
            }
            else
                theApp.m_wndVehicles.m_ListBox.DeleteString( iInd );

            // if we're done, we can delete this guy now
            if ( iWinType == W32s )
                if ( theApp.m_wndVehicles.m_ListBox.GetCount( ) == 1 )
                    if ( theApp.m_wndVehicles.m_ListBox.GetItemData( 0 ) == NULL )
                        theApp.m_wndVehicles.m_ListBox.ResetContent( );
        }
    }

    delete[] m_phexPath;
}

void CVehicle::RemoveUnit( )
{

    CUnit::RemoveUnit( );

    // track
    GetOwner( )->AddVehsHave( -1 );

    if ( !theApp.AmInGame( ) )
        return;

    // we can't see anything anymore (may be an alliance unit)
    if ( SpottingOn( ) )
        DecrementSpotting( );
    DoSpottingOff( );

    if ( !GetOwner( )->IsLocal( ) )
        return;

    // remove from # driving vehicles, # total
    GetOwner( )->AddPplVeh( -GetData( )->GetPeople( ) );

    if ( GetOwner( )->IsMe( ) )
    {
        if ( GetData( )->IsTransport( ) )
            theGame.m_pHpRtr->MsgDeleteUnit( this );

        if ( theGame.GetScenario( ) == 6 )
        {
            // if destroyed then shift the remainder
            for ( int iInd = 0; iInd < 5; iInd++ )
                if ( theGame.m_adwScenarioUnits[iInd] == m_dwID )
                {
                    TRAP( );
                    if ( iInd > 4 )
                        memmove( &theGame.m_adwScenarioUnits[iInd], &theGame.m_adwScenarioUnits[iInd + 1], 4 - iInd );
                    theGame.m_adwScenarioUnits[4] = 0;
                    break;
                }
        }

        // update counts
        switch ( GetData( )->GetType( ) )
        {
        case CTransportData::construction:
            GetOwner( )->m_iNumCranes--;
            break;

        case CTransportData::med_truck:
        case CTransportData::heavy_truck:
            GetOwner( )->m_iNumTrucks--;
            break;
        }
    }

    // delete our route
    POSITION pos = m_route.GetHeadPosition( );
    while ( pos != NULL )
    {
        CRoute* pR = m_route.GetNext( pos );
        delete pR;
    }
    m_route.RemoveAll( );

    DestroyAllWindows( );

    if ( theApp.m_wndVehicles.m_hWnd != NULL )
    {
        int iInd = theApp.m_wndVehicles.FindItem( this );
        ASSERT_STRICT( iInd >= 0 );
        if ( iInd >= 0 )
        {
            // Win32s bug
            if ( ( iInd == 0 ) && ( iWinType == W32s ) )
            {
                theApp.m_wndVehicles.m_ListBox.SetItemData( iInd, NULL );
                theApp.m_wndVehicles.m_ListBox.SetCurSel( -1 );
                theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL );
            }
            else
                theApp.m_wndVehicles.m_ListBox.DeleteString( iInd );

            // if we're done, we can delete this guy now
            if ( iWinType == W32s )
                if ( theApp.m_wndVehicles.m_ListBox.GetCount( ) == 1 )
                    if ( theApp.m_wndVehicles.m_ListBox.GetItemData( 0 ) == NULL )
                        theApp.m_wndVehicles.m_ListBox.ResetContent( );
        }
    }
}

void CVehicle::DestroyRouteWindow( )
{

    ASSERT_STRICT_VALID( this );

    if ( m_pWndRoute != NULL )
    {
        if ( m_pWndRoute->m_hWnd != NULL )
            m_pWndRoute->DestroyWindow( );
        else
            delete m_pWndRoute;
    }

    m_pWndRoute = NULL;
}

void CVehicle::DestroyBuildWindow( )
{

    ASSERT_STRICT_VALID( this );

    if ( ( m_pDlgStructure != NULL ) && ( m_pDlgStructure->m_hWnd != NULL ) )
        m_pDlgStructure->DestroyWindow( );
    else
        delete m_pDlgStructure;
    m_pDlgStructure = NULL;
}

CVehicle* CVehicle::Create( const CSubHex& ptHead, const CSubHex& ptTail, int iVeh, int iOwner, DWORD ID,
                            VEH_MODE iRouteMode, CHexCoord& hexDest, DWORD dwIDBldg, int iDelay )
{

    ASSERT( ( iVeh != CTransportData::med_truck ) && ( iVeh != CTransportData::marines ) );

    // having problems with this
    if ( theGame._GetPlayerByPlyr( iOwner ) == NULL )
    {
        TRAP( );
        return NULL;
    }

    // make an instance of a Vehicle
    CVehicle* pVeh = new CVehicle( iVeh, iOwner, ID );

    // uses some people
    pVeh->GetOwner( )->PplBldgToVeh( theTransports.GetData( iVeh )->GetPeople( ) );

    // take the count
    pVeh->GetOwner( )->IncVehsBuilt( );
    pVeh->GetOwner( )->AddVehsHave( 1 );

    // set its location
    pVeh->m_ptNext = pVeh->m_ptHead = ptHead;
    pVeh->m_ptTail                  = ptTail;
    if ( pVeh->GetData( )->GetVehFlags( ) & CTransportData::FL1hex )
        pVeh->m_ptTail = ptHead;

    pVeh->m_iDir = pVeh->CalcDir( );

    if ( pVeh->GetTurret( ) )
        pVeh->GetTurret( )->SetDir( pVeh->m_iDir );

    pVeh->m_hexDest   = hexDest;
    pVeh->m_ptDest    = CSubHex( hexDest );  // i wonder if this is correct
    pVeh->m_iSpeed    = 0;
    pVeh->m_iDestMode = sub;
    pVeh->SetLoc( TRUE );

    // set to waiting if hex(es) taken
    pVeh->m_iDelay = iDelay;
    if ( ( pVeh->m_iDelay > 0 ) || ( theBuildingHex._GetBuilding( pVeh->m_ptHead ) != NULL ) ||
         ( theBuildingHex._GetBuilding( pVeh->m_ptTail ) != NULL ) )
        iRouteMode = cant_deploy;
    else
        // blocked but not in a building - we destroy it (best I could think of)
        if ( ( theVehicleHex.GetVehicle( pVeh->m_ptHead ) != NULL ) ||
             ( theVehicleHex.GetVehicle( pVeh->m_ptTail ) != NULL ) )
        {
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_UNIT_STAT,
                       "Vehicle %d (%s) built at sub (%d,%d), must be killed built by %s", pVeh->GetID( ),
                       (LPCSTR)pVeh->GetData( )->GetDesc( ), pVeh->m_ptHead.x, pVeh->m_ptHead.y,
                       pVeh->GetOwner( )->GetName( ) );
#endif
            ASSERT( FALSE );
            pVeh->PrepareToDie( 0 );
            return ( pVeh );
        }

    // set next if moving
    if ( iRouteMode == moving )
    {
        TRAP( );
        pVeh->m_ptNext.x += pVeh->m_ptHead.x - pVeh->m_ptTail.x;
        pVeh->m_ptNext.y += pVeh->m_ptHead.y - pVeh->m_ptTail.y;
        pVeh->m_ptNext.Wrap( );
        if ( theVehicleHex.GetVehicle( pVeh->m_ptNext ) != NULL )
        {
            TRAP( );
            pVeh->m_ptNext = pVeh->m_ptHead;
        }
    }

    pVeh->_SetEventAndRoute( none, iRouteMode );
    pVeh->m_bFlags |= told_ai_stop;

    // set to waiting if hex(es) taken
    if ( iRouteMode != cant_deploy )
        pVeh->TakeOwnership( );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_UNIT_STAT,
               "Vehicle %d (%s) built at sub (%d,%d), sent to sub (%d,%d), mode %d, built by %s", pVeh->GetID( ),
               (LPCSTR)pVeh->GetData( )->GetDesc( ), pVeh->m_ptHead.x, pVeh->m_ptHead.y, pVeh->m_ptDest.x,
               pVeh->m_ptDest.y,
               pVeh->GetRouteMode( ), pVeh->GetOwner( )->GetName( ) );
#endif

    // put it at the end of the list
    theVehicleMap.Add( pVeh );

    // spotting
    if ( pVeh->DoSpotting( ) )
    {
        pVeh->m_iVisible += 1;
        if ( pVeh->m_cOwn )
        {
            pVeh->DetermineSpotting( );
            pVeh->IncrementSpotting( pVeh->GetHexHead( ) );
        }
    }

    // add to the listbox
    if ( pVeh->GetOwner( )->IsMe( ) )
        theApp.m_wndVehicles.AddToList( pVeh );

    // if we're seeable - see if we want to fight
    if ( ( pVeh->m_cOwn ) && ( pVeh->GetOwner( )->IsLocal( ) ) )
        pVeh->OppoAndOthers( );

    // send it to its dest (must follow VehNew message)
    if ( ( pVeh->GetOwner( )->IsLocal( ) ) && ( pVeh->GetHexHead( ) != hexDest ) )
    {
        if ( pVeh->GetRouteMode( ) == moving )
        {
            TRAP( );
            pVeh->KickStart( );
        }
        else
            pVeh->GetPath( FALSE );
        ASSERT( ( pVeh->m_cOwn ) || ( pVeh->m_cMode != moving ) );
    }

    if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( pVeh->GetData( )->IsTransport( ) ) )
        theGame.m_pHpRtr->MsgVehNew( pVeh, dwIDBldg );

    // scenario tracking of construction
    if ( pVeh->GetOwner( )->IsMe( ) )
        switch ( theGame.GetScenario( ) )
        {
        case 4:
            if ( pVeh->GetData( )->GetType( ) == CTransportData::light_scout )
                theGame.m_iScenarioVar |= 0x01;
            if ( pVeh->GetData( )->GetType( ) == CTransportData::construction )
                theGame.m_iScenarioVar |= 0x02;
            if ( pVeh->GetData( )->GetType( ) == CTransportData::heavy_truck )
                theGame.m_iScenarioVar |= 0x04;
            break;
        case 5:
            if ( ( pVeh->GetData( )->GetType( ) == CTransportData::light_tank ) ||
                 ( pVeh->GetData( )->GetType( ) == CTransportData::med_tank ) ||
                 ( pVeh->GetData( )->GetType( ) == CTransportData::heavy_tank ) )
                theGame.m_iScenarioVar++;
            break;
        }

    // init to legit values
    CMsgVehGoto msg( pVeh );
    pVeh->m_mvlTraffic = *( msg.ToLoc( ) );

#ifdef _DEBUG
    pVeh->m_Initialized = TRUE;
    ASSERT_VALID_LOC( pVeh );
    ASSERT_STRICT_VALID( &theVehicleMap );
    ASSERT_STRICT( pVeh->m_dwTimeBlocked < 24 * 20 );
#endif

    ASSERT_VALID( pVeh );
    return ( pVeh );
}

CDlgBuildStructure* CVehicle::GetDlgBuild( )
{

    if ( m_pDlgStructure == NULL )
        m_pDlgStructure = new CDlgBuildStructure( theApp.m_pMainWnd, this );
    ASSERT_STRICT_VALID( m_pDlgStructure );

    if ( m_pDlgStructure->m_hWnd == NULL )
        m_pDlgStructure->Create( theApp.m_pMainWnd );

    m_pDlgStructure->ShowWindow( SW_SHOWNORMAL );
    m_pDlgStructure->SetFocus( );

    return ( m_pDlgStructure );
}

void CVehicle::StartConst( CBuilding* pBldg )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID_OR_NULL( pBldg );

    m_lOperMod = 0;

    m_pBldg = pBldg;

    if ( pBldg == NULL )
        m_cMode = stop;
    else
    {
        // see if we are there yet
        if ( theBuildingHex._GetBuilding( m_ptHead ) != pBldg )
        {
            ASSERT( !GetOwner( )->IsLocal( ) );
            ReleaseOwnership( );
            m_ptDest = m_ptNext = m_ptHead = m_ptTail = m_hexDest = pBldg->GetHex( );
        }

        SetBuilding( pBldg->GetHex( ), pBldg->GetData( )->GetType( ), pBldg->GetDir( ) );
        _SetEventAndRoute( build, run );
        EnterBuilding( );

        // tell the router we need materials (mainly for repair)
        if ( pBldg->GetOwner( )->IsMe( ) )
            theGame.m_pHpRtr->MsgOutMat( pBldg );
        else
            pBldg->MaterialMessage( );
    }
}

void CVehicle::StopConstruction( CBuilding* pBldg )
{

    ASSERT_STRICT_VALID( pBldg );

    POSITION pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh->m_pBldg == pBldg )
        {
            pVeh->m_pBldg = NULL;
            pVeh->SetEvent( none );
            pVeh->ExitBuilding( );
            pVeh->MaterialChange( );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CUnitMaps

void CBuildingMap::Add( CBuilding* pBldg )
{

#ifdef _STRICT
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pBldg );
    CBuilding* pTest;
    ASSERT_STRICT( !Lookup( pBldg->GetID( ), pTest ) );
#endif

    SetAt( pBldg->GetID( ), pBldg );
}

void CBuildingMap::Remove( CBuilding* pBldg )
{

#ifdef _STRICT
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pBldg );
    CBuilding* pTest;
    ASSERT_STRICT( Lookup( pBldg->GetID( ), pTest ) );
    ASSERT_STRICT( pTest == pBldg );
#endif

    RemoveKey( pBldg->GetID( ) );
}

void CVehicleMap::Add( CVehicle* pVeh )
{

#ifdef _STRICT
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pVeh );
    CVehicle* pTest;
    ASSERT_STRICT( !Lookup( pVeh->GetID( ), pTest ) );
#endif

    SetAt( pVeh->GetID( ), pVeh );
}

void CVehicleMap::Remove( CVehicle* pVeh )
{

#ifdef _STRICT
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pVeh );
    CVehicle* pTest;
    ASSERT_STRICT( Lookup( pVeh->GetID( ), pTest ) );
    ASSERT_STRICT( pTest == pVeh );
#endif

    RemoveKey( pVeh->GetID( ) );
}


void CUnit::Serialize( CArchive& ar )
{

    CUnitTile::Serialize( ar );

    m_flag.Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        ar << m_dwID;
        ar << (WORD)m_pOwner->GetPlyrNum( );
        ar << m_maploc;
        if ( m_pUnitTarget == NULL )
            ar << (DWORD)0;
        else
            ar << m_pUnitTarget->GetID( );
        if ( m_pUnitOppo == NULL )
            ar << (DWORD)0;
        else
            ar << m_pUnitOppo->GetID( );
        ar << (DWORD)m_unitFlags;
        ar << m_ptTarget;
        ar << m_ptUs;
        ar << m_iLOS;
        ar << m_dwReloadMod;
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) ar << m_aiStore[iInd];
        ar << m_iDamagePoints;
        ar << m_iDamagePer;
        ar << m_fDamageMult;
        ar << m_fDamPerfMult;

        ar << m_iVisible;

        ar << m_iSpottingRange;
        ar << m_iRange;
        for ( int iInd = 0; iInd < CUnitData::num_attacks; iInd++ ) ar << m_iAttack[iInd];
        ar << m_iDefense;
        ar << m_iAccuracy;
        ar << m_iFireRate;

        BYTE byTurret = NULL != GetTurret( );

        ar << byTurret;

        if ( byTurret )
            GetTurret( )->Serialize( ar );
    }

    else
    {
        ar >> m_dwID;
        WORD w;
        ar >> w;
        m_pOwner = theGame.GetPlayerByPlyr( w );
        ar >> m_maploc;
        m_maploc.Wrap( );
        DWORD dw;
        ar >> dw;
        m_pUnitTarget = (CUnit*)dw;
        ar >> dw;
        m_pUnitOppo = (CUnit*)dw;
        ar >> dw;
        m_unitFlags = (UNIT_FLAGS)dw;
        ar >> m_ptTarget;
        ar >> m_ptUs;
        ar >> m_iLOS;
        ar >> m_dwReloadMod;
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) ar >> m_aiStore[iInd];
        ar >> m_iDamagePoints;
        ar >> m_iDamagePer;
        ar >> m_fDamageMult;
        ar >> m_fDamPerfMult;
        m_iLastShowDamagePer = 100;  // force a redraw

        ar >> m_iVisible;

        ar >> m_iSpottingRange;
        ar >> m_iRange;
        for ( int iInd = 0; iInd < CUnitData::num_attacks; iInd++ ) ar >> m_iAttack[iInd];
        ar >> m_iDefense;
        ar >> m_iAccuracy;
        ar >> m_iFireRate;

        BYTE byTurret;

        ar >> byTurret;

        if ( byTurret )
        {
            m_ptrturret = new CTurret( );

            m_ptrturret->Serialize( ar );
        }
        else
            m_ptrturret = NULL;
    }
}

void CUnit::FixUp( )
{

    // update the damage level
    CDamageDisplay* pDd = GetDamageDisplay( );
    if ( pDd )
        pDd->SetDamage( min( 100, 100 - m_iDamagePer ) );

    if ( m_pUnitTarget != NULL )
        m_pUnitTarget = ::GetUnit( (DWORD)m_pUnitTarget );
    if ( m_pUnitOppo != NULL )
        m_pUnitOppo = ::GetUnit( (DWORD)m_pUnitOppo );

#ifdef _DEBUG
    m_Initialized = TRUE;
#endif
}

void CBuilding::Serialize( CArchive& ar )
{

    CUnit::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );
        ar << (WORD)theStructures.GetIndex( (CStructureData*)m_pUnitData );

        ar << m_hex;
        ar << m_iBldgDir;
        ar << m_iLastPer;
        ar << m_iBuildDone;
        ar << m_iConstDone;
        ar << m_iFoundTime;
        ar << m_iFoundPer;
        ar << m_iSkltnPer;
        ar << m_iFinalPer;
        ar << m_iLastMaterialTake;
        ar << m_iRepairWork;
        for ( int iOn = 0; iOn < CMaterialTypes::num_build_types; iOn++ ) ar << m_aiRepair[iOn];
        ar << m_iMatTo;
        ar << m_fOperMod;

        ar << m_hexExit;
        ar << m_hexShip;
        ar << (LONG)m_iExitDir;
        ar << (LONG)m_iShipDir;

        ar << m_iVisConstDone << m_iVisFoundPer << m_iVisSkltnPer << m_iVisFinalPer;

        ar << m_cx << m_cy;
    }

    else
    {
        WORD w;
        ar >> w;
        m_pUnitData = theStructures.GetData( w );

        if ( 0 <= m_iSpriteID )
            m_psprite = theStructures.GetSprite( m_iSpriteID );

        ar >> m_hex;
        m_hex.Wrap( );
        ar >> m_iBldgDir;
        ar >> m_iLastPer;
        ar >> m_iBuildDone;
        ar >> m_iConstDone;
        ar >> m_iFoundTime;
        ar >> m_iFoundPer;
        ar >> m_iSkltnPer;
        ar >> m_iFinalPer;
        ar >> m_iLastMaterialTake;
        ar >> m_iRepairWork;
        for ( int iOn = 0; iOn < CMaterialTypes::num_build_types; iOn++ ) ar >> m_aiRepair[iOn];
        ar >> m_iMatTo;
        ar >> m_fOperMod;

        ar >> m_hexExit;
        ar >> m_hexShip;
        LONG l;
        ar >> l;
        m_iExitDir = l;
        ar >> l;
        m_iShipDir = l;

        ar >> m_iVisConstDone >> m_iVisFoundPer >> m_iVisSkltnPer >> m_iVisFinalPer;

        ar >> m_cx >> m_cy;

        AssignToHex( m_hex, theMap.GetHex( m_hex )->GetAlt( ) );
        theBuildingMap.Add( this );
    }
}

void CBuilding::FixForPlayer( )
{

    CUnit::FixForPlayer( );

    // get the spotting and oppo fire going
    if ( GetOwner( )->IsMe( ) )
    {
        theApp.m_wndBldgs.AddToList( this );
        MakeBldgVisible( );

        DetermineSpotting( );
        IncrementSpotting( m_hex );

        // animate if operating
        AnimateOperating( IsOperating( ) );
    }

    if ( GetOwner( )->IsLocal( ) )
        OppoAndOthers( );
}

void CMineBuilding::Serialize( CArchive& ar )
{

    CBuilding::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );
        ar << m_iDensity;
        ar << m_iMinerals;
        ar << m_fAmountMod;
    }
    else
    {
        ar >> m_iDensity;
        ar >> m_iMinerals;
        ar >> m_fAmountMod;
    }
}

void CFarmBuilding::Serialize( CArchive& ar )
{
    CBuilding::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );
        ar << m_iTerMult;
    }
    else
        ar >> m_iTerMult;
}

void CVehicleBuilding::Serialize( CArchive& ar )
{

    CBuilding::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );
        for ( int iInd = 0; iInd < CMaterialTypes::num_build_types; iInd++ ) ar << m_aiUsed[iInd];

        if ( m_pBldUnt == NULL )
            ar << (LONG)0;
        else
            ar << (LONG)m_pBldUnt->GetVehType( );
    }

    else
    {
        for ( int iInd = 0; iInd < CMaterialTypes::num_build_types; iInd++ ) ar >> m_aiUsed[iInd];

        LONG iIndex;
        ar >> iIndex;
        if ( iIndex == 0 )
            m_pBldUnt = NULL;
        else
            AssignBldUnit( iIndex );
    }
}

//---------------------------------------------------------------------------
// CMuzzleFlash::CMuzzleFlash
//---------------------------------------------------------------------------
CMuzzleFlash::CMuzzleFlash( )
{
    EnableAnimations( FALSE );
}

//---------------------------------------------------------------------------
// CMuzzleFlash::Serialize
//---------------------------------------------------------------------------
void CMuzzleFlash::Serialize( CArchive& ar )
{
    CUnitTile::Serialize( ar );

    if ( !ar.IsStoring( ) )
        m_psprite = theFlashes.GetSprite( m_iSpriteID );
}

//---------------------------------------------------------------------------
// CTurret::Serialize
//---------------------------------------------------------------------------
void CTurret::Serialize( CArchive& ar )
{
    CUnitTile::Serialize( ar );

    m_muzzleflash.Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        ar << m_iDir;
    }
    else
    {
        if ( 0 <= m_iSpriteID )
            m_psprite = theTurrets.GetSprite( m_iSpriteID );

        ar >> m_iDir;
    }
}

//---------------------------------------------------------------------------
// CFlag::Serialize
//---------------------------------------------------------------------------
void CFlag::Serialize( CArchive& ar )
{
    CEffectTile::Serialize( ar );  // Assigns the sprite, if any

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        ar << (LONG)m_iPlayer;
    }
    else
    {
        LONG l;
        ar >> l;

        m_iPlayer = l;
    }
}

void CVehicle::Serialize( CArchive& ar )
{

    CUnit::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );

        ar << (WORD)theTransports.GetIndex( (CTransportData*)m_pUnitData );

        ar << (WORD)m_route.GetCount( );
        POSITION pos  = m_route.GetHeadPosition( );
        int      iPos = 0, iOn = 0;
        TRAP( m_route.GetCount( ) > 0 );
        while ( pos != NULL )
        {
            CRoute* pR = m_route.GetNext( pos );
            pR->Serialize( ar );
            if ( pos == m_pos )
                iPos = iOn;
            iOn++;
        }
        ar << (WORD)iPos;

        ar << m_ptDest;
        ar << m_hexDest;
        ar << m_hexLastDest;
        ar << (BYTE)m_iDestMode;
        ar << m_cOwn;
        ar << (BYTE)m_cMode;

        if ( m_pBldg == NULL )
            ar << (DWORD)0;
        else
            ar << m_pBldg->GetID( );
        ar << (BYTE)m_iEvent;
        ar << m_hexBldg;
        ar << m_iBldgType;
        ar << m_iBuildDir;

        ar << m_hexStart;
        ar << m_hexEnd;
        ar << m_iBuildDone;
        ar << m_iLastPer;

        ar << m_ptNext;
        ar << m_ptHead;
        ar << m_ptTail;
        ar << m_hexNext;
        ar << m_iStepsLeft;
        ar << m_iDir;

        ar << m_iSpeed;
        ar << m_iXadd;
        ar << m_iYadd;
        ar << m_iDadd << m_iTadd;

        ar << m_dwTimeBlocked;
        ar << m_bFlags;
        ar << m_lOperMod;

        if ( m_pTransport == NULL )
            ar << (DWORD)0;
        else
            ar << m_pTransport->GetID( );
        ar << (DWORD)m_iCargoSize;
        if ( m_pVehLoadOn == NULL )
            ar << (DWORD)0;
        else
            ar << m_pVehLoadOn->GetID( );

        ar << (LONG)m_iPathLen;
        ar << m_iPathOff;
        for ( int iInd = 0; iInd < m_iPathLen; iInd++ ) ar << *( m_phexPath + iInd );
    }

    else
    {
        WORD w;
        ar >> w;
        m_pUnitData = theTransports.GetData( w );

        if ( 0 <= m_iSpriteID )
            m_psprite = theTransports.GetSprite( m_iSpriteID );

        LONG l;

        WORD wNR;
        ar >> wNR;
        while ( wNR-- > 0 )
        {
            CRoute* pR = new CRoute( );
            pR->Serialize( ar );
            m_route.AddTail( pR );
        }

        // get m_pos
        ar >> wNR;
        m_pos = NULL;
        if ( m_route.GetCount( ) > 0 )
        {
            POSITION pos = m_route.GetHeadPosition( );
            while ( pos != NULL )
            {
                if ( wNR <= 0 )
                {
                    m_pos = pos;
                    break;
                }
                wNR--;
                m_route.GetNext( pos );
            }
        }

        ar >> m_ptDest;
        ar >> m_hexDest;
        ar >> m_hexLastDest;
        BYTE b;
        ar >> b;
        m_iDestMode = (VEH_POS)b;
        ar >> m_cOwn;
        ar >> b;
        m_cMode = (VEH_MODE)b;

        DWORD dw;
        ar >> dw;
        m_pBldg = (CBuilding*)dw;
        ar >> b;
        m_iEvent = (VEH_EVENT)b;
        ar >> m_hexBldg;
        ar >> m_iBldgType;
        ar >> m_iBuildDir;

        ar >> m_hexStart;
        ar >> m_hexEnd;
        ar >> m_iBuildDone;
        ar >> m_iLastPer;

        ar >> m_ptNext;
        ar >> m_ptHead;
        ar >> m_ptTail;
        ar >> m_hexNext;
        ar >> m_iStepsLeft;
        ar >> m_iDir;

        ar >> m_iSpeed;
        ar >> m_iXadd;
        ar >> m_iYadd;
        ar >> m_iDadd >> m_iTadd;

        ar >> m_dwTimeBlocked;
        ar >> m_bFlags;
        m_bFlags = 0;
        m_bFlags &= ~( hp_controls | told_ai_stop );
        ar >> m_lOperMod;

        ar >> dw;
        m_pTransport = (CVehicle*)dw;
        ar >> dw;
        m_iCargoSize = dw;
        ar >> dw;
        m_pVehLoadOn = (CVehicle*)dw;

        ar >> l;
        m_iPathLen = l;
        ar >> m_iPathOff;
        if ( m_iPathLen == 0 )
            m_phexPath = NULL;
        else
        {
            m_phexPath = new CHexCoord[m_iPathLen];
            for ( int iInd = 0; iInd < m_iPathLen; iInd++ ) ar >> *( m_phexPath + iInd );
        }

        m_ptDest.Wrap( );
        m_hexDest.Wrap( );
        m_hexLastDest.Wrap( );
        m_hexBldg.Wrap( );
        m_hexStart.Wrap( );
        m_hexEnd.Wrap( );
        m_ptNext.Wrap( );
        m_ptHead.Wrap( );
        m_ptTail.Wrap( );
        m_hexNext.Wrap( );

        if ( m_cOwn )
        {
            ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptHead ) == NULL );
            ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptTail ) == NULL );
            theVehicleHex.GrabHex( m_ptHead, this );
            if ( m_ptTail != m_ptHead )
                theVehicleHex.GrabHex( m_ptTail, this );
            if ( ( m_cMode == moving ) && ( m_ptNext != m_ptHead ) )
            {
                ASSERT_STRICT( theVehicleHex.GetVehicle( m_ptNext ) == NULL );
                theVehicleHex.GrabHex( m_ptNext, this );
            }
        }
        theVehicleMap.Add( this );
    }
}

void CVehicle::FixUp( )
{

    CUnit::FixUp( );

    if ( m_pBldg != NULL )
        m_pBldg = theBuildingMap.GetBldg( (DWORD)m_pBldg );

    if ( m_pTransport != NULL )
    {
        m_pTransport = theVehicleMap.GetVehicle( (DWORD)m_pTransport );
        if ( m_pTransport == this )
            m_pTransport = NULL;
        if ( m_pTransport != NULL )
        {
            m_pTransport->m_lstCargo.AddTail( this );
            if ( GetData( )->IsPeople( ) )
                m_pTransport->m_iCargoSize += 1;
            else
                m_pTransport->m_iCargoSize += MAX_CARGO;
        }
    }

    TRAP( m_pVehLoadOn != NULL );
    if ( m_pVehLoadOn != NULL )
        m_pVehLoadOn = theVehicleMap.GetVehicle( (DWORD)m_pVehLoadOn );
}

void CVehicle::FixForPlayer( )
{

    CUnit::FixForPlayer( );

    if ( GetOwner( )->IsMe( ) )
    {
        theApp.m_wndVehicles.AddToList( this );

        DoSpottingOn( );
        IncVisible( );

        if ( m_cOwn )
        {
            DetermineSpotting( );
            IncrementSpotting( GetHexHead( ) );
        }
    }

    if ( ( m_cOwn ) && ( GetOwner( )->IsLocal( ) ) )
        OppoAndOthers( );
}

void CRoute::Serialize( CArchive& ar )
{

    CObject::Serialize( ar );

    if ( ar.IsStoring( ) )
    {
        ASSERT_STRICT_VALID( this );
        ar << m_hex << m_iType;
    }

    else
        ar >> m_hex >> m_iType;
}

void SerializeElements( CArchive& ar, CBuilding** ppBldg, int iCount )
{

    TRAP( );
    while ( iCount-- )
    {
        ( *ppBldg )->Serialize( ar );
        ppBldg++;
    }
}

void SerializeElements( CArchive& ar, CVehicle** ppVeh, int iCount )
{

    TRAP( );
    while ( iCount-- )
    {
        ( *ppVeh )->Serialize( ar );
        ppVeh++;
    }
}

void SerializeElements( CArchive& ar, CRoute** ppRt, int iCount )
{

    while ( iCount-- )
    {
        *ppRt = new CRoute( );
        ( *ppRt )->Serialize( ar );
        ppRt++;
    }
}


/////////////////////////////////////////////////////////////////////////////
// all unit.h asserts

#ifdef _DEBUG
void CUnitData::AssertValid( ) const
{

    // assert the base class
    CObject::AssertValid( );

    if ( theRsrch.GetSize( ) > 0 )
        for ( int iOn = 0; iOn < 4; iOn++ )
            ASSERT( ( 0 <= m_iRsrchReq[iOn] ) && ( m_iRsrchReq[iOn] < theRsrch.GetSize( ) ) );

    ASSERT( ( m_udFlags & ~0x1F ) == 0 );
    ASSERT( ( 0 <= m_iPeople ) && ( m_iPeople < 192 ) );
    ASSERT( ( 0 <= m_iSpottingRange ) && ( m_iSpottingRange <= 10 ) );
    ASSERT( ( 1 <= m_iDamagePoints ) && ( m_iDamagePoints < 36000 ) );
    ASSERT( ( 0 <= m_iRange ) && ( m_iRange <= 32 ) );
    for ( int iOn = 0; iOn < num_attacks; iOn++ ) ASSERT( ( 0 <= m_iAttack[iOn] ) && ( m_iAttack[iOn] < 5000 ) );
    ASSERT( ( 0 <= m_iTargetType ) && ( m_iTargetType < CUnitData::num_attacks ) );
    ASSERT( ( 0 <= m_iDefense ) && ( m_iDefense < 40 ) );
    ASSERT( ( 0 <= m_iFireRate ) && ( m_iFireRate < 1440 ) );
    ASSERT( ( 0 <= m_iAccuracy ) && ( m_iAccuracy < 20 ) );
    ASSERT_VALID_CSTRING( &m_sDesc );
    ASSERT_VALID_CSTRING( &m_sText );
    ASSERT( ( 1 <= m_cx ) && ( m_cx <= 5 ) );
    ASSERT( ( 1 <= m_cy ) && ( m_cy <= 5 ) );
    ASSERT( ( 0 <= m_iSoundIdle ) && ( m_iSoundIdle < 200 ) );
    ASSERT( ( 0 <= m_iSoundRun ) && ( m_iSoundRun < 200 ) );
    ASSERT( ( 0 <= m_dwIDProj ) && ( m_dwIDProj < 200 ) );
}

void CStructureType::AssertValid( ) const
{

    CObject::AssertValid( );
}

void CStructure::AssertValid( ) const
{

    // assert the base class
    CSpriteStore<CStructureSprite>::AssertValid( );

    ASSERT( this == &theStructures );

    // if not init'ed yet, return
    if ( GetCount( ) == 0 )
        return;

    CStructureData** ppSd = m_ppData;
    for ( int iOn = 0; iOn < theStructures.m_iNumBuildings; iOn++, ppSd++ ) ASSERT_VALID( *ppSd );
}

void CBuildMaterials::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );

    ASSERT( m_iUnionType == CStructureData::UTmaterials );
    // BUGBUG - more
}

void CBuildUnit::AssertValid( ) const
{

    // assert base object
    CObject::AssertValid( );

    // BUGBUG - more
}

void CBuildVehicle::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );

    ASSERT( m_iUnionType == CStructureData::UTvehicle );
    ASSERT( m_aBldUnits.GetSize( ) > 0 );
    ASSERT_VALID( &m_aBldUnits );
}

void CBuildHousing::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );

    ASSERT( m_iUnionType == CStructureData::UThousing );
    // BUGBUG - more
}

void CBuildCommand::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTcommand );
}

void CBuildEmbassy::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTembassy );
}

void CBuildFort::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTfort );
}

void CBuildPower::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTpower );

    ASSERT( m_iPower <= 1200 );
}

void CBuildResearch::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTresearch );
}

void CBuildRepair::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTrepair );
}

void CBuildMine::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTmine );

#ifndef _GG
    ASSERT( m_iTime <= 60 * 24 );
    ASSERT( m_iAmount <= 1500 );
    ASSERT( m_iOutput <= CMaterialTypes::num_types );
#endif
}

void CBuildWarehouse::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTwarehouse );
}

void CBuildFarm::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTfarm );
}

void CBuildShipyard::AssertValid( ) const
{

    // assert base object
    CStructureData::AssertValid( );
    ASSERT( m_iUnionType == CStructureData::UTshipyard );
}

void CStructureData::AssertValid( ) const
{

    // assert the base class
    CUnitData::AssertValid( );

    ASSERT( ( -1 <= m_iUnionType ) && ( m_iUnionType < num_union_types ) );
    ASSERT( ( -1 <= m_iType ) && ( m_iType < num_types ) );
    ASSERT( ( -1 <= m_iBldgCat ) && ( m_iBldgCat < dlg_num_items ) );
    ASSERT( this == *( theStructures.m_ppData + m_iType ) );

    // was failing, dunno why lol
    // VTFIXME
   // ASSERT( ( m_bFlags & 0x07 ) == m_bFlags );

    ASSERT( ( 0 <= m_hexExit.X( ) ) && ( m_hexExit.X( ) < m_cx ) );
    ASSERT( ( 0 <= m_hexExit.Y( ) ) && ( m_hexExit.Y( ) < m_cy ) );
    ASSERT( ( 0 <= m_iExitDir ) && ( m_iExitDir < 4 ) );

// GGFIXIT #ifndef _GG
#if 0
    ASSERT (((m_iExitDir == 0) && (m_hexExit.Y() == 0)) ||
                                                            ((m_iExitDir == 1) && (m_hexExit.X() == m_cx-1)) ||
                                                            ((m_iExitDir == 2) && (m_hexExit.Y() == m_cy-1)) ||
                                                            ((m_iExitDir == 3) && (m_hexExit.X() == 0)));
#endif
    ASSERT( ( 0 <= m_hexShip.X( ) ) && ( m_hexShip.X( ) < m_cx ) );
    ASSERT( ( 0 <= m_hexShip.Y( ) ) && ( m_hexShip.Y( ) < m_cy ) );
    ASSERT( ( ( m_iShipDir == 0 ) && ( m_hexShip.Y( ) == 0 ) ) ||
            ( ( m_iShipDir == 1 ) && ( m_hexShip.X( ) == m_cx - 1 ) ) ||
            ( ( m_iShipDir == 2 ) && ( m_hexShip.Y( ) == m_cy - 1 ) ) ||
            ( ( m_iShipDir == 3 ) && ( m_hexShip.X( ) == 0 ) ) );
}

void CTransport::AssertValid( ) const
{

    // assert the base class
    CSpriteStore<CVehicleSprite>::AssertValid( );

    ASSERT( this == &theTransports );

    // if not init'ed yet, return
    if ( GetCount( ) == 0 )
        return;

    CTransportData const* pTd = m_pData;
    for ( int iOn = 0; iOn < m_iNumTransports; iOn++, pTd++ ) ASSERT_VALID( pTd );
}

void CTransportData::AssertValid( ) const
{

    // assert the base class
    CUnitData::AssertValid( );

    ASSERT( ( 0 <= m_iType ) && ( m_iType < num_types ) );
    ASSERT( ( 0 <= m_iSetupFire ) && ( m_iSetupFire < 288 ) );
    ASSERT( ( 0 <= m_iPeopleCarry ) && ( m_iPeopleCarry < 5 * MAX_CARGO ) );
    ASSERT( ( 0 <= m_iSpeed ) && ( m_iSpeed < 52 ) );
    ASSERT( ( 0 <= m_cWheelType ) && ( m_cWheelType < NUM_WHEEL_TYPES ) );
    ASSERT( ( 0 <= m_cWaterDepth ) && ( m_cWaterDepth <= 16 ) );

    ASSERT( this == theTransports.m_pData + m_iType );
}

void CUnit::AssertValid( ) const
{

    // assert base object
    CTile::AssertValid( );

    if ( !m_Initialized )
        return;

    ASSERT( m_dwID > 0 );
    ASSERT_VALID( m_pUnitData );
    ASSERT( ( m_iUnitType == building ) || ( m_iUnitType == vehicle ) );
    ASSERT( ( 0 <= m_maploc.x ) && ( m_maploc.x < theMap.m_eX * MAX_HEX_HT ) );
    ASSERT( ( 0 <= m_maploc.y ) && ( m_maploc.y < theMap.m_eY * MAX_HEX_HT ) );
    // circular if attacking each other	ASSERT_VALID_OR_NULL (m_pUnitTarget);
    // circular if attacking each other	ASSERT_VALID_OR_NULL (m_pUnitOppo);
    ASSERT_VALID_STRUCT( &m_ptTarget );
    ASSERT_VALID_STRUCT( &m_ptUs );
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ ) ASSERT( m_aiStore[iOn] >= 0 );
    ASSERT( ( 0 <= m_iDamagePoints ) && ( m_iDamagePoints <= m_pUnitData->m_iDamagePoints ) );
    ASSERT( ( 0 <= m_iDamagePer ) && ( m_iDamagePer <= 100 ) );
    ASSERT( ( 0 <= m_fDamageMult ) && ( m_fDamageMult <= 1 ) );
    ASSERT( ( 0.5 <= m_fDamPerfMult ) && ( m_fDamPerfMult <= 1 ) );
#ifdef STRICTER_ASSERTS
    ASSERT( ( m_unitFlags & ~0x1F ) == 0 ); // this was failing for loaded games, but went away eventually...
#endif
}

void CBuilding::AssertValid( ) const
{

    // assert base object
    CUnit::AssertValid( );
    ASSERT( m_iUnitType == CUnit::building );

    if ( !m_Initialized )
        return;

    ASSERT_HEX_COORD( &m_hex );
    ASSERT_VALID( m_pUnitData );
    CBuilding* pBldg;
    ASSERT( ( theBuildingMap.Lookup( m_dwID, pBldg ) != NULL ) && ( pBldg == this ) );

    ASSERT_HEX_COORD( &m_hexExit );
    ASSERT_HEX_COORD( &m_hexShip );
    ASSERT( ( 0 <= m_iExitDir ) && ( m_iExitDir < 4 ) );
    ASSERT( ( 0 <= m_iShipDir ) && ( m_iShipDir < 4 ) );

    ASSERT( ( 0 <= m_iBldgDir ) && ( m_iBldgDir < 4 ) );
    if ( m_iBldgDir & 1 )
        ASSERT( ( m_cx == m_pUnitData->m_cy ) && ( m_cy == m_pUnitData->m_cx ) );
    else
        ASSERT( ( m_cx == m_pUnitData->m_cx ) && ( m_cy == m_pUnitData->m_cy ) );

    for ( int _x = m_hex.X( ); _x < m_hex.X( ) + m_cx; _x++ )
        for ( int _y = m_hex.Y( ); _y < m_hex.Y( ) + m_cy; _y++ )
        {
            CHexCoord _hex( _x, _y );
            _hex.Wrap( );
            ASSERT( theBuildingHex.GetBuilding( _hex ) == this );
        }

    // BUGBUG - Doesn't work when hexes cross map boundary
    // ASSERT (m_maploc.x == m_hex.X () * MAX_HEX_HT + (m_pUnitData->m_cx * MAX_HEX_HT) / 2);
    // ASSERT (m_maploc.y == m_hex.Y () * MAX_HEX_HT + (m_pUnitData->m_cy * MAX_HEX_HT) / 2);

    // BUGBUG - fix this up
    //	ASSERT ((-1 <= m_iVehBuild) && (m_iVehBuild < NUM_TRANSPORTS));
    //	ASSERT ((-1 <= m_iBuildDone) && (m_iBuildDone <= BUILD_DONE));
}

void CVehicleBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTvehicle ) ||
            ( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTshipyard ) );

    ASSERT_VALID_OR_NULL( m_pBldUnt );
}

void CMaterialBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTmaterials );
}

void CPowerBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTpower );
}

void CRepairBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTrepair );
    ASSERT_VALID_OR_NULL( m_pVehRepairing );
}

void CShipyardBuilding::AssertValid( ) const
{

    // assert base object
    CVehicleBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTshipyard );
    ASSERT_VALID_OR_NULL( m_pVehRepairing );
}

void CWarehouseBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTwarehouse );
}

void CMineBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTmine );
}

void CFarmBuilding::AssertValid( ) const
{

    // assert base object
    CBuilding::AssertValid( );
    ASSERT( ( (CStructureData*)m_pUnitData )->m_iUnionType == CStructureData::UTfarm );
}

void CVehicle::AssertValid( ) const
{

    // assert base object
    CUnit::AssertValid( );
    ASSERT( m_iUnitType == CUnit::vehicle );

    if ( !m_Initialized )
        return;

    ASSERT( ( 0 <= m_cMode ) && ( m_cMode < num_mode ) );
    ASSERT( ( 0 <= m_iEvent ) && ( m_iEvent < num_event ) );
    ASSERT( ( 0 <= m_iDir ) && ( m_iDir < FULL_ROT ) );
    if ( m_phexPath != NULL )
        ASSERT( AfxIsValidAddress( m_phexPath, sizeof( CHexCoord ) * m_iPathLen ) );
    ASSERT( ( 0 <= m_iPathOff ) && ( m_iPathOff <= m_iPathLen ) );
    ASSERT_VALID_OR_NULL( m_pBldg );
    CVehicle* pVeh;
    ASSERT( ( theVehicleMap.Lookup( m_dwID, pVeh ) != NULL ) && ( pVeh == this ) );

    if ( ( m_iPathLen > 0 ) && ( m_iPathOff > 1 ) )
    {
#ifndef _GG
        ASSERT( m_iPathOff <= m_iPathLen );
#endif
    }

    ASSERT_HEX_COORD( &m_hexDest );
    ASSERT_VALID_STRUCT( &m_ptHead );
    ASSERT_VALID_STRUCT( &m_ptTail );
#ifndef _GG
#if STRICTER_ASSERTS
    if ( GetData( )->GetVehFlags( ) & CTransportData::FL1hex )
        ASSERT( m_ptHead == m_ptTail );
    else
        ASSERT( m_ptHead != m_ptTail );
    ASSERT( ( abs( CSubHex::Diff( m_ptNext.x - m_ptHead.x ) ) < 2 ) &&
            ( abs( CSubHex::Diff( m_ptNext.y - m_ptHead.y ) ) < 2 ) );
    ASSERT( ( abs( CSubHex::Diff( m_ptHead.x - m_ptTail.x ) ) < 2 ) &&
            ( abs( CSubHex::Diff( m_ptHead.y - m_ptTail.y ) ) < 2 ) );
#endif
#endif

    CHexCoord _hex( m_ptHead );
    CHex*     pHex = theMap._GetHex( _hex );
    // BUGBUG - landing craft	ASSERT ((theTerrain.GetData (pHex->GetType ()).m_iWheelMult [((CTransportData*)m_pUnitData)->m_cWheelType] > 0) ||
    //									((((CTransportData*)m_pUnitData)->m_cWheelType == CWheelTypes::water) &&
    //									(theBuildingHex.GetBuilding (_hex) != NULL)));

    if ( m_cOwn )
    {
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) == this );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) == this );
        BYTE bUn  = theMap._GetHex( m_ptHead.x / 2, m_ptHead.y / 2 )->GetUnits( );
        BYTE bVal = 1 << ( ( m_ptHead.x & 1 ) + ( ( m_ptHead.y & 1 ) * 2 ) );
        ASSERT( bUn & bVal );
        bUn  = theMap._GetHex( m_ptTail.x / 2, m_ptTail.y / 2 )->GetUnits( );
        bVal = 1 << ( ( m_ptTail.x & 1 ) + ( ( m_ptTail.y & 1 ) * 2 ) );
        ASSERT( bUn & bVal );

        if ( m_cMode == moving )
        {
            ASSERT_HEX_COORD( &m_hexLastDest );
            ASSERT_VALID_STRUCT( &m_ptNext );
            ASSERT_HEX_COORD( &m_hexNext );
            ASSERT( m_hexDest.SameHex( m_ptDest ) );
        }

        // check spotting
        if ( m_bSpotted )
        {
            DWORD const* pdwSpot = m_dwaSpot;
            CHexCoord    _hex( m_ptHead.x / 2 - MAX_SPOTTING, m_ptHead.y / 2 - MAX_SPOTTING );
            _hex.Wrap( );
            int  x        = _hex.X( );
            int  iNumBlks = SPOTTING_ARRAY_SIZE;
            BOOL bSpots   = FALSE;

            while ( iNumBlks-- > 0 )
            {
                if ( *pdwSpot != 0 )
                {
                    bSpots     = TRUE;
                    int   iInd = 1;
                    CHex* pHex = theMap._GetHex( _hex );

                    for ( int iNum = 32; iNum > 0; iNum-- )
                    {
                        if ( *pdwSpot & iInd )
                            ASSERT( pHex->GetVisibility( ) );
                        iInd <<= 1;
                        _hex.Xinc( );
                        if ( _hex.X( ) == 0 )
                            pHex = theMap._GetHex( _hex );
                        else
                            pHex = theMap._Xinc( pHex );
                    }
                }

                _hex.X( ) = x;
                _hex.Yinc( );
                pdwSpot++;
            }

            if ( ( bSpots ) && ( pVeh->GetOwner( )->IsMe( ) ) )
                ASSERT( theMap._GetHex( m_ptHead )->GetVisibility( ) );
        }
    }

    else
    {
        ASSERT( theVehicleHex.GetVehicle( m_ptNext ) != this );
        ASSERT( theVehicleHex.GetVehicle( m_ptHead ) != this );
        ASSERT( theVehicleHex.GetVehicle( m_ptTail ) != this );
    }
}

void CVehicle::AssertValidAndLoc( ) const
{
    int x, y;

    // assert it
    AssertValid( );

    if ( ( m_maploc.x == 0 ) && ( m_maploc.y == 0 ) )
        return;

    if ( ( m_cMode != moving ) && ( m_cMode != contention ) )
    {
        if ( abs( m_ptHead.x - m_ptTail.x ) <= 2 )
            x = ( m_ptHead.x + m_ptTail.x ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
        else
        {
            x = ( m_ptHead.x + m_ptTail.x + theMap.Get_eX( ) * 2 ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
            if ( x >= theMap.Get_eX( ) * MAX_HEX_HT )
                x -= theMap.Get_eX( ) * MAX_HEX_HT;
        }

#ifdef STRICTER_ASSERTS
        ASSERT( x == m_maploc.x );
#endif

        if ( abs( m_ptHead.y - m_ptTail.y ) <= 2 )
            y = ( m_ptHead.y + m_ptTail.y ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
        else
        {
            y = ( m_ptHead.y + m_ptTail.y + theMap.Get_eY( ) * 2 ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
            if ( y >= theMap.Get_eY( ) * MAX_HEX_HT )
                y -= theMap.Get_eY( ) * MAX_HEX_HT;
        }
#ifdef STRICTER_ASSERTS
        ASSERT( y == m_maploc.y );
#endif

        if ( !( GetData( )->GetVehFlags( ) & CTransportData::FL1hex ) )
#ifndef _GG
#if STRICTER_ASSERTS
            // triggers on every vehicle spawn
            ASSERT( m_iDir == aiDir[GetDirIndex( m_ptHead, m_ptTail )] )
#endif
#endif
                ;
    }

    if ( m_cMode == moving )
    {
        if ( GetData( )->GetVehFlags( ) & CTransportData::FL1hex )
            x = m_ptNext.x * MAX_HEX_HT / 2 + MAX_HEX_HT / 4;
        else
        {
            if ( abs( m_ptNext.x - m_ptHead.x ) <= 2 )
                x = ( m_ptNext.x + m_ptHead.x ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
            else
            {
                x = ( m_ptNext.x + m_ptHead.x + theMap.Get_eX( ) * 2 ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
                if ( x >= theMap.Get_eX( ) * MAX_HEX_HT )
                    x -= theMap.Get_eX( ) * MAX_HEX_HT;
            }
        }
        int x2 = m_maploc.x + m_iXadd * m_iStepsLeft;
        if ( x2 < 0 )
            x2 += theMap.Get_eX( ) * MAX_HEX_HT;
        else if ( x2 >= theMap.Get_eX( ) * MAX_HEX_HT )
            x2 -= theMap.Get_eX( ) * MAX_HEX_HT;

#ifdef STRICTER_ASSERTS
        ASSERT( x == x2 );
#endif

        if ( GetData( )->GetVehFlags( ) & CTransportData::FL1hex )
            y = m_ptNext.y * MAX_HEX_HT / 2 + MAX_HEX_HT / 4;
        else
        {
            if ( abs( m_ptNext.y - m_ptHead.y ) <= 2 )
                y = ( m_ptNext.y + m_ptHead.y ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
            else
            {
                y = ( m_ptNext.y + m_ptHead.y + theMap.Get_eY( ) * 2 ) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
                if ( y >= theMap.Get_eY( ) * MAX_HEX_HT )
                    y -= theMap.Get_eX( ) * MAX_HEX_HT;
            }
        }

        int y2 = m_maploc.y + m_iYadd * m_iStepsLeft;
        if ( y2 < 0 )
            y2 += theMap.Get_eY( ) * MAX_HEX_HT;
        else if ( y2 >= theMap.Get_eY( ) * MAX_HEX_HT )
            y2 -= theMap.Get_eY( ) * MAX_HEX_HT;
        ASSERT( y == y2 );

        if ( m_ptNext != m_ptHead )
            ASSERT( __roll( 0, FULL_ROT, m_iDir + m_iDadd * m_iStepsLeft ) ==
                    aiDir[GetDirIndex( m_ptNext, m_ptHead )] );
    }
}

//--------------------------------------------------------------------------
// CTurret::AssertValid
//--------------------------------------------------------------------------
void CTurret::AssertValid( ) const
{
    // assert base object
    CUnitTile::AssertValid( );

    ASSERT( 0 <= m_iDir && m_iDir < FULL_ROT );
}

//--------------------------------------------------------------------------
// CFlag::AssertValid
//--------------------------------------------------------------------------
void CFlag::AssertValid( ) const
{
    // assert base object
    CUnitTile::AssertValid( );

    // GGFIXIT: assert m_iPlayer is valid
}


#endif
