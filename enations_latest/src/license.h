// License.h : header file
//
// CDlgLicense — Non-MFC license dialog.
// Uses Win32 MessageBoxA to display license text from LANG data.

#ifndef __LICENSE_H__
#define __LICENSE_H__

class CDlgLicense
{
public:
    // iText: which license chunk (0-based) to load from LANG/LEGL
    // bOK: TRUE = read-only (OK button), FALSE = acceptance (Yes/Cancel)
    CDlgLicense(int iText, BOOL bOK = FALSE, CWnd* pParent = NULL);

    // Returns IDOK on acceptance, IDCANCEL on rejection
    int DoModal();

    int  m_iText;
    BOOL m_bOK;
};

#endif // __LICENSE_H__
