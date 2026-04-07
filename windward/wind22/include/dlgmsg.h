#ifndef __DLGMSG_H__
#define __DLGMSG_H__

// DlgMsg.h : header file
//
// CDlgMsg — message box with "don't show again" registry suppression.
// No longer inherits CDialog. Uses Win32 MessageBoxA internally.

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg

class CDlgMsg
{
// Construction
public:
 CDlgMsg(CWnd* pParent = NULL);

 int  MsgBox (char const * psPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );
 int  MsgBox (UINT nIDPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );

 CString  m_sSection;  // for check box
 CString  m_sEntry;
 BOOL m_btnCheck;
 CString m_sText;
};

#endif
