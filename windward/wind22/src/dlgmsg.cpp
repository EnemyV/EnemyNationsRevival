// DlgMsg.cpp : implementation file
//
// CDlgMsg — No longer CDialog-based, no longer CString-based.
// Uses Win32 MessageBoxA with registry-based suppression.

#include "stdafx.h"
#include "_windwrd.h"
#include "DlgMsg.h"
#include "w22_settings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg

int CDlgMsg::MsgBox (UINT nIDPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault)
{
 char buf[1024];
 int len = ::LoadStringA( ::GetModuleHandleA(NULL), nIDPrompt, buf, (int)sizeof(buf) );
 if ( len <= 0 )
  buf[0] = '\0';
 return ( MsgBox ( buf, nType, psEntry, psSection, iDefault ) );
}

int CDlgMsg::MsgBox (char const * psPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault)
{
 // if they don't want to be warned - ok
 if ( w22::GetProfileInt( psEntry, psSection, 0 ) != 0 )
  return iDefault;

 // Use Win32 MessageBox instead of MFC DoModal
 UINT uType = MB_TASKMODAL;

 // Map button style
 if ( nType & MB_YESNOCANCEL )
  uType |= MB_YESNOCANCEL;
 else if ( nType & MB_YESNO )
  uType |= MB_YESNO;
 else
  uType |= MB_OK;

 // Map icon style
 if ( nType & MB_ICONSTOP )
  uType |= MB_ICONSTOP;
 else if ( nType & MB_ICONEXCLAMATION )
  uType |= MB_ICONEXCLAMATION;
 else if ( nType & MB_ICONINFORMATION )
  uType |= MB_ICONINFORMATION;

 HWND hParent = w22::GetMainHWND();

 int iRtn = ::MessageBoxA( hParent, psPrompt ? psPrompt : "", "Enemy Nations", uType );

 // Map OK->YES for callers that expect IDYES (original mapped IDOK->IDYES)
 if ( iRtn == IDOK )
  iRtn = IDYES;

 // Suppress future prompts — original only suppressed if checkbox was checked.
 // Without a checkbox, we always suppress after user responds.
 w22::WriteProfileInt( psEntry, psSection, 1 );

 return ( iRtn );
}
