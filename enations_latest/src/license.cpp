// License.cpp : implementation file
//
// CDlgLicense — Non-MFC license dialog.

#include "stdafx.h"
#include "lastplnt.h"
#include "License.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgLicense::CDlgLicense(int iText, BOOL bOK, CWnd* /*pParent*/)
{
    m_iText = iText;
    m_bOK = bOK;
}

int CDlgLicense::DoModal()
{
    // Load license text from data file (LANG/LEGL/LICn chunk)
    std::string strLicense;
    try
    {
        CMmio* pMmio = theDataFile.OpenAsMMIO( NULL, "LANG" );
        pMmio->DescendRiff( 'L', 'A', 'N', 'G' );
        pMmio->DescendList( 'L', 'E', 'G', 'L' );
        pMmio->DescendChunk( 'L', 'I', 'C', (char)( '0' + m_iText ) );

        long lSize = pMmio->ReadLong();
        std::vector<char> buf( lSize + 2, 0 );
        pMmio->Read( buf.data(), lSize );
        strLicense.assign( buf.data(), lSize );

        delete pMmio;
    }
    catch ( ... )
    {
        strLicense = "(License text unavailable)";
    }

    UINT uType = MB_TASKMODAL;
    if ( m_bOK )
        uType |= MB_OK | MB_ICONINFORMATION;
    else
        uType |= MB_YESNO | MB_ICONINFORMATION;

    int iRtn = ::MessageBoxA( NULL, strLicense.c_str(), "Enemy Nations - License", uType );

    // Map Yes -> OK for callers that check IDOK
    if ( iRtn == IDYES )
        iRtn = IDOK;
    // Map No -> Cancel for callers that check IDCANCEL
    if ( iRtn == IDNO )
        iRtn = IDCANCEL;

    return iRtn;
}
