// License.cpp : implementation file
//
// CDlgLicense — SDL2-rendered license dialog. Loads license text from
// LANG/LEGL/LICn data-file chunk and displays it modally with either
// an OK button (read-only) or a Yes/Cancel pair (acceptance prompt).

#include "stdafx.h"
#include "lastplnt.h"
#include "license.h"
#include "GameWindow.h"
#include "SDL2UI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgLicense::CDlgLicense( int iText, BOOL bOK, CWnd* /*pParent*/ )
    : m_iText( iText )
    , m_bOK( bOK )
{
}

namespace {

class LicenseDialog : public SDL2Dialog
{
public:
    LicenseDialog( GameWindow* gw, const std::string& text, bool readOnly )
        : SDL2Dialog( gw, "Enemy Nations - License", 640, 480 )
        , m_text( text )
        , m_readOnly( readOnly )
    {
    }

protected:
    void OnInit() override
    {
        const int margin = 12;
        const int btnH   = 26;
        const int btnY   = m_y + m_height - btnH - 10;

        // License text fills the dialog above the buttons.
        auto* lbl = AddWidget<SDL2Label>(
            m_x + margin, m_y + 30,
            m_width - margin * 2, btnY - ( m_y + 30 ) - 8,
            m_text );
        lbl->SetWrapped( true );
        lbl->SetTopAligned( true );

        if ( m_readOnly )
        {
            // OK button centered.
            const int bw = 80;
            AddWidget<SDL2Button>(
                m_x + ( m_width - bw ) / 2, btnY, bw, btnH, "OK",
                [this]() { EndDialog( IDOK ); } );
        }
        else
        {
            // Yes / Cancel pair — Yes accepts, Cancel rejects.
            const int bw  = 90;
            const int gap = 20;
            const int totalW = bw * 2 + gap;
            const int startX = m_x + ( m_width - totalW ) / 2;
            AddWidget<SDL2Button>(
                startX, btnY, bw, btnH, "Yes",
                [this]() { EndDialog( IDOK ); } );
            AddWidget<SDL2Button>(
                startX + bw + gap, btnY, bw, btnH, "Cancel",
                [this]() { EndDialog( IDCANCEL ); } );
        }
    }

    void OnCancel() override { EndDialog( IDCANCEL ); }

private:
    std::string m_text;
    bool        m_readOnly;
};

}  // namespace

int CDlgLicense::DoModal()
{
    // Load license text from data file (LANG/LEGL/LICn chunk).
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

    GameWindow* gw = theApp.m_gameWindow ? theApp.m_gameWindow.get() : nullptr;
    // The demo license is shown during early InitInstance, before the SDL game
    // window exists (created much later). An SDL2Dialog needs a parent
    // GameWindow, so with none yet we can't show it — accept by default.
    if ( !gw )
        return IDOK;
    LicenseDialog dlg( gw, strLicense, m_bOK != FALSE );
    return dlg.DoModal();
}
