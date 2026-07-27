#ifndef __DLGMSG_H__
#define __DLGMSG_H__

// DlgMsg.h : header file
//
// CDlgMsg — message box with "don't show again" registry suppression.
// Uses Win32 MessageBoxA internally. No MFC dependency.

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg

class CDlgMsg
{
// Construction
public:
 CDlgMsg() : m_btnCheck(FALSE) {}

 int  MsgBox (char const * psPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );
 int  MsgBox (UINT nIDPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );

 BOOL m_btnCheck;
};

#endif
