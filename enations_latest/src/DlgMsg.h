// DlgMsg.h : header file
//
// CDlgModelessMsg — notification popup that the user dismisses by
// clicking OK. Rendered with the SDL2 dialog toolkit (same look as the
// rest of the in-game dialogs).
//
// Self-deletes after dismiss: callers do `new CDlgModelessMsg(); pDlg->Create(msg)`
// and forget. GameWindow's non-modal dialog plumbing handles the
// teardown after EndDialog fires.

#pragma once

#include "SDL2UI.h"
#include <string>

class CWnd;

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg

class CDlgModelessMsg : public SDL2Dialog
{
public:
    CDlgModelessMsg( CWnd* pParent = NULL );  // pParent kept for API compat; unused
    ~CDlgModelessMsg() override;

    // Set message text and open the dialog non-modally. The dialog
    // self-destructs after the user clicks OK (GameWindow cleanup pass).
    void Create( const char* pMsg );

    std::string m_sMsg;

protected:
    void OnInit() override;
};
