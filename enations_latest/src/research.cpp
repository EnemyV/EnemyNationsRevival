//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// research.cpp : implementation file
//

#include "research.h"

#include "area.h"
#include "SDL2GameDialogs.h"
#include "SDL2MFCPanel.h"
#include "bitmaps.h"
#include "building.inl"
#include "icons.h"
#include "lastplnt.h"
#include "stdafx.h"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CRsrchArray theRsrch;

void ResearchDiscovered( int iItem )
{

    if ( iItem > 0 )
        theGame.GetMe( )->m_iNumDiscovered++;

    // Live-refresh the research window if it's open (non-modal). Mirrors the
    // original ResearchDiscovered -> CDlgResearch::UpdateChoices( TRUE ): the
    // just-discovered topic drops off the list, newly unlocked topics appear, and
    // the stale "current research" marker clears. Without this the open window's
    // list stays frozen until reopened, and arms the "Discovery" button to re-show
    // this item's result text.
    theApp.m_wndBar.RefreshResearch( iItem );

    // check and update the unit build dialogs
    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
            pBldg->UpdateChoices( );
    }

    // check and update the vehicle build dialogs
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh->GetOwner( )->IsMe( ) )
            pVeh->UpdateChoices( );
    }

    // copper discovered
    if ( iItem == CRsrchArray::copper )
        theAreaList.XilDiscovered( );
}


/////////////////////////////////////////////////////////////////////////////
// CRsrchStatus

CRsrchStatus::CRsrchStatus( )
{

    m_bDiscovered    = FALSE;
    m_iPtsDiscovered = 0;
}

void CRsrchStatus::Serialize( CArchive& ar )
{

    if ( ar.IsStoring( ) )
    {
        ASSERT_VALID( this );
        //TRAP( );

        ar << m_bDiscovered << m_iPtsDiscovered;
    }
    else
    {
        //TRAP( );
        ar >> m_bDiscovered >> m_iPtsDiscovered;
    }
}

void ConstructElements( CRsrchStatus* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchStatus::CRsrchStatus( );
}

void DestructElements( CRsrchStatus* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchStatus::~CRsrchStatus( );
}

void SerializeElements( CArchive& ar, CRsrchStatus* pData, int iCount )
{
    for ( int i = 0; i < iCount; i++ ) pData[i].Serialize( ar );
}


/////////////////////////////////////////////////////////////////////////////
// CRsrchItem

CRsrchItem::CRsrchItem( )
{

    m_iPtsRequired      = 0;
    m_iNumRsrchRequired = 0;
    m_piRsrchRequired   = NULL;
    m_iNumBldgsRequired = 0;
    m_piBldgsRequired   = NULL;
    m_iScenarioReq      = 0;
}

CRsrchItem::~CRsrchItem( )
{

    delete[] m_piRsrchRequired;
    delete[] m_piBldgsRequired;
}

void ConstructElements( CRsrchItem* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchItem::CRsrchItem( );
}

void DestructElements( CRsrchItem* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchItem::~CRsrchItem( );
}

#ifdef _DEBUG
void CRsrchItem::AssertValid( ) const
{

    // assert the base class
    CObject::AssertValid( );
    TRAP( );

    int iOn = 0;
    for ( iOn = 0; iOn < m_iNumRsrchRequired; iOn++ )
        ASSERT( ( 0 < m_piRsrchRequired[iOn] ) && ( m_piRsrchRequired[iOn] < theRsrch.GetSize( ) ) );
    for ( iOn = 0; iOn < m_iNumBldgsRequired; iOn++ )
        ASSERT( ( 0 < m_piBldgsRequired[iOn] ) && ( m_piBldgsRequired[iOn] < theStructures.GetNumBuildings( ) ) );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CRsrchArray - the R&D data

void CRsrchArray::Open( )
{

    ASSERT_VALID( this );
    ASSERT( GetSize( ) == 0 );

    // read in the RIF data
    CMmio* pMmio = theDataFile.OpenAsMMIO( "research", "RSRH" );

    pMmio->DescendRiff( 'R', 'S', 'R', 'H' );
    pMmio->DescendList( 'I', 'T', 'M', 'S' );

    pMmio->DescendChunk( 'N', 'U', 'M', 'I' );
    int iSize = pMmio->ReadShort( );
    ASSERT( iSize + 1 == bridge_2 );  // DAT topics end at acc_3; bridge_2..5 are in-code
    pMmio->AscendChunk( );
    SetSize( num_types );

    // read in the per/item stuff
    // note - we make R&D level 0 discovered
    for ( int iOn = 0; iOn < iSize; iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn + 1 );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        pRi->m_iPtsRequired = pMmio->ReadLong( );
        pRi->m_iScenarioReq = pMmio->ReadLong( );
        if ( ( pRi->m_iNumRsrchRequired = pMmio->ReadLong( ) ) > 0 )
        {
            int  iNum = pRi->m_iNumRsrchRequired;
            int* piOn = pRi->m_piRsrchRequired = new int[iNum];
            while ( iNum-- > 0 ) *piOn++ = pMmio->ReadLong( );
        }
        if ( ( pRi->m_iNumBldgsRequired = pMmio->ReadLong( ) ) > 0 )
        {
            int  iNum = pRi->m_iNumBldgsRequired;
            int* piOn = pRi->m_piBldgsRequired = new int[iNum];
            while ( iNum-- > 0 ) *piOn++ = pMmio->ReadLong( );
        }
        pMmio->AscendChunk( );
    }
    pMmio->AscendList( );

    delete pMmio;

    // get the text
    pMmio = theDataFile.OpenAsMMIO( NULL, "LANG" );

    pMmio->DescendRiff( 'L', 'A', 'N', 'G' );
    pMmio->DescendList( 'R', 'S', 'R', 'H' );

    for ( int iOn = 0; iOn < iSize; iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn + 1 );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        pMmio->ReadString( pRi->m_sName );
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'D', 'E', 'S', 'C' );
        pMmio->ReadString( pRi->m_sDesc );
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'R', 'S', 'L', 'T' );
        pMmio->ReadString( pRi->m_sResult );
        pMmio->AscendChunk( );
    }
    pMmio->AscendList( );
    delete pMmio;

    // In-code research topics: Bridges 2-5 (not in the DAT file). Each tier costs
    // double the previous tier's points, requires the previous tier, and extends
    // the max bridge span by +25% of the base span (see CPlayer::GetMaxSpan).
    {
        static char const* aszName[4] = { "Bridges 2", "Bridges 3", "Bridges 4", "Bridges 5" };
        static char const* aszDesc[4] = {
            "Improved trusses let bridges span 25% more water than the original design.",
            "Bridges can span 50% more water than the original design.",
            "Bridges can span 75% more water than the original design.",
            "Bridges can span twice as much water as the original design." };
        static char const* aszRslt[4] = {
            "Our engineers can now build bridges 25% longer.",
            "Our engineers can now build bridges 50% longer.",
            "Our engineers can now build bridges 75% longer.",
            "Our engineers can now build bridges twice as long." };

        int iPts = ElementAt( bridge ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 4; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( bridge_2 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( bridge ).m_iScenarioReq;
            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = ( 0 == iOn ) ? (int)bridge : (int)( bridge_2 + iOn - 1 );
            pRi->m_sName              = aszName[iOn];
            pRi->m_sDesc              = aszDesc[iOn];
            pRi->m_sResult            = aszRslt[iOn];
        }
    }

#ifdef _DEBUG
    theDataFile.DisableNegativeSeekChecking( );
    theDataFile.EnableNegativeSeekChecking( );
#endif

    ASSERT_VALID( this );
}

void CRsrchArray::Close( )
{

    ASSERT_VALID( this );

    for ( int iOn = 0; iOn < GetSize( ); iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn );
        delete pRi->m_piRsrchRequired;
        delete pRi->m_piBldgsRequired;
        pRi->m_piRsrchRequired = NULL;
        pRi->m_piBldgsRequired = NULL;
    }

    RemoveAll( );
}

