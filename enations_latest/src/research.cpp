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

    if ( ( theApp.m_pdlgRsrch != NULL ) && ( theApp.m_pdlgRsrch->m_hWnd != NULL ) )
    {
        if ( iItem > 0 )
            theApp.m_pdlgRsrch->ItemDiscovered( iItem );
        theApp.m_pdlgRsrch->UpdateChoices( TRUE );
    }

    // check the unit build dialogs
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
        TRAP( );

        ar << m_bDiscovered << m_iPtsDiscovered;
    }
    else
    {
        TRAP( );
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
    ASSERT( iSize + 1 == num_types );
    pMmio->AscendChunk( );
    SetSize( iSize + 1 );

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


/////////////////////////////////////////////////////////////////////////////
// CDlgResearch dialog

CDlgResearch::CDlgResearch( CWnd* pParent /*=NULL*/ ): CDialog( CDlgResearch::IDD, pParent )
{
    //{{AFX_DATA_INIT(CDlgResearch)
    //}}AFX_DATA_INIT

    m_pDib = m_pDibBtn = NULL;
    m_pDlgDiscvr       = NULL;
    m_iRsrchNum        = 0;
    m_iPerDone         = 0;
    m_strDesc          = _T("");
    m_bShareLimit      = FALSE;
}


void CDlgResearch::DoDataExchange( CDataExchange* pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CDlgResearch)
    DDX_Control( pDX, IDC_RSRCH_FOUND, m_btnDiscovery );
    DDX_Control( pDX, IDC_RSRCH_LIST, m_lstRsrch );
    //}}AFX_DATA_MAP
}

int CResearchListBox::CompareItem( LPCOMPAREITEMSTRUCT lpCompareItemStruct )
{
    CString str1, str2;

    GetText( lpCompareItemStruct->itemID1, str1 );
    GetText( lpCompareItemStruct->itemID2, str2 );

    return str1.Compare( str2 );  // negative if str1<str2, 0 if equal, positive if str1>str2
}


BEGIN_MESSAGE_MAP( CDlgResearch, CDialog )
//{{AFX_MSG_MAP(CDlgResearch)
ON_LBN_DBLCLK( IDC_RSRCH_LIST, OnDblclkRsrchList )
ON_LBN_SELCHANGE( IDC_RSRCH_LIST, OnSelchangeRsrchList )
ON_WM_SYSCOMMAND( )
ON_WM_DESTROY( )
ON_WM_MOVE( )
ON_WM_ERASEBKGND( )
ON_WM_MEASUREITEM( )
ON_WM_DRAWITEM( )
ON_WM_PAINT( )
ON_BN_CLICKED( IDC_RSRCH_FOUND, OnRsrchFound )
ON_WM_PALETTECHANGED( )
ON_WM_QUERYNEWPALETTE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CDlgResearch message handlers

BOOL CDlgResearch::OnEraseBkgnd( CDC* )
{

    return ( 1 );
}

const CRect rectText( 192, 37, 420, 156 );

void CDlgResearch::OnPaletteChanged( CWnd* pFocusWnd )
{
    static BOOL bInFunc = FALSE;

    CDialog::OnPaletteChanged( pFocusWnd );

    // Win32s locks up if we do the below code
    if ( iWinType == W32s )
        return;

    // stop infinite recursion
    if ( bInFunc )
        return;
    bInFunc = TRUE;

    CClientDC dc( this );
    int       iRtn = thePal.PalMsg( dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM)pFocusWnd->m_hWnd, 0 );

    // invalidate the window
    if ( iRtn )
        InvalidateRect( NULL );

    SendMessage( WM_NCPAINT, 0, 0 );

    bInFunc = FALSE;
}

BOOL CDlgResearch::OnQueryNewPalette( )
{

    if ( iWinType == W32s )
        return CDialog::OnQueryNewPalette( );

    CClientDC dc( this );
    thePal.PalMsg( dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0 );

    SendMessage( WM_NCPAINT, 0, 0 );
    return CDialog::OnQueryNewPalette( );
}

void CDlgResearch::OnPaint( )
{

    ASSERT_VALID( this );

    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    // paint background (stretch in case...)
    CRect rect;
    GetClientRect( &rect );
    CDIB* pdib = theBitmaps.GetByIndex( DIB_RSRCH_BKGND );
    pdib->StretchBlt( m_pDib, rect, pdib->GetRect( ) );

    // paint the light bulbs
    m_statInst.m_pDib->Clear( NULL, 253 );
    m_statInst.DrawStatDone( );
    m_statInst.m_pDib->TranBlt( m_pDib, m_statInst.m_rDest, CPoint( 0, 0 ) );

    // paint the text
    CDC* pDcTxt = CDC::FromHandle( m_pDib->GetDC( ) );
    if ( pDcTxt == NULL )
    {
        thePal.EndPaint( dc.m_hDC );
        return;
    }
    thePal.Paint( pDcTxt->m_hDC );
    pDcTxt->SetBkMode( TRANSPARENT );
    pDcTxt->SetTextColor( PALETTERGB( 41, 255, 8 ) );
    rect = rectText;
    FitDrawText( pDcTxt, theApp.RDFont( ), m_strDesc, rect );

    m_pDib->BitBlt( dc, m_pDib->GetRect( ), CPoint( 0, 0 ) );
    thePal.EndPaint( pDcTxt->m_hDC );
    if ( m_pDib->IsBitmapSelected( ) )
        m_pDib->ReleaseDC( );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndBase::OnPaint() for painting messages
}

void CDlgResearch::OnMeasureItem( int, LPMEASUREITEMSTRUCT lpMIS )
{

    ASSERT_VALID( this );

    CWindowDC dc( NULL );
    CFont*    pOld = NULL;
    if ( m_fntList.GetSafeHandle( ) != NULL )
        pOld = dc.SelectObject( &m_fntList );
    TEXTMETRIC tm;
    dc.GetTextMetrics( &tm );
    int iHt = __max( tm.tmHeight, theApp.TextHt( ) );

    lpMIS->itemHeight = __max( lpMIS->itemHeight, (UINT)iHt );
    if ( pOld != NULL )
        dc.SelectObject( pOld );
}

void CDlgResearch::OnDrawItem( int, LPDRAWITEMSTRUCT pDis )
{

    // buttons
    if ( ODT_BUTTON == pDis->CtlType )
    {
        CDIB* pDibBmp = theBitmaps.GetByIndex( DIB_RESEARCH_BTNS );
        CRect rSrc( pDibBmp->GetRect( ) );
        rSrc.right /= 3;
        CRect rDest( m_pDibBtn->GetRect( ) );
        if ( pDis->itemState & ( ODS_DISABLED | ODS_GRAYED ) )
            rSrc.OffsetRect( rSrc.Width( ) * 2, 0 );
        else if ( pDis->itemState & ODS_SELECTED )
        {
            rSrc.OffsetRect( rSrc.Width( ), 0 );
            rDest.OffsetRect( 2, 1 );
        }
        pDibBmp->StretchBlt( m_pDibBtn, m_pDibBtn->GetRect( ), rSrc );

        // paint the text
        CDC* pDc = CDC::FromHandle( m_pDibBtn->GetDC( ) );
        if ( pDc == NULL )
            return;
        pDc->SetBkMode( TRANSPARENT );
        pDc->SetTextColor( CLR_UNIT_BUILD );
        pDc->SetTextColor( PALETTERGB( 255, 33, 8 ) );
        CString sText;
        GetDlgItem( pDis->CtlID )->GetWindowText( sText );
        pDc->DrawText( sText, -1, &rDest,
                       DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORDBREAK | DT_NOPREFIX );

        // BLT to the screen DC
        thePal.Paint( pDis->hDC );
        m_pDibBtn->BitBlt( pDis->hDC, &( pDis->rcItem ), CPoint( 0, 0 ) );

        thePal.EndPaint( pDis->hDC );
        if ( m_pDibBtn->IsBitmapSelected( ) )
            m_pDibBtn->ReleaseDC( );
        return;
    }

    // listbox
    CDC* pDc = CDC::FromHandle( pDis->hDC );
    if ( pDc == NULL )
        return;
    thePal.Paint( pDc->m_hDC );

    CFont* pOld = pDc->SelectObject( &m_fntList );
    pDc->SetTextColor( PALETTERGB( 255, 33, 8 ) );
    if ( pDis->itemState & ODS_SELECTED )
        pDc->SetBkColor( RGB( 0, 0, 0 ) );
    else
        pDc->SetBkColor( RGB( 255, 255, 255 ) );

    CRsrchItem* pRi = &theRsrch.ElementAt( pDis->itemData );

    // if it won't fit use a smaller font
    CFont fntTemp;
    CSize sz = pDc->GetTextExtent( pRi->m_sName );
    if ( sz.cx > pDis->rcItem.right - pDis->rcItem.left )
    {
        LOGFONT lf;
        memset( &lf, 0, sizeof( lf ) );
        m_fntList.GetLogFont( &lf );
        int iHt = lf.lfHeight;
        while ( iHt-- > 10 )
        {
            memset( &lf, 0, sizeof( lf ) );
            m_fntList.GetLogFont( &lf );
            strncpy( lf.lfFaceName, "Arial Narrow", LF_FACESIZE - 1 );
            lf.lfHeight = iHt;
            if ( fntTemp.m_hObject != NULL )
            {
                pDc->SelectObject( &m_fntList );
                fntTemp.DeleteObject( );
            }
            fntTemp.CreateFontIndirect( &lf );

            pDc->SelectObject( &fntTemp );
            sz = pDc->GetTextExtent( pRi->m_sName );
            if ( sz.cx < pDis->rcItem.right - pDis->rcItem.left )
                break;
        }
    }

    pDc->ExtTextOut( pDis->rcItem.left, pDis->rcItem.top, ETO_CLIPPED | ETO_OPAQUE, &( pDis->rcItem ), pRi->m_sName,
                     strlen( pRi->m_sName ), NULL );
    pDc->SelectObject( pOld );
    thePal.EndPaint( pDc->m_hDC );
}

void CDlgResearch::OnOK( )
{

    int iSel = m_lstRsrch.GetCurSel( );
    if ( iSel < 0 )
        return;
    iSel = m_lstRsrch.GetItemData( iSel );
    if ( iSel < 0 )
        return;

    // if we are on something NEW reduce it 10%
    int iOn = theGame.GetMe( )->GetRsrchItem( );
    if ( ( iOn > 0 ) && ( iOn != iSel ) )
    {
        CRsrchStatus* pRs = &( theGame.GetMe( )->GetRsrch( theGame.GetMe( )->GetRsrchItem( ) ) );
        pRs->m_iPtsDiscovered -= pRs->m_iPtsDiscovered / 10;
    }

    // set the topic to research
    theGame.GetMe( )->SetRsrchItem( iSel );

    // list what we are researching
    UpdateData( TRUE );
    CString sTitle;
    if ( iSel < 0 )
        sTitle.LoadString( IDS_RSRCH_NOTHING );
    else
    {
        sTitle.LoadString( IDS_RSRCH_TITLE );
        csPrintf( &sTitle, (char const*)theRsrch.ElementAt( iSel ).m_sName );
    }
    SetWindowText( sTitle );

    UpdateProgress( );
}

void CDlgResearch::ItemDiscovered( int iItem )
{

    m_iRsrchNum = iItem;
    m_btnDiscovery.EnableWindow( TRUE );

    if ( m_pDlgDiscvr != NULL )
        m_pDlgDiscvr->ItemDiscovered( iItem );

    m_iPerDone = 0;
    CString sTitle;
    sTitle.LoadString( IDS_RSRCH_NOTHING );
    SetWindowText( sTitle );

    UpdateProgress( );
}

void CDlgResearch::UpdateProgress( )
{

    int iSel = theGame.GetMe( )->GetRsrchItem( );
    if ( iSel <= 0 )
        m_iPerDone = 0;
    else
    {
        m_iPerDone = ( theGame.GetMe( )->GetRsrch( theGame.GetMe( )->GetRsrchItem( ) ).m_iPtsDiscovered * 50 ) /
                     theRsrch.ElementAt( iSel ).m_iPtsRequired;
        m_iPerDone = __min( m_iPerDone, 99 );
        m_iPerDone = __max( 1, m_iPerDone );
    }

    // draw the new bar
    m_statInst.SetPer( m_iPerDone );
    InvalidateRect( &( m_statInst.m_rDest ), FALSE );
}

void CDlgResearch::OnCancel( )
{

    DestroyWindow( );
}

BOOL CDlgResearch::OnInitDialog( )
{

    CDialog::OnInitDialog( );

    // need a scratch paint buffer
    m_pDib = new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY, CBLTFormat::DIR_BOTTOMUP, 447, 291 );
    m_pDibBtn =
        new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY, CBLTFormat::DIR_BOTTOMUP, 116, 25 );

    // set the size, check the location
    CRect rWin, rClnt;
    GetWindowRect( &rWin );
    GetClientRect( &rClnt );
    SetWindowPos( NULL, __max( 0, rWin.right ), __max( 0, rWin.top ), 447 + rWin.Width( ) - rClnt.Width( ),
                  291 + rWin.Height( ) - rClnt.Height( ), SWP_NOZORDER );

    // the light bulbs
    CRect rect( 17, 248, 425, 272 );
    m_statInst.Attach( &theIcons, ICON_RESEARCH );
    m_statInst.SetSize( rect );
    m_statInst.SetPer( 0 );

    m_lstRsrch.SetWindowPos( NULL, 18, 35, 155, 198, SWP_NOZORDER );
    GetDlgItem( IDOK )->SetWindowPos( NULL, 188, 170, 116, 25, SWP_NOZORDER );
    m_btnDiscovery.SetWindowPos( NULL, 311, 211, 116, 25, SWP_NOZORDER );
    GetDlgItem( IDCANCEL )->SetWindowPos( NULL, 311, 170, 116, 25, SWP_NOZORDER );

    // font for the listbox
    LOGFONT lf;
    memset( &lf, 0, sizeof( lf ) );
    lf.lfHeight   = theApp.GetProfileInt( "StatusBar", "RDListHeight", 18 );
    CString sFont = theApp.GetProfileString( "StatusBar", "RDListFont", "Newtown Italic" );
    strncpy( lf.lfFaceName, sFont, LF_FACESIZE - 1 );
    m_fntList.CreateFontIndirect( &lf );

    // if rebuilding use old pos
    if ( theGame.m_wpRsrch.length == 0 )
    {
        // pos below world map
        SetWindowPos( NULL, 0, theApp.m_iRow1, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );

        // save position
        theGame.m_wpRsrch.length = sizeof( WINDOWPLACEMENT );
        GetWindowPlacement( &( theGame.m_wpRsrch ) );
    }
    else
        SetWindowPos( NULL, theGame.m_wpRsrch.rcNormalPosition.left, theGame.m_wpRsrch.rcNormalPosition.top, 0, 0,
                      SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );

    if ( ( theGame.GetMe( )->m_iNumDiscovered >= DEMO_RESEARCH_LIMIT ) &&
         ( ( theApp.IsShareware( ) ) || ( ( theApp.IsSecondDisk( ) ) && ( !theGame.IsNetGame( ) ) ) ) )
    {
        TRAP( );
        m_bShareLimit = TRUE;
        m_strDesc.LoadString( IDS_DEMO_RESEARCH_MAX );
    }

    UpdateChoices( TRUE );

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgResearch::OnDblclkRsrchList( )
{

    UpdateChoices( FALSE );
    OnOK( );
}

void CDlgResearch::OnSelchangeRsrchList( )
{

    UpdateChoices( FALSE );
}

void CDlgResearch::UpdateChoices( BOOL bCheck )
{

    // limit of 6 in shareware version
    if ( ( theGame.GetMe( )->m_iNumDiscovered >= DEMO_RESEARCH_LIMIT ) &&
         ( ( theApp.IsShareware( ) ) || ( ( theApp.IsSecondDisk( ) ) && ( !theGame.IsNetGame( ) ) ) ) )
    {
        TRAP( );
        m_bShareLimit = TRUE;
        m_strDesc.LoadString( IDS_DEMO_RESEARCH_MAX );
        InvalidateRect( &rectText, FALSE );
        m_lstRsrch.ResetContent( );
        GetDlgItem( IDOK )->EnableWindow( FALSE );
        bCheck = FALSE;
    }

    // do we redetermine what can be researched?
    if ( bCheck )
    {
        // we want to select the one we were on before
        int iSel = m_lstRsrch.GetCurSel( );
        if ( iSel >= 0 )
            iSel = m_lstRsrch.GetItemData( iSel );
        m_lstRsrch.ResetContent( );

        for ( int iOn = 0; iOn < theRsrch.GetSize( ); iOn++ )
        {
            CRsrchItem* pRi = &theRsrch.ElementAt( iOn );

            if ( !theGame.GetMe( )->GetRsrch( iOn ).m_bDiscovered )
            {
                // no comm stuff in single player
                if ( theGame.GetServerNetNum( ) == 0 )
                    if ( ( iOn == CRsrchArray::mail ) || ( iOn == CRsrchArray::email ) ||
                         ( iOn == CRsrchArray::telephone ) )
                        goto skip_it;

                // no xilitium
                if ( ( iOn == CRsrchArray::copper ) &&
                     ( ( theApp.IsShareware( ) ) || ( ( theApp.IsSecondDisk( ) ) && ( !theGame.IsNetGame( ) ) ) ) )
                    goto skip_it;

                // now we have to see if we CAN research it
                if ( ( theGame.GetScenario( ) != -1 ) && ( theGame.GetScenario( ) < pRi->m_iScenarioReq ) )
                    goto skip_it;
                for ( int iNum = 0, *piNum = pRi->m_piRsrchRequired; iNum < pRi->m_iNumRsrchRequired; iNum++, piNum++ )
                    if ( !theGame.GetMe( )->GetRsrch( *piNum ).m_bDiscovered )
                        goto skip_it;
                for ( int iNum = 0, *piNum = pRi->m_piBldgsRequired; iNum < pRi->m_iNumBldgsRequired; iNum++, piNum++ )
                    if ( !theGame.GetMe( )->GetExists( *piNum ) )
                        goto skip_it;

                // ok, we can add it
                int iInd = m_lstRsrch.AddString( pRi->m_sName );
                m_lstRsrch.SetItemData( iInd, iOn );
            }
        skip_it:;
        }

        // re-select what was selected (if possible)
        if ( iSel >= 0 )
        {
            int iNum = m_lstRsrch.GetCount( );
            for ( int iOn = 0; iOn < iNum; iOn++ )
                if ( (int)m_lstRsrch.GetItemData( iOn ) == iSel )
                {
                    m_lstRsrch.SetCurSel( iOn );
                    break;
                }
        }

        // list what we are researching
        CString sTitle;
        if ( theGame.GetMe( )->GetRsrchItem( ) <= 0 )
            sTitle.LoadString( IDS_RSRCH_NOTHING );
        else
        {
            sTitle.LoadString( IDS_RSRCH_TITLE );
            csPrintf( &sTitle, (char const*)theRsrch.ElementAt( theGame.GetMe( )->GetRsrchItem( ) ).m_sName );
        }
        SetWindowText( sTitle );
    }

    UpdateData( TRUE );
    // list the item we are on
    int iSel = m_lstRsrch.GetCurSel( );
    if ( iSel < 0 )
    {
        if ( m_bShareLimit )
            m_strDesc.LoadString( IDS_DEMO_RESEARCH_MAX );
        else
            m_strDesc = "";
    }
    else
        m_strDesc = theRsrch.ElementAt( m_lstRsrch.GetItemData( iSel ) ).m_sDesc;
    UpdateData( FALSE );

    GetDlgItem( IDOK )->EnableWindow( m_lstRsrch.GetCurSel( ) >= 0 );

    InvalidateRect( &rectText, FALSE );

    UpdateProgress( );
}

void CDlgResearch::OnDestroy( )
{

    theGame.m_wpRsrch.length = sizeof( WINDOWPLACEMENT );
    GetWindowPlacement( &( theGame.m_wpRsrch ) );

    theApp.m_pdlgRsrch = NULL;

    delete m_pDib;
    m_pDib = NULL;
    delete m_pDibBtn;
    m_pDibBtn = NULL;

    CDialog::OnDestroy( );
}

void CDlgResearch::OnRsrchFound( )
{

    // we only let them see once
    m_btnDiscovery.EnableWindow( FALSE );

    // put up the window
    if ( m_pDlgDiscvr == NULL )
        m_pDlgDiscvr = new CDlgDiscover( this );
    if ( m_pDlgDiscvr->m_hWnd == NULL )
        m_pDlgDiscvr->Create( IDD_RSRCH_FOUND, this );

    m_pDlgDiscvr->ItemDiscovered( m_iRsrchNum );

    // only if the game is not minimized
    if ( !theApp.m_wndMain.IsIconic( ) )
        m_pDlgDiscvr->ShowWindow( SW_SHOW );
}


/////////////////////////////////////////////////////////////////////////////
// CDlgDiscover dialog


CDlgDiscover::CDlgDiscover( CWnd* pParent /*=NULL*/ ): CDialog( CDlgDiscover::IDD, pParent )
{
    //{{AFX_DATA_INIT(CDlgDiscover)
    m_strText = _T("");
    //}}AFX_DATA_INIT

    m_iRsrchNum = 0;
}


void CDlgDiscover::DoDataExchange( CDataExchange* pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CDlgDiscover)
    DDX_Text( pDX, IDC_TEXT_DISCOVERY, m_strText );
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP( CDlgDiscover, CDialog )
//{{AFX_MSG_MAP(CDlgDiscover)
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CDlgDiscover message handlers

BOOL CDlgDiscover::OnInitDialog( )
{

    CDialog::OnInitDialog( );

    NewItem( );

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgDiscover::ItemDiscovered( int iItem )
{

    m_iRsrchNum = iItem;

    // update if up
    if ( m_hWnd != NULL )
        NewItem( );
}

void CDlgDiscover::NewItem( )
{

    UpdateData( TRUE );

    // list the item we are on
    CString sTitle;
    if ( m_iRsrchNum < 0 )
    {
        m_strText = "";
        sTitle.LoadString( IDS_DISCOVERED_NO_TITLE );
    }
    else
    {
        m_strText = theRsrch.ElementAt( m_iRsrchNum ).m_sResult;
        sTitle.LoadString( IDS_DISCOVERED_TITLE );
        csPrintf( &sTitle, (char const*)theRsrch.ElementAt( m_iRsrchNum ).m_sName );
    }
    SetWindowText( sTitle );

    UpdateData( FALSE );
}
